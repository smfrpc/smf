# [smf](http://senior7515.github.io/smf/) 


[![Build Status](https://semaphoreci.com/api/v1/senior7515/smf/branches/master/badge.svg)](https://semaphoreci.com/senior7515/smf)


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

**smf** is a set of libraries and utilities (like boost:: for C++ or guava for java)
designed to be the building blocks of your next distributed systems.


## RPC subsystem

It is a new RPC system and code generation like gRPC, Cap n Proto,
Apache Thrift, etc, but designed for **microsecond tail latency**.

Current benchmarks in microseconds

| 60 byte payload  | latency   |
| ---------------- | --------- |
| p50              | 7us       |
| p90              | 8us       |
| p99              | 8us       |
| p9999            | 15us      |
| p100             | 26us      |

## Write Ahead Log (WAL) subsystem

It is a write ahead log modeled after an Apache Kafka-like interface or 
Apache Pulsar. It has topics, partitions, etc. It is designed to have a
single reader/writer per topic/partition. 

Current benchmarks in milliseconds ==> 41X faster 

3 Producers latency vs Apache Kafka

| percentile  | Apache Kafka      | smf WAL  | speedup |
| ----------- | ----------------- | -------- |     --- |
| p50	     | 878ms		     | 21ms     |     41X |
| p95	     | 1340ms		    | 36ms     |     37x |
| p99	     | 1814ms		    | 49ms     |     37x |
| p999	    | 1896ms		    | 54ms     |     35x |
| p100	    | 1930ms		    | 54ms     |     35x |



