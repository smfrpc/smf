#!/bin/bash

this_dir="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
rootdir="$(cd "${this_dir}"/.. && pwd)"

if ! command -v shellcheck > /dev/null; then
  echo "shellcheck not installed. see: https://github.com/koalaman/shellcheck"
  exit 1
fi

find "${rootdir}" -name '*.sh' -exec shellcheck {} \;
