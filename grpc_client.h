#ifndef GRPC_CLIENT_H__
#define GRPC_CLIENT_H__

struct GrpcClientUnaryResultEvent {
  virtual void status(int code, const std::string& message, const std::string& details) = 0;
  virtual void done() = 0;
};

void GrpcClientUnaryCall(const std::string&, GrpcClientUnaryResultEvent*);

#endif // GRPC_CLIENT_H__
