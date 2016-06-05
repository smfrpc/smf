#!/bin/bash
#set -x
set -e
test=$1
directory=$2
if [[ ! -e  $test ]]; then
    exit 1
fi

git_root=$(git rev-parse --show-toplevel)
third_party_libs_dir="$git_root/src/third_party/lib:$git_root/src/third_party/lib64"
export LD_LIBRARY_PATH=$third_party_libs_dir:$LD_LIBRARY_PATH
export GLOG_logtostderr=${GLOG_logtostderr:='1'}
export GLOG_v=${GLOG_v:='1'}
export GLOG_vmodule=${GLOG_vmodule,''}
export GLOG_logbufsecs=${GLOG_logbufsecs:='0'}
export GLOG_log_dir=${GLOG_log_dir:='.'}
export GTEST_COLOR=${GTEST_COLOR:='no'}
export GTEST_SHUFFLE=${GTEST_SHUFFLE:='1'}
export GTEST_BREAK_ON_FAILURE=${GTEST_BREAK_ON_FAILURE:='1'}
export GTEST_REPEAT=${GTEST_REPEAT:='1'}
# export GTEST_FILTER='] = ARGUMENTS.get('filter','*')

set -x
cd $directory
if [[ -e "pre_test_hook" ]]; then
    echo "Running pre test hook"
    sh ./pre_test_hook
fi
exec $test
set +x
