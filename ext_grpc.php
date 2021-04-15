<?hh

<<__NativeData("GrpcUnaryCallResult")>>
final class GrpcUnaryCallResult {
  private function __construct(): void {
    throw new InvalidOperationException(
      __CLASS__." objects cannot be directly created",
    );
  }

  <<__Native>>
  public function StatusCode(): int;

  <<__Native>>
  public function StatusMessage(): string;

  <<__Native>>
  public function StatusDetails(): string;

  <<__Native>>
  public function Response(): string;

  <<__Native>>
  public function Peer(): string;

  // TODO, peer and metadata.
}

type UnaryCallOpt = shape(
  'timeout_micros' => ?int,
  'metadata' => ?dict<string, vec<string>>,
);
type ChannelCreateOpt = shape(
  'max_send_message_size' => ?int,
  'max_receive_message_size' => ?int,
  'lb_policy_name' => ?string,
);

<<__NativeData("ChannelData")>>
class GrpcChannel {
  <<__Native>>
  private function __construct(string $name, string $target, darray $opt);

  <<__Memoize>>
  public static function Create(
    string $name,
    string $target,
    ChannelCreateOpt $opt = shape(),
  ): GrpcChannel {
    return new GrpcChannel($name, $target, Shapes::toArray($opt));
  }

  <<__Native>>
  private function UnaryCallInternal(
    string $method,
    string $request,
    darray $opt,
  ): Awaitable<GrpcUnaryCallResult>;


  public function UnaryCall(
    string $method,
    string $request,
    UnaryCallOpt $opt = shape(),
  ): Awaitable<GrpcUnaryCallResult> {
    return $this->UnaryCallInternal($method, $request, Shapes::toArray($opt));
  }

  <<__Native>>
  function Debug(): string;
}
