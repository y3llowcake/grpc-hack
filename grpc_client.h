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
struct Deserializer {
  virtual Status ResponseSlices(SliceList *) = 0;
};
struct Serializer {
  virtual void FillRequest(const void **, size_t *) const = 0;
};

//
// Metadata and context
//

typedef std::unordered_map<std::string, std::vector<std::string>> Md;

struct ClientCtx {
  static std::shared_ptr<ClientCtx> New();
  virtual std::string Peer() = 0;
};

//
// Unary Calls.
//

struct GrpcClientUnaryResultEvent {
  // Invoked when the request content is being serialized to the wire.
  virtual void FillRequest(const void **, size_t *) const = 0;

  // Invoked when the response is available to be deserialized from the wire.
  virtual void Response(std::unique_ptr<Deserializer>) = 0;

  // Invoked on completion.
  virtual void Done(Status s) = 0; // TODO parameter should be const ref?
};

//
// Channels
//

struct Channel {
  virtual void GrpcClientUnaryCall(const std::string &method,
                                   std::shared_ptr<ClientCtx> ctx,
                                   GrpcClientUnaryResultEvent *) = 0;
  virtual void ServerStreamingCall() = 0;
  virtual std::string Debug() = 0;
};

struct ChannelCreateParams {
  int max_send_message_size_;
  int max_receive_message_size_;
  std::string lb_policy_name_;
  ChannelCreateParams()
      : max_send_message_size_(0), max_receive_message_size_(0){};
};

std::shared_ptr<Channel> GetChannel(const std::string &name,
                                    const std::string &target,
                                    const ChannelCreateParams &params);

//
// Housekeeping.
//

void GrpcClientInit();

#endif // GRPC_CLIENT_H__
