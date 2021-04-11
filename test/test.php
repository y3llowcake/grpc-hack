<?hh // strict

<<__EntryPoint>>
function main(): void {
	echo "hi\n";
	print_r(Grpc\Extension\grpc_unary_call('foo'));
	echo "fin\n";
	sleep(5);
}
