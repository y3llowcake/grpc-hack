<?hh // strict

<<__EntryPoint>>
function main(): void {
  echo "test start\n";
  $cargs = GrpcNative\ChannelArguments::Create();
  echo "new\n";
  $channel = GrpcNative\Channel::Create('foo', 'localhost:50051', $cargs);
  echo "wutang\n";
  $ctx = GrpcNative\ClientContext::Create();
  $r = HH\Asio\join(
    $channel->UnaryCall($ctx, '/helloworld.HelloWorldService/SayHello', ''),
  );
  //$r = HH\Asio\join(grpc_unary_call('hello world'));
  echo "code: {$r->Status()->Code()}\n";
  echo "message: '{$r->Status()->Message()}'\n";
  $resp = $r->Response();
  echo "response length: ".strlen($resp)."\n";
  echo "response: '{$resp}'\n";
  echo "peer: '".$ctx->Peer()."'\n";
  echo $channel->Debug()."\n";
  $channel->ServerStreamingCall('', '');
  echo "test fin\n";
}
