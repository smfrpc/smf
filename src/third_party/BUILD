# TODO(agallego)
# 1. Add versions to the seastar application dependent libraries
#

cc_library(
    name = "gargamellibs", # evil guy in smurfs
    includes = ["include", "include/seastar", "include/dpdk"],
    linkstatic=1,
    srcs = [
        "lib/libfolly.a",
    ],
    linkopts = [
        # folly deps
        "-lboost_filesystem"
        ,"-lboost_system"
        ,"-lboost_regex"
        ,"-lpthread"
        ,"-llzma"
        ,"-lz"
        ,"-lsnappy"
        ,"-llz4"
        ,"-ljemalloc"
        ,"-levent"
        ,"-ldouble-conversion"
        ,"-lssl"
        ,"-lgflags"
        ,"-lglog"

        # seastar deps
        ,"-Wl,--whole-archive,-L:lib/libseastar.a,--no-whole-archive"
        ,"-laio"
        ,"-lboost_program_options"
        ,"-lboost_system"
        ,"-lboost_filesystem"
        ,"-lstdc++"
        ,"-lm"
        ,"-lboost_thread"
        ,"-lcryptopp"
        ,"-lrt"
        ,"-lgnutls"
        ,"-lgnutlsxx"
        ,"-lxenstore"
        ,"-L:lib/liblrte_pmd_vmxnet3_uio"
        ,"-L:lib/liblrte_pmd_i40e"
        ,"-L:lib/liblrte_pmd_ixgbe"
        ,"-L:lib/liblrte_pmd_e1000"
        ,"-L:lib/liblrte_pmd_ring"
        ,"-L:lib/liblrte_hash"
        ,"-L:lib/liblrte_kvargs"
        ,"-L:lib/liblrte_mbuf"
        ,"-L:lib/liblethdev"
        ,"-L:lib/liblrte_eal"
        ,"-L:lib/liblrte_malloc"
        ,"-L:lib/liblrte_mempool"
        ,"-L:lib/liblrte_ring"
        ,"-L:lib/liblrte_cmdline"
        ,"-L:lib/liblrte_cfgfile"
        ,"-lrt"
        ,"-lm"
        ,"-ldl"
        ,"-lhwloc"
        ,"-lnuma"
        ,"-lpciaccess"
        ,"-lxml2"
        ,"-lz"
    ],
    copts = [
        "-DHAVE_DPDK"
        ,"-DRTE_MACHINE_CPUFLAG_SSE"
        ,"-DRTE_MACHINE_CPUFLAG_SSE2"
        ,"-DRTE_MACHINE_CPUFLAG_SSE3"
        ,"-DRTE_MACHINE_CPUFLAG_SSSE3"
        ,"-DRTE_MACHINE_CPUFLAG_SSE4_1"
        ,"-DRTE_MACHINE_CPUFLAG_SSE4_2"
        ,"-DRTE_MACHINE_CPUFLAG_AES"
        ,"-DRTE_MACHINE_CPUFLAG_PCLMULQDQ"
        ,"-DRTE_MACHINE_CPUFLAG_AVX"
        ,"-DRTE_COMPILE_TIME_CPUFLAGS=RTE_CPUFLAG_SSE,RTE_CPUFLAG_SSE2,RTE_CPUFLAG_SSE3,RTE_CPUFLAG_SSSE3,RTE_CPUFLAG_SSE4_1,RTE_CPUFLAG_SSE4_2,RTE_CPUFLAG_AES,RTE_CPUFLAG_PCLMULQDQ,RTE_CPUFLAG_AVX"
        ,"-DHAVE_XEN"
        ,"-DHAVE_HWLOC"
        ,"-DHAVE_NUMA"
    ],
    visibility = ["//visibility:public"]
)
