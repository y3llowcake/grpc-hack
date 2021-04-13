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
  auto ctx = new grpc::ClientContext();

  auto meth = grpc::internal::RpcMethod("/helloworld.Greeter/SayHello", grpc::internal::RpcMethod::NORMAL_RPC);
  ::grpc::internal::CallbackUnaryCall<
    GrpcClientUnaryResultEvent,
    GrpcClientUnaryResultEvent,
    GrpcClientUnaryResultEvent,
    GrpcClientUnaryResultEvent>(ch, meth, ctx, ev, ev, [ev](grpc::Status s) {
      ev->Status(s.error_code(), s.error_message(), s.error_details());
      ev->Done();
  });
}

struct DeserializerImpl : Deserializer {
  void Slices(SliceList* list) override {
    std::vector<grpc::Slice> slices;
    auto status = bb_.Dump(&slices);
    if (!status.ok()) {
      //
      // TODO YOU MUST FIX THIS
      //
      return;
    }
    for (auto s : slices) {
      list->push_back(std::make_pair<const uint8_t*, size_t>(s.begin(), s.size()));
    } 
  }
  grpc::ByteBuffer bb_;
};

// https://grpc.github.io/grpc/cpp/classgrpc_1_1_serialization_traits.html
template <>
class grpc::SerializationTraits<GrpcClientUnaryResultEvent, void> {
public:
	static grpc::Status Deserialize(ByteBuffer* byte_buffer,
      GrpcClientUnaryResultEvent* dest) {
    auto d = new DeserializerImpl();
    d->bb_.Swap(byte_buffer);
    std::unique_ptr<Deserializer> dp(d);
    dest->Response(std::move(dp));
		return Status::OK;
	}
	static grpc::Status Serialize(const GrpcClientUnaryResultEvent& source,
		ByteBuffer* buffer, bool* own_buffer) {
    const void* c;
    size_t l;
    source.FillRequest(&c, &l);
    *own_buffer = true;
    grpc::Slice slice(c, l, grpc::Slice::STATIC_SLICE);
    ByteBuffer tmp(&slice, 1);
    buffer->Swap(&tmp);
		return Status::OK;
	}
};
