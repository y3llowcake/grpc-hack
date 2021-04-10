load("@com_github_grpc_grpc//bazel:cc_grpc_library.bzl", "cc_grpc_library")

cc_library(
    name = "grpc_client_lib",
    srcs = ["grpc_client.cc"],
    hdrs = ["grpc_client.h"],
    deps = ["@com_github_grpc_grpc//:grpc++"],
    # linkopts = ['-ldl'],
)
