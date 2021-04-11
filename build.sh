#!/bin/bash
set -xeuo pipefail

HHVM_SOURCE_ROOT=/home/cy/co/hhvm

# hacky version of hphpize
cp ${HHVM_SOURCE_ROOT}/hphp/tools/hphpize/hphpize.cmake CMakeLists.txt

# bazel test //...
bazel build :grpc_client_lib

#
# https://hacklang.slack.com/archives/CAAGK9GKW/p1525741700000075
# So we use DSO_TEST_MODE for now because it points make at the folly header
# files in the hhvm source dir. They are currently missing from
# /usr/local/include, but that should get fixed at some point...
#
cmake -D HHVM_DSO_TEST_MODE=1 -D GRPC_CLIENT_LIB=`realpath ./bazel-bin/libgrpc_client_lib.a` .

make
PATH="${HHVM_SOURCE_ROOT}/hphp/hhvm/:$PATH" ./test/test.sh
