#include <iostream>

#include "hphp/runtime/base/array-init.h"
#include "hphp/runtime/base/array-iterator.h"
#include "hphp/runtime/base/builtin-functions.h"
#include "hphp/runtime/ext/asio/asio-external-thread-event.h"
#include "hphp/runtime/ext/extension.h"
#include "hphp/runtime/vm/native-data.h"

#include <grpc_client.h>

namespace HPHP {

#define NATIVE_DATA_CLASS(cls, ptr)                                            \
  struct cls {                                                                 \
    static Class *s_class;                                                     \
    static const StaticString s_className;                                     \
    static Class *getClass() {                                                 \
      if (s_class == nullptr) {                                                \
        s_class = Class::lookup(s_className.get());                            \
        assertx(s_class);                                                      \
      }                                                                        \
      return s_class;                                                          \
    }                                                                          \
    static Object newInstance(ptr data) {                                      \
      Object obj{getClass()};                                                  \
      auto *d = Native::data<cls>(obj);                                        \
      d->data_ = std::move(data);                                              \
      return obj;                                                              \
    }                                                                          \
    cls(){};                                                                   \
    ptr data_;                                                                 \
    cls(const cls &) = delete;                                                 \
    cls &operator=(const cls &) = delete;                                      \
  };                                                                           \
  Class *cls::s_class = nullptr;                                               \
  const StaticString cls::s_className(#cls);

struct UnaryCallResultData {
  Status status_;
  String resp_;
  String peer_;
  std::shared_ptr<ClientContext> ctx_;
  UnaryCallResultData() : resp_("") {}
};

NATIVE_DATA_CLASS(GrpcUnaryCallResult, std::unique_ptr<UnaryCallResultData>);

static int HHVM_METHOD(GrpcUnaryCallResult, StatusCode) {
  auto *d = Native::data<GrpcUnaryCallResult>(this_);
  return d->data_->status_.code_;
}

static String HHVM_METHOD(GrpcUnaryCallResult, StatusMessage) {
  auto *d = Native::data<GrpcUnaryCallResult>(this_);
  return d->data_->status_.message_;
}

static String HHVM_METHOD(GrpcUnaryCallResult, StatusDetails) {
  auto *d = Native::data<GrpcUnaryCallResult>(this_);
  return d->data_->status_.details_;
}

static String HHVM_METHOD(GrpcUnaryCallResult, Response) {
  auto *d = Native::data<GrpcUnaryCallResult>(this_);
  return String(d->data_->resp_);
}

static String HHVM_METHOD(GrpcUnaryCallResult, Peer) {
  auto *d = Native::data<GrpcUnaryCallResult>(this_);
  return String(d->data_->ctx_->Peer());
}

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

struct ChannelData {
  std::shared_ptr<Channel> channel_;
};

const auto optMaxSend = HPHP::StaticString("max_send_message_size");
const auto optMaxReceive = HPHP::StaticString("max_receive_message_size");
const auto optLbPolicy = HPHP::StaticString("lb_policy_name");
static void HHVM_METHOD(GrpcChannel, __construct, const String &name,
                        const String &target, const Array &opt) {
  ChannelCreateParams p;
  if (opt.exists(optMaxSend)) {
    p.max_send_message_size_ = opt[optMaxSend].toInt64();
  }
  if (opt.exists(optMaxReceive)) {
    p.max_receive_message_size_ = opt[optMaxReceive].toInt64();
  }
  if (opt.exists(optLbPolicy)) {
    p.lb_policy_name_ = opt[optLbPolicy].toString().toCppString();
  }

  auto *d = Native::data<ChannelData>(this_);
  d->channel_ = GetChannel(name.toCppString(), target.toCppString(), p);
}

const auto optTimeoutMicros = HPHP::StaticString("timeout_micros");
const auto optMetadata = HPHP::StaticString("metadata");
static Object HHVM_METHOD(GrpcChannel, unaryCallInternal, const String &method,
                          const String &req, const Array &opt) {
  UnaryCallParams p;
  p.method_ = method.toCppString();
  if (opt.exists(optTimeoutMicros)) {
    p.timeout_micros_ = opt[optTimeoutMicros].toInt64();
  }

  if (opt.exists(optMetadata)) {
    auto metadata = opt[optMetadata].toDict();
    for (auto it = metadata.begin(); !it.end(); it.next()) {
      auto k = it.first().toString().toCppString();
      if (p.md_.find(k) == p.md_.end()) {
        p.md_[k] = std::vector<std::string>();
      }
      for (auto vit = it.second().toArray().begin(); !vit.end(); vit.next()) {
        p.md_[k].push_back(vit.second().toString().toCppString());
      }
    }
  }

  auto event = new GrpcEvent(req);
  auto *d = Native::data<ChannelData>(this_);
  event->data_->ctx_ = std::move(d->channel_->GrpcClientUnaryCall(p, event));
  return Object{event->getWaitHandle()};
}

static void HHVM_METHOD(GrpcChannel, serverStreamingCallInternal,
                        const String &method, const String &req) {
  auto *d = Native::data<ChannelData>(this_);
  d->channel_->ServerStreamingCall();
}

static String HHVM_METHOD(GrpcChannel, Debug) {
  auto *d = Native::data<ChannelData>(this_);
  return d->channel_->Debug();
}

const StaticString s_ChannelData("ChannelData");
struct GrpcExtension : Extension {
  GrpcExtension() : Extension("grpc", "0.0.1") { GrpcClientInit(); }

  void moduleInit() override {
    HHVM_ME(GrpcChannel, __construct);
    HHVM_ME(GrpcChannel, unaryCallInternal);
    HHVM_ME(GrpcChannel, serverStreamingCallInternal);
    HHVM_ME(GrpcChannel, Debug);
    Native::registerNativeDataInfo<ChannelData>(s_ChannelData.get());

    HHVM_ME(GrpcUnaryCallResult, StatusCode);
    HHVM_ME(GrpcUnaryCallResult, StatusMessage);
    HHVM_ME(GrpcUnaryCallResult, StatusDetails);
    HHVM_ME(GrpcUnaryCallResult, Response);
    HHVM_ME(GrpcUnaryCallResult, Peer);
    Native::registerNativeDataInfo<GrpcUnaryCallResult>(
        GrpcUnaryCallResult::s_className.get());

    loadSystemlib();
  }
} s_grpc_extension;

HHVM_GET_MODULE(grpc);

} // namespace HPHP
