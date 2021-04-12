#ifndef GRPC_CLIENT_H__
#define GRPC_CLIENT_H__

struct GrpcClientUnaryResultEvent {
  virtual void done(int) = 0;
};

void GrpcClientUnaryCall(GrpcClientUnaryResultEvent*);

#endif // GRPC_CLIENT_H__
