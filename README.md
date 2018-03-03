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


[![Analytics](https://ga-beacon.appspot.com/UA-99983285-1/chromeskel_a/readme?pixel)]()


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

Build Seastar

```bash
git clone https://github.com/scylladb/seastar.git
cd seastar
git submodule update --init --recursive

# --enable-dpdk to build w/ dpdk 
#
./configure 
ninja
```

Build smf

```bash
git clone https://github.com/senior7515/smf
cd smf
git submodule update --init --recursive
SEASTAR_DIR=/path/to/seastar ./debug
# or: SEASTAR_DIR=/path/to/seastar ./release
```

That's about it! 
