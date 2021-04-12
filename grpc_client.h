#ifndef GRPC_CLIENT_H__
#define GRPC_CLIENT_H__

struct GrpcClientUnaryResultEvent {
  // Invoked when status is available.
  virtual void status(int code, const std::string& message, const std::string& details) = 0;
  // Invoked when the request content is being requested by the caller.
  virtual void fillRequest(const void**, size_t*) const = 0;
  // Invoked on completion.
  virtual void done() = 0;
};

void GrpcClientUnaryCall(GrpcClientUnaryResultEvent*);

#endif // GRPC_CLIENT_H__
