#!/bin/bash --login
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
subject_line=$(grep 'Subject: ' $patch_file  | awk '{$1=""; print $0}')

# go to the root to apply patch
cwd=$CWD
git_root=$(git rev-parse --show-toplevel)
cd $git_root


git am -3 --scissors $patch_file

git send-email HEAD^..HEAD --compose --subject "$subject_line" --to smurf-dev@googlegroups.com
