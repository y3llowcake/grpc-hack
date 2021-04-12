#include <iostream>

#include "hphp/runtime/ext/extension.h"
#include "hphp/runtime/base/builtin-functions.h"
#include "hphp/runtime/base/array-init.h"
#include "hphp/runtime/ext/asio/asio-external-thread-event.h"
#include "hphp/runtime/vm/native-data.h"

#include <grpc_client.h>

namespace HPHP {


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
	static Object newInstance() {
		Object obj{getClass()};
		//auto* data = Native::data<AsyncMysqlClientStats>(obj);
		//data->setPerfValues(std::move(values));
		return obj;
	}

  GrpcUnaryCallResult(const GrpcUnaryCallResult&) = delete;
  GrpcUnaryCallResult& operator=(const GrpcUnaryCallResult&) = delete;
};

static int HHVM_METHOD(GrpcUnaryCallResult, StatusCode) {
  //auto* data = Native::data<GrpcUnaryCallResult>(this_);
	//return data->m_code;
  return 1337;
}

Class* GrpcUnaryCallResult::s_class = nullptr;
const StaticString GrpcUnaryCallResult::s_className("GrpcUnaryCallResult");

//IMPLEMENT_GET_CLASS(GrpcUnaryCallResult)

/*const StaticString
  s_code("code"),
  s_status("status");*/

struct GrpcEvent final : AsioExternalThreadEvent {
  public:
  void done() {
    markAsFinished();
  }
  protected:
  // Invoked by the ASIO Framework after we have markAsFinished(); this is
  // where we return data to PHP.
  void unserialize(TypedValue& result) final {
   std::cout << "wow.\n";
/*   auto ret = make_darray(
      s_status, make_darray(
        s_code, -1
      )
    );
    tvCopy(make_tv<KindOfString>(StringData::Make("foo")), result);
    //tvCopy(make_tv<KindOfDArray>(ret), result);
    //tvCopy(make_array_like_tv(ret.get()), result); // TODO: I have no idea if this is correct from a ref/unref perspective.*/
   auto res = GrpcUnaryCallResult::newInstance();
   tvCopy(make_tv<KindOfObject>(res.detach()), result);
   std::cout << "bong bong.\n";

  }
};

Object HHVM_FUNCTION(grpc_unary_call, const String& data) {
  mainz(0, NULL);
  auto event = new GrpcEvent();
  event->done();
  return Object{event->getWaitHandle()};
	// return ret;
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
