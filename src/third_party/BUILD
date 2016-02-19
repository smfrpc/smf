# TODO(agallego)
# 1. Add versions to the seastar application dependent libraries
#

cc_library(
    name = "folly",
    hdrs = glob(["include/folly/*.h"]),
    linkstatic=1,
    srcs = ["lib/libfolly.so.57.0.0"],
    copts = ["-Ilib"],
    linkopts = [
        "-libboost_filesystem.so.1.55.0"
        ,"-libboost_system.so.1.55.0"
        ,"-libboost_regex.so.1.55.0"
        ,"-libpthread.so.0"
        ,"-liblzma.so.5"
        ,"-libz.so.1"
        ,"-libsnappy.so.1"
        ,"-liblz4.so.1"
        ,"-libjemalloc.so.1"
        ,"-libevent-2.0.so.5"
        ,"-libdouble-conversion.so.1"
        ,"-libssl.so.1.0.0"
        ,"-libgflags.so.2"
        ,"-libglog.so.0"
    ]
)

cc_library(
    name = "seastar",
    linkstatic=1,
    hdrs = glob([
        "include/seastar/*.h",
        "include/seastar/*.hh"
    ]),
    linkopts = [
        "-Wl,--whole-archive,-lseastar,--no-whole-archive"
        ,"-pthread"
        ,"-aio"
        ,"-boost_program_options"
        ,"-boost_system"
        ,"-boost_filesystem"
        ,"-stdc++"
        ,"-m"
        ,"-boost_thread"
        ,"-cryptopp"
        ,"-rt"
        ,"-gnutls"
        ,"-gnutlsxx"
        ,"-xenstore"
        ,"-rte_pmd_vmxnet3_uio"
        ,"-rte_pmd_i40e"
        ,"-rte_pmd_ixgbe"
        ,"-rte_pmd_e1000"
        ,"-rte_pmd_ring"
        ,"-rte_hash"
        ,"-rte_kvargs"
        ,"-rte_mbuf"
        ,"-ethdev"
        ,"-rte_eal"
        ,"-rte_malloc"
        ,"-rte_mempool"
        ,"-rte_ring"
        ,"-rte_cmdline"
        ,"-rte_cfgfile"
        ,"-rt"
        ,"-m"
        ,"-dl"
        ,"-hwloc"
        ,"-numa"
        ,"-pciaccess"
        ,"-xml2"
        ,"-z"
    ],
    copts = [
        "-Ilib"
        ,"-DHAVE_DPDK"
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
    ]
)
