#include <grpc++/grpc++.h>

#include <grpcpp/impl/codegen/client_callback.h>
#include <grpcpp/impl/codegen/byte_buffer.h>

#include "src/core/lib/surface/channel.h"
#include "src/core/lib/json/json.h"

#include "grpc_client.h"

#include <unordered_map>

struct ChannelRecord {
  std::shared_ptr<grpc::Channel> channel_;
  grpc_channel* core_channel_;
};

class ChannelStore {
	public:
    static ChannelStore* Singleton;
    std::shared_ptr<grpc::Channel> Channel(
        const std::string& name,
        const std::string& target) {
      std::lock_guard<std::mutex> guard(mu_);
      auto it = map_.find(name);
      if (it != map_.end()) {
        return it->second->channel_;
      }
      //auto cch = grpc::InsecureChannelCredentials()->CreateChannelImpl(target, args);
      // We do a more complicated form of the above so we can get access to the
      // core channel pointer, which in turn can be used with channelz apis.
      grpc::ChannelArguments args;
      grpc_channel_args channel_args;
      args.SetChannelArgs(&channel_args);

      auto record = std::make_shared<ChannelRecord>();
      record->core_channel_ = grpc_insecure_channel_create(target.c_str(), &channel_args, nullptr);
      std::vector<std::unique_ptr<grpc::experimental::ClientInterceptorFactoryInterface>> interceptor_creators;
      record->channel_ = ::grpc::CreateChannelInternal("", record->core_channel_, std::move(interceptor_creators));
      map_[name] = record;
      return record->channel_;
    }
  std::string Debug() {
    std::lock_guard<std::mutex> guard(mu_);
    grpc_core::Json::Object o;
    for(auto kv : map_) {
      auto node = grpc_channel_get_channelz_node(kv.second->core_channel_);
      o[kv.first] = node->RenderJson();
    }
    return grpc_core::Json(o).Dump(2);
  }
    // grpc_channel_get_channelz_node -> RendorJSON;
	private:
    std::mutex mu_;
    std::unordered_map<std::string, std::shared_ptr<ChannelRecord>> map_;
};

ChannelStore* ChannelStore::Singleton = nullptr;

void GrpcClientInit() {
  grpc_init();
  ChannelStore::Singleton = new ChannelStore();
}

std::string GrpcClientDebug() {
  return ChannelStore::Singleton->Debug();
}

Status FromGrpcStatus(const grpc::Status s) {
  Status r;
  r.code_ = s.error_code();
  r.message_ = s.error_message();
  r.details_ = s.error_details();
  return r;
}

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
