package(default_visibility=["//visibility:public"])
genrule(
    name="gen",
    srcs=[
        "//demo_apps:demo_service_fbs",
    ],
    tools=["//src/smfc:smfc"],
    outs=["demo_service.smf.fb.go"],
    cmd=
    "./$(location //src/smfc:smfc) --logtostderr --filename=$< --output_path=$$(dirname $@)",
)
