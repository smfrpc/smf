---
layout: post
title: Introducing smf 
---


# **smf** is pronounced **/smɝf/**

--- 

| Site        | Link
| ------------ | --------
| Mailing List | https://groups.google.com/forum/#!forum/smf-dev
| Documentation | https://senior7515.github.io/smf/



**smf** is a set of mechanically friendly subsystems
that run under the [seastar](http://www.seastar-project.org/)
share-nothing paradigm.


## [RPC](/rpc)

* Code generation compiler for RPC
* Load testing framework for RPC subsystem 
* Kernel-bypass RPC via DPDK
* 0-copy Serialization framework based on Google's Flatbuffers project
* Arbitrary filter chaning on incoming and outgoing channels (like twitter's Finagle)
* Small 16byte overhead
* Binary with pluggable compressors

## [WAL - Write Ahead Log](/write_ahead_log)

* Write-Ahead-Log - WAL abstraction w/ IO_DIRECT support
* Write behind writers (configurable)
* Sharded (ability to run on every core efficiently)
* Paritioned (like Apache Pulsar or Apache Kafka)

## In the works:

* Chain Replication 
* Raft Consensus

## We need your help!

We have a lot of issues marked as [good first issue](https://github.com/senior7515/smf/labels/good%20first%20issue)

Look at the [contributing](https://github.com/senior7515/smf/blob/master/CONTRIBUTING.md) 
guideline, for more details. 

Take a look at the issue list, or send an email to the
[smf-dev mailing List](https://groups.google.com/forum/#!forum/smf-dev)
to get started. 

## Getting started


```bash


cd $(git rev-parse --show-toplevel)/misc
./provision_vagrant.sh


```

This will give you a working VirtualBox machine to edit & run code.

Development happens on Fedora25 OS & gcc6 environment, 
but we have Ubuntu support as long as you have gcc6.

Simply run:

```bash

ROOT=$(git rev-parse --show-toplevel)
cd $ROOT/meta
source source_ansible_bash
ansible-playbook playbooks/devbox_all.yml

```


# enabling core dumps 

These are the core pattern directives

```
%c  core file size soft resource limit of crashing process (since Linux 2.6.24)
%d  dump mode—same as value returned by prctl(2) PR_GET_DUMPABLE (since Linux 3.7)
%e  executable filename (without path prefix)
%E  pathname of executable, with slashes ('/') replaced by exclamation marks ('!') (since Linux 3.0).
%g  (numeric) real GID of dumped process
%h  hostname (same as nodename returned by uname(2))
%i  TID of thread that triggered core dump, as seen in the PID namespace in which the thread resides (since Linux 3.18)
%I  TID of thread that triggered core dump, as seen in the initial PID namespace (since Linux 3.18)
%p  PID of dumped process, as seen in the PID namespace in which the process resides
%P  PID of dumped process, as seen in the initial PID namespace (since Linux 3.12)
%s  number of signal causing dump
%t  time of dump, expressed as seconds since the Epoch, 1970-01-01 00:00:00 +0000 (UTC)
%u  (numeric) real UID of dumped process

```

```
 # This is what I set my core dump as. A bit verbose, but easy to read
 root$ echo 'core_dump.file_name(%e).signal(%s).size(%c).process_id(%p).uid(%u).gid(%g).time(%t).initial_pid(%P).thread_id(%I)' > /proc/sys/kernel/core_pattern
```

## asciinema

[![asciicast](https://asciinema.org/a/1u2j8vg20813jxmgbky7liwxr.png)](https://asciinema.org/a/1u2j8vg20813jxmgbky7liwxr?autoplay=1&loop=1&speed=2)

<a name="References"/>
# References

* [Persistent memory programming - pmem](http://pmem.io/)
* [Seastar Project](http://www.seastar-project.org/)
* [Data plane developtment kit - DPDK](http://dpdk.org/)
* [RAMCloud](https://ramcloud.atlassian.net/wiki/download/attachments/6848571/RAMCloudPaper.pdf)
* [Making lockless synchronization fast: performance implications of memory reclamation1](http://doi.ieeecomputersociety.org/10.1109/IPDPS.2006.163)
* [All files are not created equal: On the complexity of crafting crash-consistent applications](http://research.cs.wisc.edu/wind/Publications/alice-osdi14.pdf)
* [File consistency - danluu's blog post](http://danluu.com/file-consistency/)
* [Kafka Exactly Once](https://docs.google.com/document/d/11Jqy_GjUGtdXJK94XGsEIK7CP1SnQGdp2eF0wSw9ra8/edit)
* [flatbuffers](https://google.github.io/flatbuffers/)
* [seastar](http://www.seastar-project.org/)
* [gRPC](http://grpc.io)
* [Cassandra](https://cassandra.apache.org/)
* [PostgreSQL](https://www.postgresql.org/)
* [MySQL](https://www.mysql.com/)
* [Scylla](http://www.scylladb.com/)
* [Kudu](https://kudu.apache.org/)
* [Clock-pro caching algorithm](http://static.usenix.org/event/usenix05/tech/general/full_papers/jiang/jiang_html/html.html)
* [Cap'n Proto](https://capnproto.org/)

