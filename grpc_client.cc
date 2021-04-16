#include <grpc++/grpc++.h>

#include <grpcpp/impl/codegen/byte_buffer.h>
#include <grpcpp/impl/codegen/client_callback.h>

#include "src/core/lib/json/json.h"
#include "src/core/lib/surface/channel.h"

#include "grpc_client.h"

#include <unordered_map>


//
// Status
//

Status FromGrpcStatus(const grpc::Status s) {
  Status r;
  r.code_ = s.error_code();
  r.message_ = s.error_message();
  r.details_ = s.error_details();
  return r;
}

bool Status::Ok() const { return code_ == grpc::OK; }

//
// Context
//

struct ClientContextImpl: ClientContext {
  std::string Peer() override {
    return peer_;
  }
  void SetTimeoutMicros(int to) override {
    if (to > 0) {
      auto gprto =
          gpr_time_add(gpr_now(GPR_CLOCK_MONOTONIC),
                     gpr_time_from_micros(to, GPR_TIMESPAN));
      ctx_->set_deadline(gprto);
    }
  }
  void AddMetadata(const std::string& k, const std::string& v) override {
    ctx_->AddMetadata(k, v);
  }
  static inline ClientContextImpl* from(ClientContext* c) {
    return static_cast<ClientContextImpl*>(c);
  }
  std::unique_ptr<grpc::ClientContext> ctx_;
  std::string peer_;
};

std::shared_ptr<ClientContext> ClientContext::New() {
  auto impl = new ClientContextImpl;
  impl->ctx_.reset(new grpc::ClientContext());
  return std::shared_ptr<ClientContext>(impl);
}


struct ChannelImpl : Channel {
  std::shared_ptr<grpc::Channel> channel_;
  grpc_channel *core_channel_; // not owned

  void
  GrpcClientUnaryCall(const std::string &method,
      std::shared_ptr<ClientContext> ctx,
                      GrpcClientUnaryResultEvent *ev) override;

  void ServerStreamingCall() override;

  grpc_core::Json DebugJson() {
    return grpc_channel_get_channelz_node(core_channel_)->RenderJson();
  }

  std::string Debug() override { return DebugJson().Dump(2); }
};

class ChannelStore {
public:
  static ChannelStore *Singleton;

  std::shared_ptr<ChannelImpl> GetChannel(const std::string &name,
                                          const std::string &target,
                                          const ChannelCreateParams &p) {
    std::lock_guard<std::mutex> guard(mu_);
    auto it = map_.find(name);
    if (it != map_.end()) {
      return it->second;
    }
    // auto cch = grpc::InsecureChannelCredentials()->CreateChannelImpl(target,
    // args);
    // We do a more complicated form of the above so we can get access to the
    // core channel pointer, which in turn can be used with channelz apis.
    grpc::ChannelArguments args;
    if (p.max_send_message_size_ > 0) {
      args.SetMaxSendMessageSize(p.max_send_message_size_);
    }
    if (p.max_receive_message_size_ > 0) {
      args.SetMaxSendMessageSize(p.max_receive_message_size_);
    }
    if (!p.lb_policy_name_.empty()) {
      args.SetLoadBalancingPolicyName(p.lb_policy_name_);
    }

    grpc_channel_args channel_args;
    args.SetChannelArgs(&channel_args);

    auto record = std::make_shared<ChannelImpl>();
    record->core_channel_ =
        grpc_insecure_channel_create(target.c_str(), &channel_args, nullptr);
    std::vector<
        std::unique_ptr<grpc::experimental::ClientInterceptorFactoryInterface>>
        interceptor_creators;
    record->channel_ = ::grpc::CreateChannelInternal(
        "", record->core_channel_, std::move(interceptor_creators));
    map_[name] = record;
    return record;
  }

  /*
    std::string Debug() override {
      std::lock_guard<std::mutex> guard(mu_);
      grpc_core::Json::Object o;
      for(auto kv : map_) {
        o[kv.first] = kv.second->DebugJson();
      }
      return grpc_core::Json(o).Dump(2);
    }
        */
private:
  std::mutex mu_;
  std::unordered_map<std::string, std::shared_ptr<ChannelImpl>> map_;
};

ChannelStore *ChannelStore::Singleton = nullptr;

void GrpcClientInit() {
  grpc_init();
  // gpr_set_log_verbosity(GPR_LOG_SEVERITY_DEBUG);
  ChannelStore::Singleton = new ChannelStore();
}

std::shared_ptr<Channel> GetChannel(const std::string &name,
                                    const std::string &target,
                                    const ChannelCreateParams &p) {
  return ChannelStore::Singleton->GetChannel(name, target, p);
}

/*std::string GrpcClientDebug() {
  return ChannelStore::Singleton->Debug();
}*/

void
ChannelImpl::GrpcClientUnaryCall(const std::string& method,
  std::shared_ptr<ClientContext> ctx,
                                 GrpcClientUnaryResultEvent *ev) {
/*  std::shared_ptr<ClientContextImpl> ctx(new ClientContextImpl());
  for (auto kv : p.md_) {
    for (auto v : kv.second) {
      ctx->ctx_.AddMetadata(kv.first, v);
    }
  }*/
  
  auto meth = grpc::internal::RpcMethod(method.c_str(),
                                        grpc::internal::RpcMethod::NORMAL_RPC);
  ::grpc::internal::CallbackUnaryCall<
      GrpcClientUnaryResultEvent, GrpcClientUnaryResultEvent,
      GrpcClientUnaryResultEvent, GrpcClientUnaryResultEvent>(
      channel_.get(), meth, ClientContextImpl::from(ctx.get())->ctx_.get(), ev, ev, [ctx, ev](grpc::Status s) {
      auto ctxi = ClientContextImpl::from(ctx.get());
      ctxi->peer_ = ctxi->ctx_->peer();
        ev->Done(FromGrpcStatus(s));
      });
  //return std::move(ctx);
}



void ChannelImpl::ServerStreamingCall() {
  auto meth = grpc::internal::RpcMethod("/helloworld.HelloWorldService/SayHelloStream",
                                        grpc::internal::RpcMethod::SERVER_STREAMING);
  //std::shared_ptr<ClientContextImpl> ctx(new ClientContextImpl());
  grpc::ByteBuffer req;
  grpc::ByteBuffer resp;
  //reactor ClientReadReactor<groc::ByteBuffer>;
  // ClientCallbackReaderFactory<grpc::ByteBuffer>::Create(channel_.get(), meth, &ctx->ctx_, &req, reactor);
  // ClientCallbackReader
  // ClientCallbackReaderImpl 
  // ClientCallbackReaderFactory
}

struct DeserializerImpl : Deserializer {
  Status ResponseSlices(SliceList *list) override {
    std::vector<grpc::Slice> slices;
    auto status = bb_.Dump(&slices);
    if (!status.ok()) {
      return FromGrpcStatus(status);
    }
    for (auto s : slices) {
      list->push_back(
          std::make_pair<const uint8_t *, size_t>(s.begin(), s.size()));
    }
    return FromGrpcStatus(grpc::Status::OK);
  }
  grpc::ByteBuffer bb_;
};

// https://grpc.github.io/grpc/cpp/classgrpc_1_1_serialization_traits.html
template <> class grpc::SerializationTraits<GrpcClientUnaryResultEvent, void> {
public:
  static grpc::Status Deserialize(ByteBuffer *byte_buffer,
                                  GrpcClientUnaryResultEvent *dest) {
    auto d = new DeserializerImpl();
    d->bb_.Swap(byte_buffer);
    std::unique_ptr<Deserializer> dp(d);
    dest->Response(std::move(dp));
    return Status::OK;
  }
  static grpc::Status Serialize(const GrpcClientUnaryResultEvent &source,
                                ByteBuffer *buffer, bool *own_buffer) {
    const void *c;
    size_t l;
    source.FillRequest(&c, &l);
    *own_buffer = true;
    grpc::Slice slice(c, l, grpc::Slice::STATIC_SLICE);
    ByteBuffer tmp(&slice, 1);
    buffer->Swap(&tmp);
    return Status::OK;
  }
};
