#!/bin/bash --login
CURRENT=${PWD}
git_root=$(git rev-parse --show-toplevel)
linter="${git_root}/misc/cpplint.py --counting=detailed"
git_user_name=$(git config user.name)
cd ${git_root}
FILES=$(git ls-files | grep -iP "\.(hpp|cc|h|java|scala)$")
for f in ${FILES}; do
    if [[ $(head -n1 ${f} | grep Copyright) == "" ]]; then
        f_contents=$(cat ${f});
        echo "// Copyright 2017 ${git_user_name}" > ${f};
        echo "//" >> ${f}
        echo "${f_contents}" >> ${f};
    fi
    # echo "Formatting ${f}";
    clang-format -i ${f};
    # echo "cpplinting ${f}";
    # uses CPPLINT.cfg
    python ${linter} ${git_root}/${f}

done
cd ${CURRENT}
