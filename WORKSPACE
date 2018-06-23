# go

load("@bazel_tools//tools/build_defs/repo:http.bzl", "http_archive")

http_archive(
    name="io_bazel_rules_go",
    urls=[
        "https://github.com/bazelbuild/rules_go/releases/download/0.12.1/rules_go-0.12.1.tar.gz",
    ],
    sha256="8b68d0630d63d95dacc0016c3bb4b76154fe34fca93efd65d1c366de3fcb4294",
)

http_archive(
    name="bazel_gazelle",
    urls=[
        "https://github.com/bazelbuild/bazel-gazelle/releases/download/0.12.0/bazel-gazelle-0.12.0.tar.gz",
    ],
    sha256="ddedc7aaeb61f2654d7d7d4fd7940052ea992ccdb031b8f9797ed143ac7e8d43",
)

load(
    "@io_bazel_rules_go//go:def.bzl",
    "go_rules_dependencies",
    "go_register_toolchains",
)

go_rules_dependencies()

go_register_toolchains()

load("@bazel_gazelle//:deps.bzl", "gazelle_dependencies", "go_repository")

gazelle_dependencies()

# java

maven_jar(
    name="io_netty",
    artifact="io.netty:netty-all:4.1.25.Final",
)

maven_jar(
    name="com_github_davidmoten",
    artifact="com.github.davidmoten:flatbuffers-java:1.9.0.1",
)

maven_jar(
    name="net_openhft",
    artifact="net.openhft:zero-allocation-hashing:0.8",
)

maven_jar(
    name="org_apache_logging_log4j_core",
    artifact="org.apache.logging.log4j:log4j-core:2.11.0",
)

maven_jar(
    name="org_apache_logging_log4j_api",
    artifact="org.apache.logging.log4j:log4j-api:2.11.0",
)

go_repository(
    name="com_github_cespare_xxhash",
    commit="48099fad606eafc26e3a569fad19ff510fff4df6",
    importpath="github.com/cespare/xxhash",
)

# local repos
local_repository(
    name = "third_party_flatbuffers",
    path = "src/third_party/flatbuffers",
)
