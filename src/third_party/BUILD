package(default_visibility = ["//visibility:public"])

cc_library(
    name = "seastar",
    includes = ["include/seastar", "include/dpdk"],
    hdrs = glob(["include/seastar/**/*.hh",
                 "include/seastar/*.hh",
                 "include/dpdk/*.h"]),
    linkstatic=1,
    linkopts = [
        "-Wl,--whole-archive,-lseastar,--no-whole-archive"
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
