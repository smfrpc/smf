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

# first look for the subject line,
# then set the first field to blank
subject_line=$(grep '^Subject: ' "$patch_file" | awk '{$1=""; print $0}')

git send-email HEAD^..HEAD --compose --subject "$subject_line" --to smf-dev@googlegroups.com
