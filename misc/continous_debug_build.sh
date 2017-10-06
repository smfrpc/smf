#!/bin/bash

# Copyright 2017 Alexander Gallego
#


if [[ $(which inotifywait) == "" ]]; then
    echo "You do not have inotifywait installed."
    echo "In fedora builds you want to use: dnf install inotify-tools"
fi
system="darwin"
if [[ $(which lsb_release 2> /dev/null) != "" ]]; then
    system=$(lsb_release -si | tr '[:upper:]' '[:lower:]' )
fi


git_root=$(git rev-parse --show-toplevel)
cd $git_root

# First make sure that it actually builds
./debug

build_tool="ninja"
if [[ $system == "fedora" ]]; then
    build_tool="ninja-build"
fi

function build_no_tests(){
    pushd $git_root/build_debug
    $build_tool
    popd
}

file_watch_extensions=(h cc)
build_counter=1
# set up the watches on root/src
while true; do
    echo "******************************************************"
    echo "Setting up watches"
    echo
    echo
    while inotifywait -q -r \
                      -e create,close_write,delete,modify,moved_to,moved_from \
                      $git_root/src; do

        let build_counter++
        echo "Triggering build: ${build_counter}"
        echo
        build_no_tests
        if [[ $? != 0 ]]; then
            echo "Bad exit code from build system...: $?"
            for i in {{1..10}}; do
                echo "Trying again in $((10-$i)) seconds"
                sleep 1
            done
        else
            sleep 5
        fi
        echo
        echo
        echo
    done
done
