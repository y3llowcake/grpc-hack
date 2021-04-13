#ifndef GRPC_CLIENT_H__
#define GRPC_CLIENT_H__

typedef std::vector<std::pair<const uint8_t*, size_t>> SliceList;

struct Deserializer {
  virtual void Slices(SliceList*) = 0;
};

struct GrpcClientUnaryResultEvent {
  // Invoked when status is available.
  virtual void Status(int code, const std::string& message, const std::string& details) = 0;
  // Invoked when the request content is being requested by the caller.
  virtual void FillRequest(const void**, size_t*) const = 0;
  
  // Invoked when the response is available, receiver takes ownership of the
  // deserializer.
  virtual void Response(std::unique_ptr<Deserializer>) = 0;

  // Invoked on completion.
  virtual void Done() = 0;
};

void GrpcClientUnaryCall(GrpcClientUnaryResultEvent*);

#endif // GRPC_CLIENT_H__
