#include "hphp/runtime/ext/extension.h"
#include "hphp/runtime/base/builtin-functions.h"

#include <grpc_client.h>

namespace HPHP {

String HHVM_FUNCTION(grpc, const String& data) {
  mainz(0, NULL);
  return String("foobar");
	// return ret;
}

struct GrpcExtension : Extension {
  GrpcExtension(): Extension("grpc", "0.0.1") {}

  void moduleInit() override {
    HHVM_FE(grpc);
    loadSystemlib();
  }
} s_grpc_extension;

HHVM_GET_MODULE(grpc);

} // namespace HPHP
