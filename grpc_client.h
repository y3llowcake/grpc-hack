#ifndef GRPC_CLIENT_H__
#define GRPC_CLIENT_H__

#include <string>
#include <memory>
#include <unordered_map>
#include <vector>

struct Status {
  int code_;
  std::string message_;
  std::string details_;
  bool Ok() const;
};

typedef std::vector<std::pair<const uint8_t*, size_t>> SliceList;

struct Deserializer {
  virtual Status Slices(SliceList*) = 0;
};

typedef std::unordered_map<std::string, std::vector<std::string>> Md;

struct ClientContext {
  virtual void Metadata(Md*) = 0;
  virtual std::string Peer() = 0;
};

struct GrpcClientUnaryResultEvent {
  // Invoked when the request content is being serialized to the wire.
  virtual void FillRequest(const void**, size_t*) const = 0;
  
  // Invoked when the response is available to be deserialized from the wire.
  virtual void Response(std::unique_ptr<Deserializer>) = 0;

  // Invoked on completion.
  virtual void Done(Status s) = 0; // TODO parameter should be const ref?
};

struct UnaryCallParams {
  std::string method_;
  Md md_;
  int64_t timeout_micros_;
  UnaryCallParams() : timeout_micros_(0) {};
};

struct ChannelCreateParams {
};

struct Channel {
  virtual std::unique_ptr<ClientContext> GrpcClientUnaryCall(const UnaryCallParams&, GrpcClientUnaryResultEvent*) = 0;
  virtual std::string Debug() = 0;
};

std::shared_ptr<Channel> GetChannel(const std::string& name, const std::string& target, const ChannelCreateParams& params);

void GrpcClientInit();

#endif // GRPC_CLIENT_H__
