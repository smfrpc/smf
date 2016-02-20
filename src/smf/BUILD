cc_binary(
    name = "smf",
    srcs = ["main.cc"],
    copts = [
        "-fPIC"
        ,"-fuse-ld=gold"
        ,"-Wall"
        ,"-Werror"
        ,"-Wextra"
        ,"-Wformat"
        ,"-pedantic"
        ,"-Wmissing-braces"
        ,"-Wparentheses"
        ,"-Wpointer-arith"
        ,"-Wformat-security"
        ,"-Wunused"
        ,"-Wno-unused-parameter"
        ,"-Wcast-align"
        ,"-Wmissing-field-initializers"
        ,"-O2"
        ,"-std=c++1y"
        ,"-ftemplate-depth-256"
        ,"-Woverloaded-virtual"
        ,"-Wsign-promo"
        ,"-Wreorder"
        ,"-fvisibility=hidden"
        ,"-Wno-error=literal-suffix"
        ,"-Wno-literal-suffix"
        ,"-Wno-invalid-offsetof"
    ],
    deps = ["//src/third_party:gargamellibs"]
)
