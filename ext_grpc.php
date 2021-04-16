<?hh

namespace GrpcNative {
  <<__NativeData("GrpcStatus")>>
  final class Status {
    private function __construct(): void {
      throw new InvalidOperationException(
        __CLASS__." objects cannot be directly created",
      );
    }

    <<__Native>>
    public function Code(): int;

    <<__Native>>
    public function Message(): string;

    <<__Native>>
    public function Details(): string;
  }

  <<__NativeData("GrpcClientContext")>>
  final class ClientContext {
    private function __construct(): void {
      throw new InvalidOperationException(
        __CLASS__." objects cannot be directly created",
      );
    }

    <<__Native>>
    public static function Create(): ClientContext;

    <<__Native>>
    public function Peer(): string;
  }

  <<__NativeData("GrpcUnaryCallResult")>>
  final class UnaryCallResult {
    private function __construct(): void {
      throw new InvalidOperationException(
        __CLASS__." objects cannot be directly created",
      );
    }

    <<__Native>>
    public function Status(): Status;

    <<__Native>>
    public function Response(): string;
  }

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
    public function UnaryCall(
      ClientContext $ctx,
      string $method,
      string $request,
    ): Awaitable<UnaryCallResult>;

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
