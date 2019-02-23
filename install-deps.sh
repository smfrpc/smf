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
        libtool \
        m4 \
        ninja-build \
        automake \
        pkg-config \
        xfslibs-dev \
        systemtap-sdt-dev \
        ragel ${extra}
}

function rpms() {
    yumdnf="yum"
    if command -v dnf > /dev/null; then
        yumdnf="dnf"
    fi

    case ${ID} in
      centos|rhel)
        MAJOR_VERSION="$(echo $VERSION_ID | cut -d. -f1)"
        $SUDO yum-config-manager --add-repo https://dl.fedoraproject.org/pub/epel/$MAJOR_VERSION/x86_64/
        $SUDO yum install --nogpgcheck -y epel-release
        $SUDO rpm --import /etc/pki/rpm-gpg/RPM-GPG-KEY-EPEL-$MAJOR_VERSION
        $SUDO rm -f /etc/yum.repos.d/dl.fedoraproject.org*
        if test $ID = centos -a $MAJOR_VERSION = 7 ; then
          yum install -y centos-release-scl
          yum install -y devtoolset-8
          dts_ver=8
        fi
        ;;
    esac

    if [ -n "${USE_CLANG}" ]; then
        extra=clang
    fi

    cmake="cmake"
    case ${ID} in
      centos|rhel)
        MAJOR_VERSION="$(echo $VERSION_ID | cut -d. -f1)"
        if test $MAJOR_VERSION = 7 ; then
          cmake="cmake3"
        fi
    esac

    ${yumdnf} install -y \
        ${cmake} \
        gcc-c++ \
        m4 \
        libtool \
        make \
        which \
        ragel \
        xfsprogs-devel \
        systemtap-sdt-devel \
        doxygen ${extra}

    if [ -n "$dts_ver" ]; then
      if test -t 1; then
        # interactive shell
        cat <<EOF
Your GCC is too old. Please run following command to add DTS to your environment:

scl enable devtoolset-8 bash

Or add following line to the end of ~/.bashrc to add it permanently:

source scl_source enable devtoolset-8

see https://www.softwarecollections.org/en/scls/rhscl/devtoolset-8/ for more details.
EOF
      else
        # non-interactive shell
        source /opt/rh/devtoolset-$dts_ver/enable
      fi
    fi
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
