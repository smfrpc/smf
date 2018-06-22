#!/bin/bash

# Copyright 2018 SMF Authors
#

set -evx
CC=/usr/bin/gcc CXX=/usr/bin/g++ exec bazel build --verbose_failures "$@"
