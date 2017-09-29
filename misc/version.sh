#!/usr/bin/env bash

# Copyright 2017 Alexander Gallego
#


git_root=$(git rev-parse --show-toplevel)
function print_smf_version() {
    git_hash=$(git log -1 --pretty=format:"%H")
    git_short_hash=$(git log -1 --pretty=format:"%h")
    git_date=$(git log -1 --pretty=format:"%ai")

    echo "#ifndef SMF_VERSION_H"
    echo "#define SMF_VERSION_H"
    echo
    echo "namespace smf {"
    echo "struct source_code_version {"
    echo "  static const char *kGitVersion = \"${git_hash}\";"
    echo "  static const char *kGitShortVersion = \"${git_short_hash}\";"
    echo "  static const char *kGitVersionDate  = \"${git_date}\";"
    echo "};"
    echo "}  // namespace smf"
    echo
    echo "#endif"

}

print_smf_version > "${git_root}/src/version.h"
