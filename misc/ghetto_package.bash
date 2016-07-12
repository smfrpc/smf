#!/bin/bash
# set -ex
program_name="$1"
if [[ $program_name == "" ]]; then
    echo "usage: ./ghetto_package program_name ";
    exit 1;
fi
libs=$(ldd "$program_name" | awk '{print $3}' | sed  '/^$/d');
output_dir=$(mktemp -d --suffix "$program_name")

mkdir -p $output_dir/lib
mkdir -p $output_dir/bin
mkdir -p $output_dir/etc

cp $program_name $output_dir/bin
for f in "${libs[@]}"; do
    cp $f $output_dir/lib;
done

echo "Done packaging. Output folder: $output_dir"
