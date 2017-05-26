#!/bin/bash

set -e

if [[ $(which inotifywait) == "" ]]; then
    echo "You do not have inotifywait installed."
    echo "In fedora builds you want to use: dnf install inotify-tools"
fi
system="darwin"
if [[ $(which lsb_release) != "" ]]; then
    system=$(lsb_release -si | tr '[:upper:]' '[:lower:]' )
fi


git_root=$(git rev-parse --show-toplevel)
cd $git_root

# First make sure that it actually builds
./debug


function build_no_tests(){
    pushd $git_root/build_debug
    if [[ $system == "fedora" ]]; then
        ninja-build
    else
        ninja
    fi
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
        sleep 5
        echo
        echo
        echo
    done
done
