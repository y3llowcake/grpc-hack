#ifndef GRPC_CLIENT_H__
#define GRPC_CLIENT_H__

#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

//
// Status
//

struct Status {
  int code_;
  std::string message_;
  std::string details_;
  bool Ok() const;
};

//
// SerDe
//

typedef std::vector<std::pair<const uint8_t *, size_t>> SliceList;
struct Response {
  virtual Status Slices(SliceList *) = 0;
};

struct Deserializer {
  virtual void ResponseReady(std::unique_ptr<Response>) = 0;
};
struct Serializer {
  // Invoked when the request content is being serialized to the wire.
  virtual void FillRequest(const void **, size_t *) const = 0;
};

//
// Metadata and context
//

typedef std::unordered_map<std::string, std::vector<std::string>> Md;

struct ClientContext {
  static std::shared_ptr<ClientContext> New();
  virtual void SetTimeoutMicros(int to) = 0;
  virtual void AddMetadata(const std::string &k, const std::string &v) = 0;
  virtual std::string Peer() = 0;
};

//
// Callbacks.
//

struct UnaryResultEvents : Serializer, Deserializer {
  // Invoked on completion.
  virtual void Done(Status s) = 0; // TODO parameter should be const ref?
};
/*
struct StreamResultEvents : Deserializer {
  virtual void Done(Status s) = 0;
}

struct StreamReader {
  virtual void Read(StreamResultEvents *e);
}
*/
//
// Channels
//

struct Channel {
  virtual void UnaryCall(const std::string &method,
                         std::shared_ptr<ClientContext> ctx,
                         UnaryResultEvents *) = 0;
  virtual void ServerStreamingCall(const std::string &method,
                                   std::shared_ptr<ClientContext> ctx) = 0;
  virtual std::string Debug() = 0;
};

//
// ChannelArguments
//
struct ChannelArguments {
  static std::shared_ptr<ChannelArguments> New();
  virtual void SetMaxReceiveMessageSize(int size) = 0;
  virtual void SetMaxSendMessageSize(int size) = 0;
  virtual void SetLoadBalancingPolicyName(const std::string &lb) = 0;
};

std::shared_ptr<Channel> GetChannel(const std::string &name,
                                    const std::string &target,
                                    std::shared_ptr<ChannelArguments> args);

//
// Housekeeping.
//

void GrpcClientInit();

#endif // GRPC_CLIENT_H__
