<?hh // strict

<<__EntryPoint>>
function main(): void {
	echo "hi\n";
	print_r(HH\Asio\join(grpc_unary_call('foo'))->StatusCode());
	echo "fin\n";
	sleep(2);
}
