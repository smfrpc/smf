# smurf [![Build Status](https://travis-ci.org/senior7515/smurf.svg?branch=master)](https://travis-ci.org/senior7515/smurf)
Fastest durable log broker in the west.

# tl;dr

Mechanically friendly logging service.

* Using the Seastar project we can bypass the TCP stack and run in user mode
* The persistent memory programming module allows you to store - reliably
bytes to disk at memory speeds. It is effectively a series of abstractions
around mmap(2) files and posix_fallocate(2) / truncate(2) + more.
* The format is intended for mid size records ~64KB. Records smaller are
encouraged and fast up to this max (65'536 bytes). Records bigger will get split
This is the same design as the SST tables on rocksdb and leveldb(not stable yet)

# debugging - set core_pattern

```
 root$ echo 'core.%e.%p.%h.%t' > /proc/sys/kernel/core_pattern
```
# References:

* [Persistent memory programming - pmem](http://pmem.io/)
* [Seastar Project](http://www.seastar-project.org/)
* [Data plane developtment kit - DPDK](http://dpdk.org/)
* [RAMCloud](https://ramcloud.atlassian.net/wiki/download/attachments/6848571/RAMCloudPaper.pdf)
* [Making lockless synchronization fast: performance implications of memory reclamation1](http://doi.ieeecomputersociety.org/10.1109/IPDPS.2006.163)
* [All files are not created equal: On the complexity of crafting crash-consistent applications](http://research.cs.wisc.edu/wind/Publications/alice-osdi14.pdf)
* [File consistency - danluu's blog post](http://danluu.com/file-consistency/)

Yours Truly,
[@gallegoxx](https://twitter.com/gallegoxx)
