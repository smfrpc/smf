#!/bin/bash --login
#set -ex
lsb_release -a
git_root=$(git rev-parse --show-toplevel)
exec $git_root/compile test
