#!/bin/bash
#set -ex
program_name="$1"
if [[ $program_name == "" ]]; then
    echo "usage: ./ghetto_package program_name ";
    exit 1;
fi
black_list=("\/lib64\/libpthread.so.0" "\/lib64\/libresolv.so.2" "\/lib64\/libc.so.6")
libs=$(ldd "$program_name" | awk '{print $3}' | sed  '/^$/d');
for b in ${black_list[@]}; do
    libs=$(echo "${libs}" | sed s/"${b}"//g | sed  '/^$/d')
done

output_dir="/tmp/${program_name}_$(git log --oneline -n1 | awk '{print $1}')"

if [[ -e $output_dir ]]; then
    rm -rf $output_dir;
fi

mkdir -p $output_dir/lib
mkdir -p $output_dir/bin
mkdir -p $output_dir/etc

cp $program_name $output_dir/bin
for f in "${libs[@]}"; do
    cp $f $output_dir/lib;
done

echo "Done packaging. Output folder: $output_dir"
