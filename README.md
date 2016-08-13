# smurf

* RPC mechanism using flatbuffers & seastar.
* Code generation for services.

# tl;dr

using seastar-project.org for multiple projects. direction to be defined

# debugging - set core_pattern

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
