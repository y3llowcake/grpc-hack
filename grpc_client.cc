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
void GrpcClientUnaryCall(GrpcClientUnaryResultEvent* ev) {
  ChannelStore::Init(); // TODO move.
  auto ch = ChannelStore::Get()->Channel();

  // auto slice = new grpc::Slice(sreq); // This is a copy.
  //auto slice = new grpc::Slice(sreq, Slice::StaticSlice)
  //auto reqbb = std::make_shared<grpc::ByteBuffer>(slice, 1);
  auto ctx = new grpc::ClientContext();


  // auto req = new grpc::ByteBuffer(NULL, 0); 
  auto meth = grpc::internal::RpcMethod("/helloworld.Greeter/SayHello", grpc::internal::RpcMethod::NORMAL_RPC);
  auto bb = new grpc::ByteBuffer();
  ::grpc::internal::CallbackUnaryCall<
    GrpcClientUnaryResultEvent,
    grpc::ByteBuffer,
    GrpcClientUnaryResultEvent,
    grpc::ByteBuffer>(ch, meth, ctx, ev, bb, [ev, bb](grpc::Status s) {
    ev->status(s.error_code(), s.error_message(), s.error_details());
    ev->done();
  //std::vector<grpc::Slice> slices;
	//bb.Dump(&slices);
	//std::string str(reinterpret_cast<const char*>(slices[0].begin()), slices[0].size());
	//std::cout << str << "\n";  
  });
}

// https://grpc.github.io/grpc/cpp/classgrpc_1_1_serialization_traits.html
template <>
class grpc::SerializationTraits<GrpcClientUnaryResultEvent, void> {
public:
	static grpc::Status Deserialize(ByteBuffer* byte_buffer,
      GrpcClientUnaryResultEvent* dest) {
		// dest->set_buffer(byte_buffer->buffer_);
		return Status::OK;
	}
	static grpc::Status Serialize(const GrpcClientUnaryResultEvent& source,
		ByteBuffer* buffer, bool* own_buffer) {
    const void* c;
    size_t l;
    source.fillRequest(&c, &l);
    *own_buffer = true;
    grpc::Slice slice(c, l, grpc::Slice::STATIC_SLICE);
    ByteBuffer tmp(&slice, 1);
    buffer->Swap(&tmp);
		return Status::OK;
	}
};
