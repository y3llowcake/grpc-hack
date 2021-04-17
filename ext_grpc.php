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
    <<__Native>>
    public function SetTimeoutMicros(int $to): void;
    <<__Native>>
    public function AddMetadata(string $k, string $v): void;
  }

  /*  <<__NativeData("GrpcUnaryCallResult")>>
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
  	}*/

  <<__NativeData("GrpcStreamReader")>>
  final class StreamReader {
    private function __construct(): void {
      throw new InvalidOperationException(
        __CLASS__." objects cannot be directly created",
      );
    }
    <<__Native>>
    public function Read(): Awaitable<bool>;
    <<__Native>>
    public function Status(): Status;
    <<__Native>>
    public function Response(): string;
  }


  <<__NativeData("GrpcChannelArguments")>>
  final class ChannelArguments {
    private function __construct(): void {
      throw new InvalidOperationException(
        __CLASS__." objects cannot be directly created",
      );
    }
    <<__Native>>
    public static function Create(): ChannelArguments;
    <<__Native>>
    public function SetMaxMessageReceiveSize(int $size): void;
    <<__Native>>
    public function SetMaxMessageSendSize(int $size): void;
    <<__Native>>
    public function SetLoadBalancingPolicyName(string $name): void;

  }

  <<__NativeData("GrpcChannel")>>
  final class Channel {
    private function __construct(): void {
      throw new InvalidOperationException(
        __CLASS__." objects cannot be directly created",
      );
    }

    <<__Native>>
    public static function Create(
      string $name,
      string $target,
      ChannelArguments $args,
    ): Channel;

    <<__Native>>
    public function UnaryCall(
      ClientContext $ctx,
      string $method,
      string $request,
    ): Awaitable<(string, Status)>;


    <<__Native>>
    public function ServerStreamingCall(
      ClientContext $ctx,
      string $method,
      string $request,
    ): StreamReader;

    <<__Native>>
    function Debug(): string;
  }
}
