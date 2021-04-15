<?hh // strict

<<__EntryPoint>>
function main(): void {
  echo "test start\n";
  $channel = GrpcChannel::Create(
    'foo',
    'localhost:50051',
    shape('lb_policy_name' => 'round_robin'),
  );
  $r = HH\Asio\join(
    $channel->UnaryCall('/helloworld.HelloWorldService/SayHello', ''),
  );
  //$r = HH\Asio\join(grpc_unary_call('hello world'));
  echo "code: {$r->StatusCode()}\n";
  echo "message: '{$r->StatusMessage()}'\n";
  $resp = $r->Response();
  echo "response length: ".strlen($resp)."\n";
  echo "response: '{$resp}'\n";
  echo "peer: '".$r->Peer()."'\n";
  echo $channel->Debug()."\n";
  echo "test fin\n";
}
