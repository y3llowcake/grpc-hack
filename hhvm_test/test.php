<?hh // strict

<<__EntryPoint>>
function main(): void {
  echo "test start\n";
  $ctx = GrpcNative\ClientContext::Create();
  $channel = GrpcNative\Channel::Create(
    'foo',
    'localhost:50051',
    shape('lb_policy_name' => 'round_robin'),
  );
  $r = HH\Asio\join(
    $channel->UnaryCall($ctx, '/helloworld.HelloWorldService/SayHello', ''),
  );
  //$r = HH\Asio\join(grpc_unary_call('hello world'));
  echo "code: {$r->Status()->Code()}\n";
  echo "message: '{$r->Status()->Message()}'\n";
  $resp = $r->Response();
  echo "response length: ".strlen($resp)."\n";
  echo "response: '{$resp}'\n";
  // echo "peer: '".$r->Peer()."'\n";
  echo $channel->Debug()."\n";
  $channel->ServerStreamingCall('', '');
  echo "test fin\n";
}
