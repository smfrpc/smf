# [smf - the fastest RPC in the West](http://senior7515.github.io/smf/) [![Travis-ci Build Status](https://travis-ci.org/senior7515/smf.svg?branch=master)](https://travis-ci.org/senior7515/smf)
[![FOSSA Status](https://app.fossa.io/api/projects/git%2Bgithub.com%2Fsenior7515%2Fsmf.svg?type=shield)](https://app.fossa.io/projects/git%2Bgithub.com%2Fsenior7515%2Fsmf?ref=badge_shield)


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

Build smf

```bash
git clone https://github.com/senior7515/smf
cd smf
git submodule update --init --recursive
mkdir build
cd build
make 

# to run test do 
ctest -V
```
That's about it! 


## License
[![FOSSA Status](https://app.fossa.io/api/projects/git%2Bgithub.com%2Fsenior7515%2Fsmf.svg?type=large)](https://app.fossa.io/projects/git%2Bgithub.com%2Fsenior7515%2Fsmf?ref=badge_large)