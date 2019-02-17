#!/bin/bash

# Copyright 2018 SMF Authors
#

set -e

. /etc/os-release

function debs() {
    apt-get update -y

    local extra=""
    if [ -n "${USE_CLANG}" ]; then
        extra=clang
    else
        # ensure gcc is installed so we can test its version
        apt-get install -y build-essential
        gcc_ver=$(gcc -dumpfullversion -dumpversion)
        if dpkg --compare-versions ${gcc_ver} lt 8.0; then
            if ! command -v add-apt-repository; then
                apt-get -y install software-properties-common
            fi
            add-apt-repository -y ppa:ubuntu-toolchain-r/test
            apt-get update -y
            apt-get install -y gcc-8 g++-8
            update-alternatives --remove-all gcc || true
            update-alternatives --install /usr/bin/g++ g++-8 /usr/bin/g++-8  100
            update-alternatives --install /usr/bin/gcc gcc-8 /usr/bin/gcc-8  100
        fi
    fi

    apt-get install -y \
        cmake \
        build-essential \
        m4 \
        pkg-config \
        liblz4-dev \
        xfslibs-dev \
        libsctp-dev \
        systemtap-sdt-dev \
        libcrypto++-dev \
        ragel \
        libhwloc-dev ${extra}
}

function rpms() {
    yumdnf="yum"
    if command -v dnf > /dev/null; then
        yumdnf="dnf"
    fi

    ${yumdnf} install -y redhat-lsb-core
    case $(lsb_release -si) in
        CentOS)
            MAJOR_VERSION=$(lsb_release -rs | cut -f1 -d.)
            $SUDO yum-config-manager --add-repo https://dl.fedoraproject.org/pub/epel/$MAJOR_VERSION/x86_64/
            $SUDO yum install --nogpgcheck -y epel-release
            $SUDO rpm --import /etc/pki/rpm-gpg/RPM-GPG-KEY-EPEL-$MAJOR_VERSION
            $SUDO rm -f /etc/yum.repos.d/dl.fedoraproject.org*
            ;;
    esac

    if [ -n "${USE_CLANG}" ]; then
        extra=clang
    fi

    ${yumdnf} install -y \
        cmake \
        gcc-c++ \
        make \
        zlib-devel \
        which \
        lz4-devel \
        cryptopp-devel \
        lksctp-tools-devel \
        ragel \
        hwloc-devel \
        numactl-devel \
        xfsprogs-devel \
        systemtap-sdt-devel \
        libpciaccess-devel \
        doxygen ${extra}
}

case $ID in
    debian|ubuntu|linuxmint)
        debs
        ;;

    centos|fedora)
        rpms
        ;;

    *)
        echo "$ID not supported. Install dependencies manually."
        exit 1
        ;;
esac
