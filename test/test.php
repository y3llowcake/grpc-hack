<?hh // strict

<<__EntryPoint>>
function main(): void {
	echo "test start\n";
	$r = HH\Asio\join(grpc_unary_call('hello world'));
	echo "code: {$r->StatusCode()}\n";
	echo "message: {$r->StatusMessage()}\n";
	echo "test fin\n";
	sleep(2);
}
