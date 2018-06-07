---
layout: post
title: Introducing smf 
---


# **smf** is pronounced **/smɝf/**

--- 

| Important Links
| ---------------- 
| [Mailing List](https://groups.google.com/forum/#!forum/smf-dev)
| [Documentation](https://senior7515.github.io/smf/)


## [**smf** RPC]({{site.baseurl}}rpc)


**smf** is a new RPC system and code generation like gRPC, Cap n Proto,
Apache Thrift, etc, but designed for **microsecond tail latency**.

Current benchmarks in microseconds

| 60 byte payload  | latency   |
| ---------------- | --------- |
| p50              | 7us       |
| p90              | 8us       |
| p99              | 8us       |
| p9999            | 15us      |
| p100             | 26us      |


Highlights:
---

* Code generation compiler for RPC
* Load testing framework for RPC subsystem 
* Kernel-bypass RPC via DPDK
* 0-copy Serialization framework based on Google's Flatbuffers project
* Arbitrary filter chaining on incoming and outgoing channels (like twitter's Finagle)
* Small 16byte overhead
* Binary with pluggable compressors

## We need your help!

We have a lot of issues marked as [good first issue](https://github.com/senior7515/smf/labels/good%20first%20issue)

Look at the [contributing](https://github.com/senior7515/smf/blob/master/CONTRIBUTING.md) 
guideline for more details. 

Take a look at the issue list or send an email to the
[smf-dev mailing List](https://groups.google.com/forum/#!forum/smf-dev)
to get started. 

## Getting started


```bash
git clone https://github.com/senior7515/smf --recursive
cd smf

# alternatively if you didn't clone recursively
#
# git submodule update --init --recursive
#

mkdir build

cd build
cmake ..
make 

```

