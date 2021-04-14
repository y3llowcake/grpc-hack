<?hh

<<__NativeData("GrpcUnaryCallResult")>> 
final class GrpcUnaryCallResult {
	private function __construct(): void {
		throw new InvalidOperationException(__CLASS__ . " objects cannot be directly created");
	}

	<<__HipHopSpecific, __Native>>
	public function StatusCode(): int;

	<<__HipHopSpecific, __Native>>
	public function StatusMessage(): string;

	<<__HipHopSpecific, __Native>>
	public function StatusDetails(): string;

	<<__HipHopSpecific, __Native>>
	public function Response(): string;

	<<__HipHopSpecific, __Native>>
	public function Peer(): string;

	// TODO, peer and metadata.
}

type UnaryCallOpt = shape();
type ChannelCreateOpt = shape();

<<__NativeData("ChannelData")>>
class GrpcChannel {
	<<__Native>>
	private function __construct(
		string $name,
		string $target,
		darray $opt
	);

	<<__Memoize>>
	public static function Create(string $name, string $target, ChannelCreateOpt $opt = shape()): GrpcChannel {
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
