#!/bin/bash
set -e

function debs() {
  if [ -n "${USE_CLANG}" ]; then
    extra=clang
  fi
  apt-get update
  apt-get install -y \
    pkg-config \
    libboost-dev \
    libboost-system-dev \
    libboost-program-options-dev \
    libboost-thread-dev \
    libboost-filesystem-dev \
    libboost-test-dev \
    build-essential \
    cmake \
    libgflags-dev \
    libgoogle-glog-dev \
    liblz4-dev \
    libaio-dev \
    libcrypto++-dev \
    libyaml-cpp-dev \
    protobuf-compiler \
    libprotobuf-dev \
    ragel \
    xfslibs-dev \
    libunwind-dev \
    systemtap-sdt-dev \
    libsctp-dev \
    libhwloc-dev \
    libxml2-dev \
    libpciaccess-dev \
    libgnutls28-dev \
    libre2-dev \
    ninja-build \
    doxygen \
    python ${extra}
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
    boost-devel \
    gnutls-devel \
    protobuf-devel \
    protobuf-compiler \
    cryptopp-devel \
    libpciaccess-devel \
    gflags-devel \
    glog-devel \
    libaio-devel \
    lz4-devel \
    hwloc-devel \
    yaml-cpp-devel \
    ragel \
    libunwind-devel \
    libxml2-devel \
    xfsprogs-devel \
    numactl-devel \
    systemtap-sdt-devel \
    lksctp-tools-devel \
    re2-devel \
    ninja-build \
    doxygen \
    python ${extra}
}

source /etc/os-release
case $ID in
  debian|ubuntu)
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
