#!/bin/bash

set -ex

system=$(lsb_release -si | tr '[:upper:]' '[:lower:]' )
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
vagrant plugin install vagrant-vbguest
vagrant plugin install vagrant-disksize
vagrant halt && vagrant up --provision --provider virtualbox
[[ $? != 0 ]] && echo "Broken Vagrant" && exit $?
