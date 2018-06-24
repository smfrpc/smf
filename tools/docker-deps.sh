#!/bin/bash

# targeted to be used by a Dockerfile

set -e
. /etc/os-release
function redhat {
    dnf update
    dnf install \
        git \
        curl \
        pkgconfig \
        zip \
        gcc-c++ \
        zlib-devel \
        unzip \
        which -y

}
function debian {
    apt-get update
    apt-get install \
            git-core \
            curl \
            pkg-config \
            zip \
            g++ \
            zlib1g-dev \
            unzip -y

}
case $ID in
    debian|ubuntu|linuxmint)
        debian
        ;;

    centos|fedora)
        redhat
        ;;

    *)
        echo "$ID not supported. Install bazel manually."
        exit 1
        ;;
esac
BAZEL_VERSION=0.14.0
curl -fSsL -O https://github.com/bazelbuild/bazel/releases/download/$BAZEL_VERSION/bazel-$BAZEL_VERSION-installer-linux-x86_64.sh
chmod +x bazel-*.sh
./bazel-$BAZEL_VERSION-installer-linux-x86_64.sh
