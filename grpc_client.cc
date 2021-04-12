#include <iostream>
#include <string>

#include <grpc++/grpc++.h>

#include <grpcpp/impl/codegen/client_callback.h>
#include <grpcpp/impl/codegen/byte_buffer.h>

#include "grpc_client.h"

class ChannelStore {
	public:
    static void Init() {
      singleton_ = new ChannelStore();
    }
    static ChannelStore* Get() {
      return singleton_;
		}
    grpc::Channel* Channel() {
      auto ch = grpc::CreateChannel("localhost:50051", grpc::InsecureChannelCredentials());
      ptr_.swap(ch);
      return ptr_.get();
    }
	private:
    static ChannelStore* singleton_;
    std::shared_ptr<grpc::Channel> ptr_;
};

ChannelStore* ChannelStore::singleton_= nullptr;

void GrpcClientUnaryCall(GrpcClientUnaryResultEvent* ev) {
  ChannelStore::Init();
  std::cout << "huzzah\n";
  auto ctx = new grpc::ClientContext();
  auto ch = ChannelStore::Get()->Channel();

  auto req = new grpc::ByteBuffer(NULL, 0); 
  auto meth = grpc::internal::RpcMethod("/helloworld.Greeter/SayHello", grpc::internal::RpcMethod::NORMAL_RPC);
  auto bb = new grpc::ByteBuffer();
  ::grpc::internal::CallbackUnaryCall<
    grpc::ByteBuffer,
    grpc::ByteBuffer,
    grpc::ByteBuffer,
    grpc::ByteBuffer>(ch, meth, ctx, req, bb, [ev, bb](grpc::Status s) {
    std::cout << "done"
    << " " << s.error_code() 
    <<" " << s.error_message() 
    << "\n";
    ev->done(s.error_code());
  //std::vector<grpc::Slice> slices;
	//bb.Dump(&slices);
	//std::string str(reinterpret_cast<const char*>(slices[0].begin()), slices[0].size());
	//std::cout << str << "\n";
    
//exit(0);
  });
  std::cout << "woo\n";
  //std::cout << "done\n";
}
