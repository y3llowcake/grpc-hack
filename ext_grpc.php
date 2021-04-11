<?hh

namespace Grpc {
namespace Extension {

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

<<__Native>>
function grpc_unary_call(string $data): darray<string, mixed>;

//function grpc_unary_call(string $data): UnaryCallResult


}
}
