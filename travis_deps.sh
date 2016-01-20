#!/bin/bash --login
#set -ex
lsb_release -a
git_root=$(git rev-parse --show-toplevel)

cd $git_root/meta

source source_ansible_bash
ansible-playbook playbooks/devbox_all.yml &
ansible_pid=$!
while ps -p $ansible_pid; do
      echo "$(date +%r) : TRAVIS 10MIN TIMEOUT BYPASS";
      sleep 60;
done
