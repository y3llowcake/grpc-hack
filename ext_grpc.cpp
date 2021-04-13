#include <iostream>

#include "hphp/runtime/ext/extension.h"
#include "hphp/runtime/base/builtin-functions.h"
#include "hphp/runtime/base/array-init.h"
#include "hphp/runtime/ext/asio/asio-external-thread-event.h"
#include "hphp/runtime/vm/native-data.h"

#include <grpc_client.h>

namespace HPHP {

struct UnaryCallResultData {
  Status status_;
  String resp_;
  std::unique_ptr<ClientContext> ctx_;
  UnaryCallResultData() : resp_("") {}
};

struct GrpcUnaryCallResult {
	static Class* s_class;
  static const StaticString s_className;	
	static Class* getClass() {
		if (s_class == nullptr) {
			s_class = Class::lookup(s_className.get());
			assertx(s_class);
		}
		return s_class;
	}

  GrpcUnaryCallResult(){};
	static Object newInstance(std::unique_ptr<UnaryCallResultData> data) {
		Object obj{getClass()};
		auto* d = Native::data<GrpcUnaryCallResult>(obj);
		d->data_ = std::move(data);
		return obj;
	}

  std::unique_ptr<UnaryCallResultData> data_;

  GrpcUnaryCallResult(const GrpcUnaryCallResult&) = delete;
  GrpcUnaryCallResult& operator=(const GrpcUnaryCallResult&) = delete;
};

Class* GrpcUnaryCallResult::s_class = nullptr;
const StaticString GrpcUnaryCallResult::s_className("GrpcUnaryCallResult");

static int HHVM_METHOD(GrpcUnaryCallResult, StatusCode) {
  auto* d = Native::data<GrpcUnaryCallResult>(this_);
	return d->data_->status_.code_;
}

static String HHVM_METHOD(GrpcUnaryCallResult, StatusMessage) {
  auto* d = Native::data<GrpcUnaryCallResult>(this_);
	return d->data_->status_.message_;
}

static String HHVM_METHOD(GrpcUnaryCallResult, StatusDetails) {
  auto* d = Native::data<GrpcUnaryCallResult>(this_);
	return d->data_->status_.details_;
}

static String HHVM_METHOD(GrpcUnaryCallResult, Response) {
  auto* d = Native::data<GrpcUnaryCallResult>(this_);
	return String(d->data_->resp_);
}

struct GrpcEvent final : AsioExternalThreadEvent, GrpcClientUnaryResultEvent {
 public:
  GrpcEvent(const String& req):
    data_(std::make_unique<UnaryCallResultData>()),
    req_(req) {}

  void Done(Status s) override {
    data_->status_ = s;
    markAsFinished();
    // TODO release req_ here?
  }

  void FillRequest(const void** c, size_t* l) const override {
    *c = req_.data();
    *l = req_.size();
  }

  void Response(std::unique_ptr<Deserializer> d) override {
    deser_ = std::move(d);
  }

  std::unique_ptr<UnaryCallResultData> data_;
  std::unique_ptr<Deserializer> deser_;
  const String req_;
 protected:
  // TODO abandon() -> ctx->TryCancel();

  // Invoked by the ASIO Framework after we have markAsFinished(); this is
  // where we return data to PHP.
  void unserialize(TypedValue& result) override final {
    if (deser_) {
      SliceList slices;
      auto status = deser_->Slices(&slices);
      if (!status.Ok()) {
        if (!data_->status_.Ok()) { // take first failure.
          data_->status_ = status;
        }
      } else {
        size_t total = 0;
        for (auto s : slices) {
          total += s.second;
        }
        auto buf = StringData::Make(total);
        auto pos = buf->mutableData();
        for (auto s : slices) {
          std::copy_n(s.first, s.second, pos);
          pos += s.second;
        }

        buf->setSize(total);
        data_->resp_ = String(buf);
        deser_.reset(nullptr); // free the grpc::ByteBuffer now.
      }
    }
    auto res = GrpcUnaryCallResult::newInstance(std::move(data_));
    tvCopy(make_tv<KindOfObject>(res.detach()), result);
  }
};

Object HHVM_FUNCTION(grpc_unary_call,
    const String& target,
    const String& method,
    const String& req) {
  UnaryCallParams p;
  p.target_ = target.toCppString();
  p.method_ = method.toCppString();
  auto event = new GrpcEvent(req);
  event->data_->ctx_ = std::move(GrpcClientUnaryCall(p, event));
  return Object{event->getWaitHandle()};
}

struct GrpcExtension : Extension {
  GrpcExtension(): Extension("grpc", "0.0.1") {
    GrpcClientInit();
  }

  void moduleInit() override {
    HHVM_FE(grpc_unary_call);
    HHVM_ME(GrpcUnaryCallResult, StatusCode);
    HHVM_ME(GrpcUnaryCallResult, StatusMessage);
    HHVM_ME(GrpcUnaryCallResult, StatusDetails);
    HHVM_ME(GrpcUnaryCallResult, Response);
    Native::registerNativeDataInfo<GrpcUnaryCallResult>(
        GrpcUnaryCallResult::s_className.get());
    loadSystemlib();
  }
} s_grpc_extension;

HHVM_GET_MODULE(grpc);

} // namespace HPHP
