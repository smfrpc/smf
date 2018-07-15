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
git clone https://github.com/senior7515/smf.git
cd smf
git clone https://github.com/senior7515/smf --recursive
./install-deps.sh
./src/third_party/seastar/install-dependencies.sh
./tools/build.sh -r
```
then, you can run sample server 
```
./build/release/demo_apps/cpp/demo_server -c 1 --ip 0.0.0.0 --port 7000 &
```
or client
```
./build/release/demo_apps/cpp/demo_client -c 1 --ip 0.0.0.0 --port 7000
```

Alternatively, you can use Dockerfile, enter tools/local_development, and then
```bash
docker build -t smf_base . 
```
and run 
```bash
docker run -p 7000:7000 -it smf_base bash
```
now, you have SMF build and sample apps, you can run for example demo_server, just type
```bash
./build/release/demo_apps/cpp/demo_server -c 1 --ip 0.0.0.0 --port 7000 &
```
and client (on the same machine)
```bash
./build/release/demo_apps/cpp/demo_client -c 1 --ip 0.0.0.0 --port 7000
```


