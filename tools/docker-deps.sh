#!/bin/bash

# Copyright 2018 SMF Authors
#


# targeted to be used by a Dockerfile

set -e

# shellcheck disable=SC1091
source /etc/os-release

function redhat() {
    dnf update -y
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
function debian() {
    apt-get update -y
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

