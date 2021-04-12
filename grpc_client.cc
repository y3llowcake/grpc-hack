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

// TODO: for sreq, what happens if the request is terminated before we start
// copying the contents onto the wire? Do I need to copy early?
void GrpcClientUnaryCall(const std::string& sreq, GrpcClientUnaryResultEvent* ev) {
  ChannelStore::Init(); // TODO move.
  auto ch = ChannelStore::Get()->Channel();

  auto slice = new grpc::Slice(sreq); // This is a copy.
  //auto slice - 
  auto reqbb = std::make_shared<grpc::ByteBuffer>(slice, 1);
  auto ctx = new grpc::ClientContext();


  auto req = new grpc::ByteBuffer(NULL, 0); 
  auto meth = grpc::internal::RpcMethod("/helloworld.Greeter/SayHello", grpc::internal::RpcMethod::NORMAL_RPC);
  auto bb = new grpc::ByteBuffer();
  ::grpc::internal::CallbackUnaryCall<
    grpc::ByteBuffer,
    grpc::ByteBuffer,
    grpc::ByteBuffer,
    grpc::ByteBuffer>(ch, meth, ctx, req, bb, [ev, bb](grpc::Status s) {
    ev->status(s.error_code(), s.error_message(), s.error_details());
    ev->done();
  //std::vector<grpc::Slice> slices;
	//bb.Dump(&slices);
	//std::string str(reinterpret_cast<const char*>(slices[0].begin()), slices[0].size());
	//std::cout << str << "\n";  
  });
}
