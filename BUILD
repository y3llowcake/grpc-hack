# load("@com_github_grpc_grpc//bazel:cc_grpc_library.bzl", "cc_grpc_library")

cc_library(
    name = "grpc_client_lib",
    srcs = ["grpc_client.cc"],
    hdrs = ["grpc_client.h"],
    deps = ["@com_github_grpc_grpc//:grpc++"],
    linkstatic = True,
    # linkopts=['-static'],
    # linkshared = True,
    # linkopts = ['-ldl'],
)


#genrule(
#    name="g",
#    srcs=[":grpc_client_lib"],
#    # cmd="$(AR) $(locations :a) $(locations :b)",
#    cmd="echo $(locations :grpc_client_lib) && echo foo && exit 1",
#    # cmd="echo foo",
#    outs = ["foo.a"],
#)
# https://github.com/bazelbuild/bazel/issues/1920
