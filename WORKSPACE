# go

load("@bazel_tools//tools/build_defs/repo:http.bzl", "http_archive")

http_archive(
    name = "io_bazel_rules_go",
    urls = ["https://github.com/bazelbuild/rules_go/releases/download/0.13.0/rules_go-0.13.0.tar.gz"],
    sha256 = "ba79c532ac400cefd1859cbc8a9829346aa69e3b99482cd5a54432092cbc3933",
)
http_archive(
    name = "bazel_gazelle",
    urls = ["https://github.com/bazelbuild/bazel-gazelle/releases/download/0.13.0/bazel-gazelle-0.13.0.tar.gz"],
    sha256 = "bc653d3e058964a5a26dcad02b6c72d7d63e6bb88d94704990b908a1445b8758",
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

maven_jar(
    name="com_github_luben_zstd_jni",
    artifact="com.github.luben:zstd-jni:1.3.5-1",
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
