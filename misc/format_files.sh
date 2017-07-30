#!/bin/bash

set -e

CURRENT=${PWD}
git_root=$(git rev-parse --show-toplevel)

cpp_lint_cmd=$git_root/src/third_party/bin/cpplint.py
cppcheck_cmd=$git_root/src/third_party/bin/cppcheck
clang_format_cmd=$(which clang-format)
git_user_name=$(git config user.name)
seastar_includes=$(pkg-config --cflags-only-I $git_root/meta/tmp/seastar/build/debug/seastar.pc)


pushd ${git_root}
FILES=$(git ls-files | grep -iP "\.(hpp|cc|h|java|proto)$")

for f in ${FILES}; do
    file=$git_root/$f
    echo $file
    # Check copyright
    #
    if [[ $(head -n1 ${f} | grep Copyright) == "" ]]; then
        f_contents=$(cat ${file});
        echo "// Copyright 2017 ${git_user_name}" > ${f};
        echo "//" >> ${file}
        echo "${f_contents}" >> ${file};
    fi

    #echo "clang-format ${file}";
    $clang_format_cmd -i ${file};

    #echo "cppcheck ${file}";
    $cppcheck_cmd \
        -DUCHAR_MAX=255 \
        -I $git_root/src \
        -I $git_root/src/third_party/include \
        -I $git_root \
        ${seastar_includes} \
        -I /usr/local/include \
        --enable=warning,performance,portability,missingInclude \
        --std=c++11 \
        --language=c++ \
        --quiet \
        --suppress=missingIncludeSystem --suppress=preprocessorErrorDirective \
        --template="[{severity}][{id}]{message} {callstack} {file}:{line}" \
        $file

    #echo "CPPLINT ${file}"
    lint_output=$($cpp_lint_cmd --verbose=5 --counting=detailed ${file})
    if [[ 0 != $(echo $lint_output | awk '{print $7}') ]]; then
        echo "$lint_output"
        exit 1
    fi
done



popd
