<?hh // strict

<<__EntryPoint>>
function main(): void {
  echo "test start\n";
  echo "version ".GrpcNative\Version()."\n";
  $cargs = GrpcNative\ChannelArguments::Create();
  echo "new\n";
  $channel = GrpcNative\Channel::Create('foo', 'localhost:50051', $cargs);
  $ctx = GrpcNative\ClientContext::Create();
  list($r, $s) = HH\Asio\join(
    $channel->UnaryCall($ctx, '/helloworld.HelloWorldService/SayHello', ''),
  );
  //$r = HH\Asio\join(grpc_unary_call('hello world'));
  echo "code: {$s->Code()}\n";
  echo "message: '{$s->Message()}'\n";
  echo "response length: ".strlen($r)."\n";
  echo "response: '{$r}'\n";
  echo "peer: '".$ctx->Peer()."'\n";
  echo $channel->Debug()."\n";
  $ctx = GrpcNative\ClientContext::Create();
  $reader = $channel->ServerStreamingCall(
    $ctx,
    '/helloworld.HelloWorldService/SayHelloStream',
    '',
  );
  echo "streaming...\n";
  while (HH\Asio\join($reader->Next())) {
    $r = $reader->Response();
    echo "response length: ".strlen($r)."\n";
    echo "response: '{$r}'\n";
  }
  $s = $reader->Status();
  echo "code: {$s->Code()}\n";
  echo "message: '{$s->Message()}'\n";

  echo "test fin\n";
}
