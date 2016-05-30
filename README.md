# smurf [![Build Status](https://travis-ci.org/senior7515/smurf.svg?branch=master)](https://travis-ci.org/senior7515/smurf)

Still not sure, if I should write an hdfs,kafka,or nothing w/ seastar.
check back in later - playing w/ the rpc mechanisms here.

# tl;dr

using seastar-project.org for multiple projects. direction to be defined

# debugging - set core_pattern

```
 root$ echo 'core.%e.%p.%h.%t' > /proc/sys/kernel/core_pattern
```
# getting started
```bash
 root$ cd $ROOT/meta
 root$ source source_ansible_bash
 root$ ansible-playbook playbooks/devbox_all.yml
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
