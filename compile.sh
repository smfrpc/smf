#!/bin/bash --login
JAVA_HOME=/usr/lib/jvm/java-8-oracle
bzl=/home/bin/bazel
bazel build --verbose_failures --verbose_explanations --explain=/dev/stdout  //src/...
