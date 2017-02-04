#!/usr/bin/bash

git_root=$(git rev-parse --show-toplevel)
function print_smf_version() {
    git_hash=$(git log -1 --pretty=format:"%H")
    git_author=$(git log -1 --pretty=format:"%aN")
    git_date=$(git log -1 --pretty=format:"%ai")
    git_subject=$(git log -1 --pretty=format:"%s")

    echo "#ifndef SMF_VERSION_H"
    echo "#define SMF_VERSION_H"
    echo
    echo "static const char *kGitVersion = \"${git_hash}\";"
    echo "static const char *kGitVersionAuthor = \"${git_author}\";"
    echo "static const char *kGitVersionDate   = \"${git_date}\";"
    echo "static const char *kGitVersionSubject ="
    echo "  \"${git_subject}\";"
    echo
    echo "#endif"

}

print_smf_version > "${git_root}/src/version.h"
