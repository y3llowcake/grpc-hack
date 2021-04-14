<?hh // strict

<<__EntryPoint>>
function main(): void {
	echo "test start\n";
	$channel = new GrpcChannel('foo', 'localhost:50051');
	$r = HH\Asio\join($channel->UnaryCall('/helloworld.HelloWorldService/SayHello', 0, dict['foo' => vec['bar', 'fizz']], ''));
	//$r = HH\Asio\join(grpc_unary_call('hello world'));
	echo "code: {$r->StatusCode()}\n";
	echo "message: '{$r->StatusMessage()}'\n";
	$resp = $r->Response();
	echo "response length: " . strlen($resp)."\n";
	echo "response: '{$resp}'\n";
	echo $channel->Debug() . "\n";
	echo "test fin\n";
}
