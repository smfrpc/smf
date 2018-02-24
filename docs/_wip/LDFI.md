---
layout: post
title:  "smf::condor - zero effort lineage driven fault injection"
date:   2017-12-21 00:00:00
categories: rpc smf condor 
comments: True
---

# WIP

- TODO(agallego) - add refs.
<br />


`smf::condor` is a proposal to autogenerate an oracle
that would proxy the requests between any two systems to systematically
explore failures of distributed systems using the `smf` RPC.

Lineage Driven Fault Injection (LDFI) is a method for 
finding faults by reasnoning backwards (_hazard step_) from correct results
to determine whether failures in the execution could have 
prevented that result. 


Failures explored:

* permanent crashed failrues 
* message loss 
* network partitions. 

Failures not explored:

* byzantine failures
* crash-recovery


The ideas for `smf::condor` come from Molly, a design and implementation by 
P. Alvaro, J. Rosen and J Hellerstine. These 2 systems differ in significant ways:

* Molly evalues a dadalus program (datalog-like langugage)
* `smf:condor` is an extension to an RPC generator and the first phase will
not implement a hazard step automatically.

The idea behind Molly is that, in the context of a protocol, every 
state change has pre-conditions, execution and post-conditions. Therefore when
there is a match of the same pre-conditions but different post-conditions, the
system has found a failure!

Molly first runs a _forward step_ of a distributed program given a set of inputs,
obtaining an outcome. Then _hazard step_ tries deduce everything
that had to happen to produce that outcome and see if it can make it not true.
That is, how can the system inject a failure to make the outcome not true.

For example, say your protocol for an RPC was this: 

```


rpc_service MyService {

   // return | x |
   int abs(int x);
}


```


A _forward step_ would be, give `MyService::abs(1)` and see it return `1`. It is key
to record the result of `1` as the system needs the lineage-enriched output.

The _hazard step_ would be to pass the inputs and recorded outputs to a SAT 
solver and have the SAT solver try to break it by for exmaple, giving it `MyService::abs(-INF)`.

Molly has many merits. In particular the mental model it gives programmers 
to reason about sensitivity to implementation mistakes. 

Some protocols for example expect perfect unique id's for transactions or deduplication. 
These protocols fail if you control the source of entropy (i.e.: /dev/urandom ) for generating 
these ids. Message replay of the same could cause catastrophic failures - such as double
charging your bitcoins.

Molly list the faults that any general-purpose verifyer should have. 
I will focus on the RPC general-purpose verifyer as an abstraction layer 
such that with zero work you get a state of the art fault-injector. 

* Message loss: `drop n out of N`
* Crash failures: `sys.exit(1)`
* Nondeterminisim in ordering and timing: `add arbitrary delayes per message(rpc) same as above for RPC` 

Note that this system is for correctness and not performance. 

# `smf::condor` codegenerated LDFI for a given fspec

* Main assumption: 

  * Succesfully delivered messages are received in a deterministic order
  * systematically explore failures.


Failures explored:

* permanent crashed failrues 
* message loss 
* network partitions. 

Failures not explored:

* byzantine failures
* crash-recovery

## Data model

```
struct fspec {
  // logical bound of executions. A counter.
  // end_of_time
  eot: uint = 0;
  
  // used for failure quiescence. That is, let the
  // system recover 
  // logical time at which message loss stops.
  //
  // eot < eff - always
  //
  // end_of_finite_failures
  eff: uint = 0;
  
  // number of times to call sys.exit(1)
  // Process crash counter.
  //
  crashes: uint = 0;
}

struct ldfi_test {
   // a duration of 0 means do not explore,
   // instead use the manually loaded spec
   // to exercise the system
   //
   duration_ms: uint = 0;
   spec:        fspec;
}


```


For trivial protocols like request-response, etc., 
we can model the protocol at the code generation level. 

For more complex protocols with harbeats, keepalives, 
anti-entropy, timeouts, retries, etc, the system 
programmer must extend some interface like such:

```

// basically any callable. - need to figure out input
// and output type
//
//
template<typename Input, 
         typename... Outputs>
struct ldfi_backward_step {
    seastar::future<...Outputs> operator()(Input i) {}
}

```


Currently, the proposal does not cover the _hazard step_
of Molly. This is deferred to the second phase of implementation.


## Implementation proposal

`smf::rpc` comes with a code generation program `smf_gen`. Currently
it generates RPC stubs like Thrift, gRPC, Cap 'n Proto, etc. The idea
is to extend this code generation step behind a flag of `--ldfi`.

Because we understand the inputs and outputs (flatbuffers c-structs),
as well as the actual communication infrastructure (`smf::rpc`), we can 
generate a proxy that can do the basic accounting. As such, ALL communications
for a system being tested will go through a single process proxy.

This proxy `smf::condor` will control:

* Control message loss: by simply dropping messages (not proxying)
* Delivery ordering and timing: by adding arbitrary `future<> sleep()` delays
to messages

In addition of this proxy, smf needs to provide filters to do the actual crashing
of processes via `exit(1)`.


```

namespace smf {

struct ldfi_exit_filter : rpc_filter<rpc_envelope> {

  seastar::future<rpc_envelope> operator()(rpc_envelope &&e) {
      if (e.letter.dynamic_headers.get("LDFI_EXIT") == "1") {
         exit(1);
      }
      return seastar::make_ready_future<rpc_envelope>(std::move(e));
  }
};

} // namespace smf

```

