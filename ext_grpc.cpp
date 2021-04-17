#include <iostream>

#include "hphp/runtime/base/array-init.h"
#include "hphp/runtime/base/array-iterator.h"
#include "hphp/runtime/base/builtin-functions.h"
#include "hphp/runtime/ext/asio/asio-external-thread-event.h"
#include "hphp/runtime/ext/extension.h"
#include "hphp/runtime/vm/native-data.h"

#include <grpc_client.h>

namespace HPHP {

#define NATIVE_DATA_CLASS(cls, hackcls, ptr, assign)                           \
  struct cls {                                                                 \
    static Class *s_class;                                                     \
    static const StaticString s_hackClassName;                                 \
    static const StaticString s_cppClassName;                                  \
    static Class *getClass() {                                                 \
      if (s_class == nullptr) {                                                \
        s_class = Class::lookup(s_hackClassName.get());                        \
        assertx(s_class);                                                      \
      }                                                                        \
      return s_class;                                                          \
    }                                                                          \
    static Object newInstance(ptr data) {                                      \
      Object obj{getClass()};                                                  \
      auto *d = Native::data<cls>(obj);                                        \
      d->data_ = assign;                                                       \
      return obj;                                                              \
    }                                                                          \
    cls(){};                                                                   \
    ptr data_;                                                                 \
    cls(const cls &) = delete;                                                 \
    cls &operator=(const cls &) = delete;                                      \
  };                                                                           \
  Class *cls::s_class = nullptr;                                               \
  const StaticString cls::s_hackClassName(#hackcls);                           \
  const StaticString cls::s_cppClassName(#cls);

#define NATIVE_GET_METHOD(rettype, cls, meth, expr)                            \
  static rettype HHVM_METHOD(cls, meth) {                                      \
    auto *d = Native::data<cls>(this_);                                        \
    return expr;                                                               \
  }

#define NATIVE_SET_METHOD(cls, meth, expr, ...)                                \
  static void HHVM_METHOD(cls, meth, __VA_ARGS__) {                            \
    auto *d = Native::data<cls>(this_);                                        \
    expr;                                                                      \
  }

//
// Status
//
NATIVE_DATA_CLASS(GrpcStatus, GrpcNative\\Status, Status, data);
NATIVE_GET_METHOD(int, GrpcStatus, Code, d->data_.code_);
NATIVE_GET_METHOD(String, GrpcStatus, Message, d->data_.message_);
NATIVE_GET_METHOD(String, GrpcStatus, Details, d->data_.details_);

// ClientContext
NATIVE_DATA_CLASS(GrpcClientContext, GrpcNative\\ClientContext,
                  std::shared_ptr<ClientContext>, std::move(data));
Object HHVM_STATIC_METHOD(GrpcClientContext, Create) {
  return GrpcClientContext::newInstance(std::move(ClientContext::New()));
}
NATIVE_GET_METHOD(String, GrpcClientContext, Peer, d->data_->Peer());
NATIVE_SET_METHOD(GrpcClientContext, SetTimeoutMicros,
                  d->data_->SetTimeoutMicros(p), int p);

// TODO maybe pass the whole dict in here to avoid crossing the boudnary so
// much?
NATIVE_SET_METHOD(GrpcClientContext, AddMetadata,
                  d->data_->AddMetadata(k.toCppString(), v.toCppString()),
                  const String &k, const String &v);

//
// UnaryCallResult
//
struct UnaryCallResultData {
  Status status_;
  String resp_;
  UnaryCallResultData() : resp_("") {}
};

NATIVE_DATA_CLASS(GrpcUnaryCallResult, GrpcNative\\UnaryCallResult,
                  std::unique_ptr<UnaryCallResultData>, std::move(data));
NATIVE_GET_METHOD(Object, GrpcUnaryCallResult, Status,
                  GrpcStatus::newInstance(d->data_->status_));
NATIVE_GET_METHOD(String, GrpcUnaryCallResult, Response, d->data_->resp_);

//
// ChannelArguments
//
NATIVE_DATA_CLASS(GrpcChannelArguments, GrpcNative\\ChannelArguments,
                  std::shared_ptr<ChannelArguments>, std::move(data));
NATIVE_SET_METHOD(GrpcChannelArguments, SetMaxReceiveMessageSize,
                  d->data_->SetMaxReceiveMessageSize(size), int size);
NATIVE_SET_METHOD(GrpcChannelArguments, SetMaxSendMessageSize,
                  d->data_->SetMaxSendMessageSize(size), int size);
NATIVE_SET_METHOD(GrpcChannelArguments, SetLoadBalancingPolicyName,
                  d->data_->SetLoadBalancingPolicyName(s.toCppString()),
                  const String &s);
Object HHVM_STATIC_METHOD(GrpcChannelArguments, Create) {
  return GrpcChannelArguments::newInstance(std::move(ChannelArguments::New()));
}

//
// Channel
//
NATIVE_DATA_CLASS(GrpcChannel, GrpcNative\\Channel, std::shared_ptr<Channel>,
                  std::move(data));
Object HHVM_STATIC_METHOD(GrpcChannel, Create, const String &name,
                          const String &target, const Object &args) {
  auto *dca = Native::data<GrpcChannelArguments>(args);
  return GrpcChannel::newInstance(std::move(
      GetChannel(name.toCppString(), target.toCppString(), dca->data_)));
}
NATIVE_GET_METHOD(String, GrpcChannel, Debug, d->data_->Debug());

struct GrpcEvent final : AsioExternalThreadEvent, GrpcClientUnaryResultEvent {
public:
  GrpcEvent(const String &req)
      : data_(std::make_unique<UnaryCallResultData>()), req_(req) {}

  void Done(Status s) override {
    data_->status_ = s;
    markAsFinished();
    // TODO release req_ here?
  }

  void FillRequest(const void **c, size_t *l) const override {
    *c = req_.data();
    *l = req_.size();
  }

  void Response(std::unique_ptr<Deserializer> d) override {
    deser_ = std::move(d);
  }

  std::unique_ptr<UnaryCallResultData> data_;
  std::unique_ptr<Deserializer> deser_;
  const String req_;

protected:
  // Invoked by the ASIO Framework after we have markAsFinished(); this is
  // where we return data to PHP.
  void unserialize(TypedValue &result) override final {
    if (deser_) {
      SliceList slices;
      auto status = deser_->ResponseSlices(&slices);
      if (!status.Ok()) {
        if (!data_->status_.Ok()) { // take first failure.
          data_->status_ = status;
        }
      } else {
        size_t total = 0;
        for (auto s : slices) {
          total += s.second;
        }
        auto buf = StringData::Make(total);
        auto pos = buf->mutableData();
        for (auto s : slices) {
          std::copy_n(s.first, s.second, pos);
          pos += s.second;
        }

        buf->setSize(total);
        data_->resp_ = String(buf);
        deser_.reset(nullptr); // free the grpc::ByteBuffer now.
      }
    }
    auto res = GrpcUnaryCallResult::newInstance(std::move(data_));
    tvCopy(make_tv<KindOfObject>(res.detach()), result);
  }
};

static Object HHVM_METHOD(GrpcChannel, UnaryCall, const Object &ctx,
                          const String &method, const String &req) {
  auto event = new GrpcEvent(req);
  auto *d = Native::data<GrpcChannel>(this_);
  auto *dctx = Native::data<GrpcClientContext>(ctx);
  d->data_->GrpcClientUnaryCall(method.toCppString(), dctx->data_, event);
  return Object{event->getWaitHandle()};
}

static void HHVM_METHOD(GrpcChannel, serverStreamingCallInternal,
                        const String &method, const String &req) {
  auto *d = Native::data<GrpcChannel>(this_);
  d->data_->ServerStreamingCall();
}

struct GrpcExtension : Extension {
  GrpcExtension() : Extension("grpc", "0.0.1") { GrpcClientInit(); }

  void moduleInit() override {
    // Status
    HHVM_MALIAS(GrpcNative\\Status, Code, GrpcStatus, Code);
    HHVM_MALIAS(GrpcNative\\Status, Message, GrpcStatus, Message);
    HHVM_MALIAS(GrpcNative\\Status, Details, GrpcStatus, Details);
    Native::registerNativeDataInfo<GrpcStatus>(
        GrpcStatus::s_cppClassName.get());

    // ClientContext
    HHVM_STATIC_MALIAS(GrpcNative\\ClientContext, Create, GrpcClientContext,
                       Create);
    HHVM_MALIAS(GrpcNative\\ClientContext, Peer, GrpcClientContext, Peer);
    HHVM_MALIAS(GrpcNative\\ClientContext, SetTimeoutMicros, GrpcClientContext,
                SetTimeoutMicros);
    HHVM_MALIAS(GrpcNative\\ClientContext, AddMetadata, GrpcClientContext,
                AddMetadata);
    Native::registerNativeDataInfo<GrpcClientContext>(
        GrpcClientContext::s_cppClassName.get());

    // ChannelArguments
    HHVM_STATIC_MALIAS(GrpcNative\\ChannelArguments, Create,
                       GrpcChannelArguments, Create);
    HHVM_MALIAS(GrpcNative\\ChannelArguments, SetMaxReceiveMessageSize,
                GrpcChannelArguments, SetMaxReceiveMessageSize);
    HHVM_MALIAS(GrpcNative\\ChannelArguments, SetMaxSendMessageSize,
                GrpcChannelArguments, SetMaxSendMessageSize);
    HHVM_MALIAS(GrpcNative\\ChannelArguments, SetLoadBalancingPolicyName,
                GrpcChannelArguments, SetLoadBalancingPolicyName);
    Native::registerNativeDataInfo<GrpcChannelArguments>(
        GrpcChannelArguments::s_cppClassName.get());

    // Channel
    HHVM_STATIC_MALIAS(GrpcNative\\Channel, Create, GrpcChannel, Create);
    HHVM_MALIAS(GrpcNative\\Channel, UnaryCall, GrpcChannel, UnaryCall);
    HHVM_MALIAS(GrpcNative\\Channel, serverStreamingCallInternal, GrpcChannel,
                serverStreamingCallInternal);
    HHVM_MALIAS(GrpcNative\\Channel, Debug, GrpcChannel, Debug);
    Native::registerNativeDataInfo<GrpcChannel>(
        GrpcChannel::s_cppClassName.get());

    // UnaryCallResult
    HHVM_MALIAS(GrpcNative\\UnaryCallResult, Status, GrpcUnaryCallResult,
                Status);
    HHVM_MALIAS(GrpcNative\\UnaryCallResult, Response, GrpcUnaryCallResult,
                Response);
    Native::registerNativeDataInfo<GrpcUnaryCallResult>(
        GrpcUnaryCallResult::s_cppClassName.get());

    loadSystemlib();
  }
} s_grpc_extension;

HHVM_GET_MODULE(grpc);

} // namespace HPHP
