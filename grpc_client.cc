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

//
// ChannelArguments
//

struct ChannelArgumentsImpl : ChannelArguments {
  void SetMaxReceiveMessageSize(int size) override { args_->SetMaxReceiveMessageSize(size); }
  void SetMaxSendMessageSize(int size) override { args_->SetMaxSendMessageSize(size); }
  void SetLoadBalancingPolicyName(const std::string& lb) override { args_->SetLoadBalancingPolicyName(lb); }
  
  static inline ChannelArgumentsImpl* from(ChannelArguments* c) {
    return static_cast<ChannelArgumentsImpl*>(c);
  }

  std::unique_ptr<grpc::ChannelArguments> args_;
};

std::shared_ptr<ChannelArguments> ChannelArguments::New() {
  auto r = new ChannelArgumentsImpl();
  r->args_.reset(new grpc::ChannelArguments);
  return std::shared_ptr<ChannelArguments>(r);
}

//
// Channel
//
struct ChannelImpl : Channel {
  std::shared_ptr<grpc::Channel> channel_;
  grpc_channel *core_channel_; // not owned

  void
  GrpcClientUnaryCall(const std::string &method,
      std::shared_ptr<ClientContext> ctx,
                      GrpcClientUnaryResultEvent *ev) override;

  void ServerStreamingCall(const std::string &method, std::shared_ptr<ClientContext> ctx) override;

  grpc_core::Json DebugJson() {
    return grpc_channel_get_channelz_node(core_channel_)->RenderJson();
  }

  std::string Debug() override { return DebugJson().Dump(2); }
};

void
ChannelImpl::GrpcClientUnaryCall(const std::string& method,
  std::shared_ptr<ClientContext> ctx,
                                 GrpcClientUnaryResultEvent *ev) {
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
}

template <class T>
class ClientReadReactor : public ::grpc::experimental::ClientReadReactor<T> {
  public:
  void OnDone(const ::grpc::Status& s) override {
    printf("wutang done!\n");
  }
  void OnReadInitialMetadataDone(bool ok) override {
    printf("wutang initial md!\n");
  }
  virtual void OnReadDone(bool ok) override {
    printf("wutang on read done\n");
  }
};

void ChannelImpl::ServerStreamingCall(const std::string& method, std::shared_ptr<ClientContext> ctx) {
  auto meth = grpc::internal::RpcMethod("/helloworld.HelloWorldService/SayHelloStream",
  //auto meth = grpc::internal::RpcMethod(method.c_str(),
                                        grpc::internal::RpcMethod::SERVER_STREAMING);
  auto req = new grpc::ByteBuffer(NULL, 0);
  auto resp = new grpc::ByteBuffer();
  printf("wutang\n");
  auto reactor = new ClientReadReactor<grpc::ByteBuffer>();
  ::grpc::internal::ClientCallbackReaderFactory<grpc::ByteBuffer>::Create(
    channel_.get(),
    meth, 
    ClientContextImpl::from(ctx.get())->ctx_.get(),
    req, 
    reactor
    );

 //reactor->AddHold();
 reactor->StartRead(resp);
 printf("startingcall\n");
 //reactor->AddHold();
 reactor->StartCall();
  printf("forever\n");

  

  //reactor ClientReadReactor<groc::ByteBuffer>;
  // ClientCallbackReaderFactory<grpc::ByteBuffer>::Create(channel_.get(), meth, &ctx->ctx_, &req, reactor);
  // ClientCallbackReader
  // ClientCallbackReaderImpl 
  // ClientCallbackReaderFactory
}


//
// ChannelStore
//

class ChannelStore {
public:
  static ChannelStore *Singleton;

  std::shared_ptr<ChannelImpl> GetChannel(const std::string &name,
                                          const std::string &target,
                                          std::shared_ptr<ChannelArguments> args) {
    std::lock_guard<std::mutex> guard(mu_);
    auto it = map_.find(name);
    if (it != map_.end()) {
      return it->second;
    }
    // auto cch = grpc::InsecureChannelCredentials()->CreateChannelImpl(target,
    // args);
    // We do a more complicated form of the above so we can get access to the
    // core channel pointer, which in turn can be used with channelz apis.
    grpc_channel_args channel_args;
    ChannelArgumentsImpl::from(args.get())->args_->SetChannelArgs(&channel_args);

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

private:
  std::mutex mu_;
  std::unordered_map<std::string, std::shared_ptr<ChannelImpl>> map_;
};

ChannelStore *ChannelStore::Singleton = nullptr;

std::shared_ptr<Channel> GetChannel(const std::string &name,
                                    const std::string &target,
                                    std::shared_ptr<ChannelArguments> args) {
  return ChannelStore::Singleton->GetChannel(name, target, std::move(args));
}

//
// SerDe
//

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

void GrpcClientInit() {
  grpc_init();
  //gpr_set_log_verbosity(GPR_LOG_SEVERITY_DEBUG);
  ChannelStore::Singleton = new ChannelStore();
}
