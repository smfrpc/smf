#!/bin/sh
# -*- mode:sh; tab-width:4; indent-tabs-mode:nil -*

setconf() {
    local key=$1
    local val=$2
    if grep -q ^$key= ${conf}; then
        sed -i -e "s:^$key=.*$:$key=$val:g" ${conf}
    else
        echo $key=$val >> ${conf}
    fi
}

conf=$1/.config
shift
machine=$1
shift
arch=$1
shift

#setconf CONFIG_RTE_MACHINE "${machine}"
#setconf CONFIG_RTE_ARCH "${arch}"

setconf CONFIG_RTE_LIBRTE_PMD_BOND n
setconf CONFIG_RTE_MBUF_SCATTER_GATHER n
setconf CONFIG_RTE_LIBRTE_IP_FRAG n
setconf CONFIG_RTE_APP_TEST n
setconf CONFIG_RTE_TEST_PMD n
setconf CONFIG_RTE_MBUF_REFCNT_ATOMIC n
setconf CONFIG_RTE_MAX_MEMSEG 8192
setconf CONFIG_RTE_EAL_IGB_UIO n
setconf CONFIG_RTE_LIBRTE_KNI n
setconf CONFIG_RTE_KNI_KMOD n
setconf CONFIG_RTE_LIBRTE_JOBSTATS n
setconf CONFIG_RTE_LIBRTE_LPM n
setconf CONFIG_RTE_LIBRTE_ACL n
setconf CONFIG_RTE_LIBRTE_POWER n
setconf CONFIG_RTE_LIBRTE_IP_FRAG n
setconf CONFIG_RTE_LIBRTE_METER n
setconf CONFIG_RTE_LIBRTE_SCHED n
setconf CONFIG_RTE_LIBRTE_DISTRIBUTOR n
setconf CONFIG_RTE_LIBRTE_PMD_CRYPTO_SCHEDULER n
setconf CONFIG_RTE_LIBRTE_REORDER n
setconf CONFIG_RTE_LIBRTE_PORT n
setconf CONFIG_RTE_LIBRTE_TABLE n
setconf CONFIG_RTE_LIBRTE_PIPELINE n
