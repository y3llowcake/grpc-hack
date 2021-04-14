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

<<__HipHopSpecific, __Native>>
	function grpc_unary_call(
		string $target,
		string $method,
		int $timeout_micros,
		dict<string, vec<string>> $metadata,
		string $request,
	): Awaitable<GrpcUnaryCallResult>;

<<__HipHopSpecific, __Native>>
function grpc_client_debug(): string;
