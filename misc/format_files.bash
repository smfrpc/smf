#!/bin/bash --login
CURRENT=${PWD}
ROOT=$(git rev-parse --show-toplevel)
cd ${ROOT}
FILES=$(git ls-files | grep -iP "\.(hpp|cc|h|java)$")
for f in ${FILES}; do
    if [[ $(head -n1 ${f} | grep Copyright) == "" ]]; then
        f_contents=$(cat ${f});
        echo "// Copyright 2016 Alexander Gallego" > ${f};
        echo "//" >> ${f}
        echo "${f_contents}" >> ${f};
    fi
    # echo "Formatting ${f}";
    clang-format -i ${f};
    # echo "cpplinting ${f}";
    # uses CPPLINT.cfg
    python ${CURRENT}/cpplint.py --counting=detailed ${ROOT}/${f}

done
cd ${CURRENT}
