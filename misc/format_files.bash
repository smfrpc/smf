#!/bin/bash --login
CURRENT=${PWD}
ROOT=$(git rev-parse --show-toplevel)
cd ${ROOT}
FILES=$(git ls-files | grep -iP "\.(hpp|cc|h|java)$")
for f in ${FILES}; do
    echo "Formatting ${f}"
    clang-format -i ${f};
done
cd ${CURRENT}
