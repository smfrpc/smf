#!/bin/bash

# Copyright 2019 SMF Authors
#


cmd="$*"
echo "Launching command in background: $cmd"
eval "${cmd}" &
while kill -0 $! >/dev/null 2>&1; do
    sleep 1m && echo "travis background: $(date)";
done
