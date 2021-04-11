#include <iostream>

#include "hphp/runtime/ext/extension.h"
#include "hphp/runtime/base/builtin-functions.h"
#include "hphp/runtime/base/array-init.h"

#include <grpc_client.h>

namespace HPHP {

const StaticString
  s_code("code"),
  s_status("status");

Array HHVM_FUNCTION(grpc_unary_call, const String& data) {
  mainz(0, NULL);
  return make_darray(
    s_status, make_darray(
      s_code, -1
    )
  );
	// return ret;
}

struct GrpcExtension : Extension {
  GrpcExtension(): Extension("grpc", "0.0.1") {}

  void moduleInit() override {
    // HHVM_FE(grpc_unary_call);
    HHVM_FALIAS(Grpc\\Extension\\grpc_unary_call, grpc_unary_call);
    loadSystemlib();
  }
} s_grpc_extension;

HHVM_GET_MODULE(grpc);

} // namespace HPHP
