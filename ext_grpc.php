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
}

<<__NativeData("ChannelData")>>
class GrpcChannel {
	<<__Native>>
	public function __construct(string $name, string $target);

	<<__Native>>
	function UnaryCall(
		string $method,
		int $timeout_micros,
		dict<string, vec<string>> $metadata,
		string $request,
	): Awaitable<GrpcUnaryCallResult>;
	
	<<__Native>>
	function Debug(): string;
}


