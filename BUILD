cc_binary(
    name = "deal",
    srcs = ["deal.cpp"],
    deps = [":lib"],
)

cc_binary(
    name = "hearts",
    srcs = ["hearts.cpp"],
    deps = [":lib"],
    copts = ["-I/usr/local/include/tf", "-march=native"],
    linkopts = ["-L/usr/local/lib", "-ltensorflow_cc", "-ltensorflow_framework"],
)

cc_binary(
    name = "tournament",
    srcs = ["tournament.cpp"],
    deps = [":lib"],
    linkopts = ["-L/usr/local/lib", "-ltensorflow_cc", "-ltensorflow_framework"],
    copts = ["-I/usr/local/include/tf", "-march=native"],
)

cc_binary(
    name = "analyze",
    srcs = ["analyze.cpp"],
    deps = [":lib"],
    linkopts = ["-L/usr/local/lib", "-ltensorflow_cc", "-ltensorflow_framework"],
    copts = ["-I/usr/local/include/tf", "-march=native"],
)

cc_test(
    name = "tests",
    srcs = glob(["tests/*.cpp"]),
    copts = ["-I/usr/local/include/tf", "-Iexternal/gtest/include", "-march=native"],
    deps = [":lib", "@gtest//:main"],
)

cc_library(
    name = "lib",
    srcs = glob(["lib/*.cpp"]),
    hdrs = glob(["lib/*.h"]),
    copts = ["-I/usr/local/include/tf", "-march=native"],
)

cc_binary(
    name = "disttest",
    srcs = ["disttest.cpp"],
    deps = [":lib"],
    copts = ["-I/usr/local/include/tf", "-march=native"],
    linkopts = ["-L/usr/local/lib", "-ltensorflow_cc", "-ltensorflow_framework"],
)

cc_binary(
    name = "lzcnttest",
    srcs = ["lzcnttest.cpp"],
    copts = ["-march=native"],
    deps = [":lib"],
)
