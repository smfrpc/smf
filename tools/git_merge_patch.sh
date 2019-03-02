#!/bin/bash --login

# Copyright 2017 Alexander Gallego
#

set -x
set -e
patch_file=$1
if [[ ! -e $patch_file ]]; then
    echo "file doesn't exist... need a valid patch file"
    exit 1
fi
echo "Working with file $patch_file"

git_root=$(git rev-parse --show-toplevel)
cd "$git_root"

git am --interactive "$patch_file"
echo "Remember to git push origin master"
