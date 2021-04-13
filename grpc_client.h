#ifndef GRPC_CLIENT_H__
#define GRPC_CLIENT_H__

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

struct ClientContext {
};

struct GrpcClientUnaryResultEvent {
  // Invoked when the request content is being requested by the caller.
  virtual void FillRequest(const void**, size_t*) const = 0;
  
  // Invoked when the response is available.
  virtual void Response(std::unique_ptr<Deserializer>) = 0;

  // Invoked on completion.
  virtual void Done(Status s) = 0; // TODO parameter should be const ref?
};

struct UnaryCallParams {
  std::string target_;
  std::string method_;
  std::map<std::string, std::vector<std::string>> md_;
  int64_t timeout_micros_;
  UnaryCallParams() : timeout_micros_(0) {};
};

std::unique_ptr<ClientContext> GrpcClientUnaryCall(const UnaryCallParams&, GrpcClientUnaryResultEvent*);

void GrpcClientInit();

#endif // GRPC_CLIENT_H__
