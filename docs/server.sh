#!/bin/bash
set -ex
bundle update
exec bundle exec jekyll serve
