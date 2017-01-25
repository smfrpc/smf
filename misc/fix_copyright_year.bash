#!/bin/bash --login
CURRENT=${PWD}
ROOT=$(git rev-parse --show-toplevel)
cd ${ROOT}
FILES=$(git ls-files | grep -iP "\.(hpp|cc|h|java|scala|js)$")
for f in ${FILES}; do
    year=$(git log --format=%aD ${f} | tail -1 | awk '{print $4}')
    existing_copyright=$(head -n1 ${f} | grep Copyright)
    comment="//"
    # if [[ $(echo "$f" | grep -iP "\.(bash|sh|py)$" ) != "" ]]; then
    #     comment="#";
    # fi

    copyright_line="${comment} Copyright (c) ${year} Alexander Gallego. All rights reserved.";
    file_contents="";

    if [[ "${existing_copyright}" == "" ]]; then
        file_contents=$(cat ${f});
    else
        file_contents=$(tail -n +3 "${f}");
    fi

    echo "adding copy right header to ${f}";
    echo "${copyright_line}" > ${f};
    echo "${comment}" >> ${f}
    echo "${file_contents}" >> ${f};

done
cd ${CURRENT}
