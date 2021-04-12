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

struct GrpcEvent final : AsioExternalThreadEvent, GrpcClientUnaryResultEvent {
  public:
  void done(int status_code) {
    data_ = std::make_unique<UnaryCallResultData>(); // todo constructor
    data_->status_code_ = status_code;
    markAsFinished();
  }
  protected:
  // Invoked by the ASIO Framework after we have markAsFinished(); this is
  // where we return data to PHP.
  void unserialize(TypedValue& result) final {
   std::cout << "wow.\n";
   auto res = GrpcUnaryCallResult::newInstance(std::move(data_));
   tvCopy(make_tv<KindOfObject>(res.detach()), result);
   std::cout << "bong bong.\n";
  }
  std::unique_ptr<UnaryCallResultData> data_;
};

Object HHVM_FUNCTION(grpc_unary_call, const String& data) {
  auto event = new GrpcEvent();
  GrpcClientUnaryCall(event);
  return Object{event->getWaitHandle()};
}

struct GrpcExtension : Extension {
  GrpcExtension(): Extension("grpc", "0.0.1") {}

  void moduleInit() override {
    HHVM_FE(grpc_unary_call);
    //HHVM_FALIAS(Grpc\\Extension\\grpc_unary_call, grpc_unary_call);
    HHVM_ME(GrpcUnaryCallResult, StatusCode);
    Native::registerNativeDataInfo<GrpcUnaryCallResult>(
        GrpcUnaryCallResult::s_className.get());
    loadSystemlib();
  }
} s_grpc_extension;

HHVM_GET_MODULE(grpc);

} // namespace HPHP
