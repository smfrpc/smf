# [smf](http://senior7515.github.io/smf/)[![Travis Build Status](https://travis-ci.org/senior7515/smf.svg?branch=master)](https://travis-ci.org/senior7515/smf) 

**smf** is pronounced **/smɝf/**

Site         | Link
------------ | --------
Mailing List  | https://groups.google.com/forum/#!forum/smf-dev
Documentation | http://senior7515.github.io/smf/

## Brief

`smf` is a set of
[mechanically friendly](https://groups.google.com/forum/#!forum/mechanical-sympathy)
subsystems. It is the evolution of a set of distributed-sys primitives
that run under the [seastar](http://www.seastar-project.org/)
share-nothing paradigm.

See the rest of the docs here: http://senior7515.github.io/smf/

Currently we have:

* Code generation compiler for RPC
* Load testing framework for RPC subsystem 
* Kernel-bypass RPC & Serialization framework based on Seastar & Google's Flatbuffers project
* Write-Ahead-Log - WAL abstraction w/ kernel bypass


In the works:

* Chain Replication ~70%
* Raft Consensus    ~40%


## We need your help!

First, look at the [contributing](CONTRIBUTING.md) guideline.

Take a look at the issue list, or send an email to the
[smf-dev mailing List](https://groups.google.com/forum/#!forum/smf-dev)
to get started. 

## Getting started


```bash


cd $(git rev-parse --show-toplevel)/misc
./provision_vagrant.sh


```

This will give you a working VirtualBox machine to edit & run code.

Development happens on Fedora25 OS & gcc6 environment. If you have that
environment, the build system, transitive dependencies, etc are all automated.

Simply run:

```bash

ROOT=$(git rev-parse --show-toplevel)
cd $ROOT/meta
source source_ansible_bash
ansible-playbook playbooks/devbox_all.yml

```

We welcome contributions to port `smf` to other platforms, and OS's.

If you don't have a Fedora25 available, use the VirtualBox script above.


## asciinema

[![asciicast](https://asciinema.org/a/1u2j8vg20813jxmgbky7liwxr.png)](https://asciinema.org/a/1u2j8vg20813jxmgbky7liwxr?autoplay=1&loop=1&speed=2)


Yours Truly,
* [@emaxerrno](https://twitter.com/emaxerrno)
* [site](http://alexgallego.org)


[![Analytics](https://ga-beacon.appspot.com/UA-99983285-1/chromeskel_a/readme?pixel)]()
