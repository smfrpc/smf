#!/bin/bash

# Copyright 2018 SMF Authors
#

set -e

. /etc/os-release

function debs() {
    if [ -n "${USE_CLANG}" ]; then
        extra=clang
    fi
    if [ "$ID" = "ubuntu" ]; then
        if [ ! -f /usr/bin/add-apt-repository ]; then
            apt-get -y install software-properties-common
        fi
        add-apt-repository -y ppa:ubuntu-toolchain-r/test
    fi
    apt-get update -y
    apt-get install -y \
            pkg-config \
            build-essential \
            g++-8 \
            gcc-8 \
            make \
            cmake \
            libaio-dev \
            libcrypto++-dev \
            xfslibs-dev \
            libunwind-dev \
            systemtap-sdt-dev \
            libsctp-dev \
            libxml2-dev \
            libpciaccess-dev \
            ninja-build \
            m4 \
            libtool \
            doxygen \
            stow \
            python ${extra}
    if [[ ! -z ${CI} ]]; then
        update-alternatives --remove-all gcc || true
        update-alternatives --install /usr/bin/g++ g++-8 /usr/bin/g++-8  100
        update-alternatives --install /usr/bin/gcc gcc-8 /usr/bin/gcc-8  100
    fi
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
              m4 \
              make \
              libpciaccess-devel \
              libaio-devel \
              libunwind-devel \
              libxml2-devel \
              xfsprogs-devel \
              systemtap-sdt-devel \
              lksctp-tools-devel \
              libtool \
              ninja-build \
              doxygen \
              stow \
              python ${extra}
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
