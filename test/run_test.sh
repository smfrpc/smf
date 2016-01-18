#!/bin/bash --login
#set -ex
git_root=$(git rev-parse --show-toplevel)
grpc_libs_path="$git_root/meta/tmp/grpc/libs/opt"
protobuf_libs_path="$git_root/meta/tmp/grpc/third_party/protobuf/src/.libs"
nvml_libs="$git_root/meta/tmp/nvml/src/nondebug"
gtest_libs="$git_root/meta/tmp/googletest/build/googlemock/gtest:$git_root/meta/tmp/googletest/build/googlemock:$git_root/meta/tmp/google_benchmark"

export LD_LIBRARY_PATH=$grpc_libs_path:$protobuf_libs_path:$gtest_libs:$nvml_libs:$LD_LIBRARY_PATH
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

cd $1
test=$2
echo "Running $2 inside $1 directory"
exec $test
