package(default_visibility = ["//visibility:public"])

java_library(
    name = "smf",
    srcs = glob(["src/main/java/smf/**/*.java"]),
    resources = glob(["src/main/resources/*.properties"]),
    deps = [
        "@com_github_davidmoten//jar",
        "@io_netty//jar",
        "@net_openhft//jar",
        "@org_apache_logging_log4j_api//jar",
        "@org_apache_logging_log4j_core//jar",
    ],
)

java_binary(
    name = "smf_client",
    srcs = glob(["src/main/java/example/**/*.java"]),
    create_executable = True,
    main_class = "example.demo.DemoApp",
    deps = [
        ":smf",
        "@com_github_davidmoten//jar",
        "@io_netty//jar",
        "@net_openhft//jar",
        "@org_apache_logging_log4j_api//jar",
        "@org_apache_logging_log4j_core//jar",
    ],
)
