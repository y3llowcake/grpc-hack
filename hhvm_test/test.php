<?hh // strict

<<__EntryPoint>>
function main(): void {
	echo "test start\n";
	$r = HH\Asio\join(grpc_unary_call('loaclhost:50051', '/helloworld.HelloWorldService/SayHello', ''));
	//$r = HH\Asio\join(grpc_unary_call('hello world'));
	echo "code: {$r->StatusCode()}\n";
	echo "message: '{$r->StatusMessage()}'\n";
	$resp = $r->Response();
	echo "response length: " . strlen($resp)."\n";
	echo "response: '{$resp}'\n";
	echo "test fin\n";
	sleep(2);
}
