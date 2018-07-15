package(default_visibility=["//visibility:public"])

_jar_deps = [
    "@com_github_davidmoten//jar",
    "@io_netty//jar",
    "@net_openhft//jar",
    "@org_apache_logging_log4j_api//jar",
    "@org_apache_logging_log4j_core//jar",
    "@com_github_luben_zstd_jni//jar",
]
java_library(
    name="smf",
    srcs=glob(["src/main/java/smf/**/*.java"]),
    resources=glob(["src/main/resources/*.properties"]),
    deps=_jar_deps,
)

java_binary(
    name="jclient",
    srcs=glob(["src/main/java/example/**/*.java"]),
    create_executable=True,
    main_class="example.demo.client.DemoApp",
    deps=[
        ":smf",
    ] + _jar_deps,
)
java_binary(
    name="jserver",
    srcs=glob(["src/main/java/example/**/*.java"]),
    create_executable=True,
    main_class="example.demo.server.DemoServer",
    deps=[
        ":smf",
    ] + _jar_deps,
)
