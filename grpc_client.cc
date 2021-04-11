#include <iostream>
#include <string>

#include <grpc++/grpc++.h>

#include <grpcpp/impl/codegen/client_callback.h>
#include <grpcpp/impl/codegen/byte_buffer.h>

#include "grpc_client.h"

int mainz(int argc, char** argv) {
  std::cout << "huzzah\n";
  ::grpc::ClientContext ctx;
  auto ch = grpc::CreateChannel("localhost:50051", grpc::InsecureChannelCredentials());
  grpc::ByteBuffer req(NULL, 0); 
  auto meth = grpc::internal::RpcMethod("/helloworld.Greeter/SayHello", grpc::internal::RpcMethod::NORMAL_RPC);
  auto bb = grpc::ByteBuffer();
  ::grpc::internal::CallbackUnaryCall<
    grpc::ByteBuffer,
    grpc::ByteBuffer,
    grpc::ByteBuffer,
    grpc::ByteBuffer>(ch.get(), meth, &ctx, &req, &bb, [&bb](grpc::Status s) {
    std::cout << "done"
    << " " << s.error_code() 
    <<" " << s.error_message() 
    << "\n";
  std::vector<grpc::Slice> slices;
	bb.Dump(&slices);
	std::string str(reinterpret_cast<const char*>(slices[0].begin()), slices[0].size());
	std::cout << str << "\n";
    
exit(0);
  });
  std::cout << "woo\n";
  while (true) {}
  std::cout << "done\n";
  return 0;
}
