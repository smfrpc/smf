#!/bin/bash
git_root=$(git rev-parse --show-toplevel)
cd $git_root
vagrant up --provision --provider virtualbox
[[ $? != 0 ]] && echo "Broken Vagrant" && exit $?
