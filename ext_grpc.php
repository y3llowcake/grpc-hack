<?hh

namespace Grpc {
  <<__NativeData("Grpc\UnaryCallResultData")>>
  final class UnaryCallResult {
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
  class Channel {
    <<__Native>>
    private function __construct(string $name, string $target, darray $opt);

    <<__Memoize>>
    public static function Create(
      string $name,
      string $target,
      ChannelCreateOpt $opt = shape(),
    ): Channel {
      return new Channel($name, $target, Shapes::toArray($opt));
    }

    <<__Native>>
    private function unaryCallInternal(
      string $method,
      string $request,
      darray $opt,
    ): Awaitable<UnaryCallResult>;


    public function UnaryCall(
      string $method,
      string $request,
      UnaryCallOpt $opt = shape(),
    ): Awaitable<UnaryCallResult> {
      return $this->unaryCallInternal($method, $request, Shapes::toArray($opt));
    }

    public function ServerStreamingCall(): void {
      $this->serverStreamingCallInternal('', '');
    }

    <<__Native>>
    public function serverStreamingCallInternal(
      string $method,
      string $request,
    ): void;

    <<__Native>>
    function Debug(): string;
  }

}
