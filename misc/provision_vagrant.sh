#!/bin/bash

# Copyright 2017 Alexander Gallego
#


set -ex
echo "usage: provision_vagrant.sh (debug|quiet)?"
if [[ $1 == "debug" ]]; then
    export VAGRANT_LOG=info
fi

export PYTHONUNBUFFERED=1

system="darwin"
if [[ $(which lsb_release) != "" ]]; then
    system=$(lsb_release -si | tr '[:upper:]' '[:lower:]' )
fi

git_root=$(git rev-parse --show-toplevel)
cd $git_root

if [[ $system == "fedora" ]]; then
    if [[ $(which vagrant) == "" ]]; then
        sudo dnf install vagrant vagrant-libvirt vagrant-libvirt-doc vagrant-lxc
    fi
    if [[ $(which VirtualBox) == "" ]]; then
        pushd /etc/yum.repos.d/
        ## Fedora 25/24/23/22/21/20/19/18/17/16 users
        sudo bash -c 'wget http://download.virtualbox.org/virtualbox/rpm/fedora/virtualbox.repo | tee '
        popd
        sudo dnf update
        sudo dnf install binutils gcc make patch libgomp glibc-headers glibc-devel \
             kernel-headers kernel-PAE-devel dkms libffi-devel ruby-devel VirtualBox
    fi
fi

if [[ $system == "darwin" ]]; then
    if [[ $(which brew) == "" ]]; then
        /usr/bin/ruby -e "$(curl -fsSL https://raw.githubusercontent.com/Homebrew/install/master/install)"
    fi
    if [[ $(which vagrant) == "" ]]; then
        brew cask install vagrant virtualbox
    fi
fi
if [[ $(vagrant plugin list | grep vbguest) == "" ]]; then
    vagrant plugin install vagrant-vbguest
fi
vagrant halt && vagrant up --provision --provider virtualbox
[[ $? != 0 ]] && echo "Broken Vagrant" && exit $?
