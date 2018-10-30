# [smf - the fastest RPC in the West](http://senior7515.github.io/smf/) [![Travis-ci Build Status](https://travis-ci.org/senior7515/smf.svg?branch=master)](https://travis-ci.org/senior7515/smf)


**smf** is pronounced **/smɝf/**

Site         | Link
------------ | --------
Mailing List  | https://groups.google.com/forum/#!forum/smf-dev
Documentation | https://senior7515.github.io/smf/

# [Official Documentation](https://senior7515.github.io/smf) 

Please visit our official documentation, 
it'll get you up and running in no time!

If you are using **smf**, drop us a line on the mailing list introducing your project. 

Please note that only CMake is the officially supported build system. Bazel is a WIP
until Java and Go ports are finished, it will not be officially supported.

# What is smf?

**smf** is a new RPC system and code generation like gRPC, Cap n Proto,
Apache Thrift, etc, but designed for **microsecond tail latency***.

Current benchmarks in microseconds

| 60 byte payload  | latency   |
| ---------------- | --------- |
| p50              | 7us       |
| p90              | 8us       |
| p99              | 8us       |
| p9999            | 15us      |
| p100             | 26us      |



# Getting started

Please see our quick
[getting started on our official docs!](https://senior7515.github.io/smf//getting_started/)

