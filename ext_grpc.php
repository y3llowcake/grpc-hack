<?hh


/*
type Status = shape(
	'code' => int,
	'message' => ?string,
	'details' => ?string,
);

type UnaryCallResult = shape(
	'status' => Status,
	'response' => ?string,
);
 */

<<__NativeData("GrpcUnaryCallResult")>> 
final class GrpcUnaryCallResult {
	private function __construct(): void {
		throw new InvalidOperationException(__CLASS__ . " objects cannot be directly created");
	}

	<<__HipHopSpecific, __Native>>
	public function StatusCode(): int;
}

<<__HipHopSpecific, __Native>>
function grpc_unary_call(string $data): Awaitable<GrpcUnaryCallResult>;
//function grpc_unary_call(string $data): Awaitable<Status>;

//function grpc_unary_call(string $data): UnaryCallResult
