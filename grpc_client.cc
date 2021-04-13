#include <grpc++/grpc++.h>

#include <grpcpp/impl/codegen/client_callback.h>
#include <grpcpp/impl/codegen/byte_buffer.h>

#include "grpc_client.h"

#include <unordered_map>

class ChannelStore {
	public:
    static ChannelStore* Singleton;
    std::shared_ptr<grpc::Channel> Channel(
        const std::string& name,
        const std::string& target) {
      std::lock_guard<std::mutex> guard(mu_);
      auto ch = map_[name];
      if (ch) {
        return ch;
      }
      ch = grpc::CreateChannel("localhost:50051", grpc::InsecureChannelCredentials());
      map_[name] = ch;
      return ch;
    }
	private:
    std::mutex mu_;
    std::unordered_map<std::string, std::shared_ptr<grpc::Channel>> map_;
};

ChannelStore* ChannelStore::Singleton = nullptr;

void GrpcClientInit() {
      ChannelStore::Singleton = new ChannelStore();
}

Status FromGrpcStatus(const grpc::Status s) {
  Status r;
  r.code_ = s.error_code();
  r.message_ = s.error_message();
  r.details_ = s.error_details();
  return r;
}

// static const Status OK = FromGrpcStatus(grpc::Status::OK);

bool Status::Ok() const {
  return code_ == grpc::OK;
}

struct ClientContextImpl : ClientContext {
  grpc::ClientContext ctx_;
};

// TODO: for sreq, what happens if the request is terminated before we start
// copying the contents onto the wire? Do I need to copy early?
std::unique_ptr<ClientContext> GrpcClientUnaryCall(const UnaryCallParams& p, GrpcClientUnaryResultEvent* ev) {
  auto ch = ChannelStore::Singleton->Channel(p.target_, p.target_);
  std::unique_ptr<ClientContextImpl> ctx(new ClientContextImpl());
  if (p.timeout_micros_ > 0) {
    auto to = gpr_time_add(
        gpr_now(GPR_CLOCK_MONOTONIC),
        gpr_time_from_micros(p.timeout_micros_, GPR_TIMESPAN));
    ctx->ctx_.set_deadline(to);
  }
  for (auto kv : p.md_) {
    for (auto v : kv.second) {
      ctx->ctx_.AddMetadata(kv.first, v);
    }
  }
  //auto meth = grpc::internal::RpcMethod("/helloworld.Greeter/SayHello", grpc::internal::RpcMethod::NORMAL_RPC);
  auto meth = grpc::internal::RpcMethod(p.method_.c_str(), grpc::internal::RpcMethod::NORMAL_RPC);
  ::grpc::internal::CallbackUnaryCall<
    GrpcClientUnaryResultEvent,
    GrpcClientUnaryResultEvent,
    GrpcClientUnaryResultEvent,
    GrpcClientUnaryResultEvent>(ch.get(), meth, &ctx->ctx_, ev, ev, [ev](grpc::Status s) {
      ev->Done(FromGrpcStatus(s));
  });
  return std::move(ctx);
}

struct DeserializerImpl : Deserializer {
  Status Slices(SliceList* list) override {
    std::vector<grpc::Slice> slices;
    auto status = bb_.Dump(&slices);
    if (!status.ok()) {
      return FromGrpcStatus(status);
    }
    for (auto s : slices) {
      list->push_back(std::make_pair<const uint8_t*, size_t>(s.begin(), s.size()));
    } 
    return FromGrpcStatus(grpc::Status::OK);
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
