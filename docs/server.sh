#!/bin/bash

# Copyright 2019 SMF Authors
#

set -ex
bundle update
exec bundle exec jekyll serve
