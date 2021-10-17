# [smf - the fastest RPC in the West](http://smfrpc.github.io/smf/) [![Travis-ci Build Status](https://app.travis-ci.com/senior7515/smf.svg?branch=master)](https://travis-ci.org/smfrpc/smf)

![](docs/public/logo.png)

> We're looking for a new maintainer for the SMF project. As I have little time to keep up with issues. Please let me know by filing an issue.


**smf** is pronounced **/smɝf/**

Site         | Link
------------ | --------
Mailing List  | https://groups.google.com/forum/#!forum/smf-dev
Documentation | https://smfrpc.github.io/smf/

# [Official Documentation](https://smfrpc.github.io/smf)

Please visit our official documentation, 
it'll get you up and running in no time!

If you are using **smf**, drop us a line on the mailing list introducing your project. 


# What is smf?

**smf** is a new RPC system and code generation like gRPC, Cap n Proto,
Apache Thrift, etc, but designed for **microsecond tail latency***.

Current benchmarks in microseconds (e2e see docs)

| 60 byte payload  | latency   |
| ---------------- | --------- |
| p50              | 7us       |
| p90              | 8us       |
| p99              | 8us       |
| p9999            | 15us      |
| p100             | 26us      |



# Getting started

Please see our quick
[getting started on our official docs!](https://smfrpc.github.io/smf//getting_started/)
