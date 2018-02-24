#!/bin/bash

# Copyright 2018 Alexander Gallego
#

set -evx
perf record \
     -e L1-icache-load-misses,branch-load-misses,cache-misses \
     -e LLC-loads,LLC-load-misses,instructions \
     -e cycles,branch-load-misses,faults,bus-cycles,mem-stores \
     -e branches \
     -p $1
