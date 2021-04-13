#include <iostream>

#include "hphp/runtime/ext/extension.h"
#include "hphp/runtime/base/builtin-functions.h"
#include "hphp/runtime/base/array-init.h"
#include "hphp/runtime/ext/asio/asio-external-thread-event.h"
#include "hphp/runtime/vm/native-data.h"

#include <grpc_client.h>

namespace HPHP {

struct UnaryCallResultData {
  int status_code_;
  std::string status_message_;
  std::string status_details_;
  String resp_;
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
	return d->data_->status_code_;
}

static String HHVM_METHOD(GrpcUnaryCallResult, StatusMessage) {
  auto* d = Native::data<GrpcUnaryCallResult>(this_);
	return d->data_->status_message_;
}

static String HHVM_METHOD(GrpcUnaryCallResult, StatusDetails) {
  auto* d = Native::data<GrpcUnaryCallResult>(this_);
	return d->data_->status_details_;
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

  void Done() override {
    markAsFinished();
  }

  void FillRequest(const void** c, size_t* l) const override {
    *c = req_.data();
    *l = req_.size();
  }

  void Response(std::unique_ptr<Deserializer> d) override {
    deser_ = std::move(d);
  }
/*  void ResponseAlloc(size_t l) override {

    std::cout << "wutang " << l << "\n";
    //data_->resp_.reset(StringData::Make(l));
    data_->resp_.reserve(l);
    //data_->resp_ = StringData::Make(l);
    std::cout << "forever\n";
  }

  void ResponseAppend(const char* c, size_t l) override {

    std::cout <<"never say\n";
    data_->resp_.get()->append(folly::StringPiece(c, l));
    //data_->resp_->append(folly::StringPiece(c, l));
    std::cout <<"never\n";
  }*/

  void Status(int status_code, const std::string& status_message, const std::string& status_details) override {
    data_->status_code_ = status_code;
    data_->status_message_ = status_message;
    data_->status_details_ = status_details;
  }

 protected:
  // TODO abandon() -> ctx->TryCancel();

  // Invoked by the ASIO Framework after we have markAsFinished(); this is
  // where we return data to PHP.
  void unserialize(TypedValue& result) override final {
    auto deser = deser_.get();
    if (deser) {
      SliceList slices;
      deser->Slices(&slices);
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
    }
    auto res = GrpcUnaryCallResult::newInstance(std::move(data_));
    tvCopy(make_tv<KindOfObject>(res.detach()), result);
  }
  std::unique_ptr<UnaryCallResultData> data_;
  std::unique_ptr<Deserializer> deser_;
  const String req_;
};

Object HHVM_FUNCTION(grpc_unary_call, const String& req) {
  auto event = new GrpcEvent(req);
  GrpcClientUnaryCall(event);
  return Object{event->getWaitHandle()};
}

struct GrpcExtension : Extension {
  GrpcExtension(): Extension("grpc", "0.0.1") {}

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
