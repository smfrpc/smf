---
layout: page
title: write ahead log 
---

We are fast!

# Latency benchmark vs Kafka
## 34x to 48x lower latency!

![alt text]({{ site.baseurl }}public/wal_kafka_latency_vs_smf_1producer.png "Latency comparison vs Kafka")

<br />


| Single Producer |
| -------------   |
| percentile | 	Apache Kafka | smf WAL | speedup |
| ---        | ---              |     --- |     --- |
| p50   	 | 411		      |      12 |     34x |
| p95	    | 921		      |      19 |     48x |
| p99	    | 980		      |      23 |     42x |
| p999	   | 993		      |      23 |     43x |
| p100	   | 995		      |      23 |     43x |


![alt text]({{ site.baseurl }}public/wal_kafka_latency_vs_smf_3producers.png "Latency comparison vs Kafka")

<br />


| 3 Producers latency vs Apache Kafka     |
| --------------------------------------- |
| percentile | Apache Kafka   | smf WAL | speedup |
| ---        | ---            | ---     |     --- |
| p50	    | 878ms		  | 21ms    |     41X |
| p95	    | 1340ms		 | 36ms    |     37x |
| p99	    | 1814ms		 | 49ms    |     37x |
| p999	   | 1896ms		 | 54ms    |     35x |
| p100	   | 1930ms		 | 54ms    |     35x |



<br />

# QPS benchmark vs Kafka
## 4x more throughput 

![alt text]({{ site.baseurl }}public/kafka_vs_smf_qps.png "QPS comparison vs Kafka")

<br />


| QPS in millions (left image)
| -------------
| Kafka: 1 producer	| 0.423908
| smf: 1 producer	| 1.712


| QPS in millions (right image)
| -------------          |          |
| Kafka: 3 producers	 | 0.955888 |
| smf: 3 producers	     | 2.269661 |



<br />

**NOTE:** I have not finished the reader part yet. This is only the writer path.
The **smf** server was running DPDK but the clients were running libevent.
See benchmark details at the bottom.


<br />

**smf**'s write ahead log is typically used to provide atomicity and
durability. Most distributed systems that manipulate data with promises of
recovering your data after a crash use a form of a WAL. These include databases:
[Cassandra](https://cassandra.apache.org/),
[PostgreSQL](https://www.postgresql.org/),
[MySQL](https://www.mysql.com/),
[Scylla](http://www.scylladb.com/),
[Kudu](https://kudu.apache.org/)
, filesystems:
[JFS](https://en.wikipedia.org/wiki/Journaling_file_system)
, even embedded databases:
[SQLite](https://www.sqlite.org/),
[RocksDB](http://rocksdb.org/), etc,
to name a few - there are very very many more.

We provide a WAL abstraction that is very performant. It uses the
O_DIRECT capabilities of the seastar filesystem API for both reads and writes.
It also provides hooks into the IO Scheduler of seastar to prioritize 
write requests, invalidations & reads. 

You can find examples in our chain-replication subsystem. Here is the gist:

```cpp

auto req = smf::wal_write_request{
            p, 
            smf::priority_manager::streaming_write_priority(),
            {it->partition()}
          };
          
```

Where the `streaming_write_priority()` could be anything that extends the IO
priority requests of seastar. This is the same technique used by ScyllaDB
to do background compactions while serving user requests at millisecond 
latencies.


On the read side, we implement
the patent-free
[clock-pro](http://static.usenix.org/event/usenix05/tech/general/full_papers/jiang/jiang_html/html.html)
algorithm for page caching and eviction. Each file
open has a deterministic memory footprint, so you know exactly how much memory
your application will use. Specifically, you know exactly how many file handles
you can have open in the worst case.

Although configurable, the expected memory footprint per **file** is this:

```cpp

    // either 10% or 10 pages. If file has less than 10 pages, then all
    // each page is typically 4096 - pulled dynamically from sysconf()
    // per filesystem

    const auto percent     = uint32_t(number_of_pages * .10);
    const auto min_default = std::min<uint32_t>(10, number_of_pages);
    max_resident_pages_    = std::max<uint32_t>(min_default, percent);
```

**NOTE:** We will be changing the read strategy to use all of the freely 
available memory using the seastar allocator hooks

![alt text]({{ site.baseurl }}public/smf_wal_filesystem.png "Intended WAL architecture")

The page caching and eviction algorithm can be succinctly described from the
authors (Feng Chen and Xiaodong Zhang) paper:

![alt text]({{ site.baseurl }}public/clock_pro_cache.png "The Clock: clock pro cache")

Note: since we don't use the OS page cache, we MUST do page caching at the
application level.

## Code usage

{% gist 28e098f49e84a2e3b2115b07fd264674 %}


# Seastar Friendly

```cpp

seastar::future<wal_write_reply> wal_impl_with_cache::append(
  wal_write_request r) {
  // 1) Get projection
  // 2) Write to disk
  // 3) Write to to cache
  // 4) Return reduced state

  return seastar::map_reduce(
    // for-all
    r.begin(), r.end(),

    // Mapper function
    [this, r](auto it) {
      auto topic = seastar::sstring(r.req->topic()->c_str());
      return this->tm_.get_manager(topic, it->partition()).then([
        this, topic, projection = wal_write_projection::translate(*it)
      ](auto mngr) { return mngr->append(projection); });
    },

    // Initial State
    wal_write_reply(0, 0),

    // Reducer function
    [this](auto acc, auto next) {
      acc.data.start_offset =
        std::min(acc.data.start_offset, next.data.start_offset);
      acc.data.end_offset = std::max(acc.data.end_offset, next.data.end_offset);
      return acc;
    });
}



```

Each l-core will get an instance of the write_ahead_log that is local
to the core. This is a common seastar paradigm for sharded<T> types.

On each core the strategy is to **simultaneously** dispatch IO to different 
files. Typically each SSD device will have a series of queues with a maximum
number of total **concurrent IO** globally, say **33**. If your
goal is to saturate the disk from the NIC as fast as possible, you 
need to dispatch them concurrently. Seastar's IO will handle backpressure
until all the requests are satified (use iotune from the seastar folder to find out
the actual numer for each device).

Our **write_ahead_log** uses a **single reader/writer per topic/partition pair**. 
This means that there is only one core responsible for reading / writing
one file. There are no locks, atomics or shared state anywhere. Each 
topic/partition pair will satisfy all reads and writes to a particular file.

Each core can, of course, handle multiple read/write requests to different
topic/partitions. 

![alt text]({{ site.baseurl }}public/smf_write_ahead_log.png "distributed write ahead log")


### Details about benchmark

* end-to-end latency measured (client's perspective)
* XFS filesystem
* Fedora 25
* Latest kafka source code (kafka_2.11-0.11.0.1.tgz)
* Latest smf master branch (381fa7d)
* 2 isolated servers:
    * 120G ram
    * 32cores - dual socket Intel(R) Xeon(R) CPU E5-2640 v3 @ 2.60GHz 
    * 10Gbps NIC - SPF+
    * 1SSD hard drive - SanDisk CloudSpeed Eco Gen II SDLF1DAR-480G 480GB SATA SSD 
    * no VM's, bare metal
* Both are running w/ lz4 encoding
* Benchmarks were ran 3 times. Took best time out of both. 
For example, I discarded runs where kafka took 15s the first time it created a topic, etc.


Kafka producers were launched like this:

```bash


/bin/kafka-run-class.sh org.apache.kafka.tools.ProducerPerformance --topic test --num-records 10000000 --record-size 100 --throughput -1 --producer-props acks=1 bootstrap.servers=x.x.x.x:9092

```

**smfb** brokers were launched like this:

```bash

# Bring down the NIC for DPDK
ifconfig ens3f1 down

# Load up UIO drivers
modprobe uio_pci_generic

# Reserve 50G per CPU socket - Node 1
echo 50 >  /sys/devices/system/node/node1/hugepages/hugepages-1048576kB/nr_hugepages 

# Reserve 50G per CPU socket - Node 0
echo 50 >  /sys/devices/system/node/node0/hugepages/hugepages-1048576kB/nr_hugepages

# Bind the NIC w/ DPDK
/smf/meta/tmp/seastar/scripts/dpdk_nic_bind.py --bind=uio_pci_generic ens3f1

# Show the status of the NIC and ensure that DPDK owns it
/smf/meta/tmp/seastar/scripts/dpdk_nic_bind.py --status


# Run the server
sudo /smf/build_release/src/smfb/smfb --ip x.x.x.x \ 
                                      --num-io-queues  11 \
                                      --max-io-requests 44 \
                                      --poll-mode \
                                      --write-ahead-log-dir . \
                                      --host-ipv4-addr x.x.x.x \
                                      --netmask-ipv4-addr 255.255.255.0 \
                                      --gw-ipv4-addr x.x.x.1 \
                                      --dhcp 0 \
                                      --dpdk-pmd \
                                      --network-stack native
```

**smfb** clients were launched like this:

```cpp

# cpu set is used so that you don't get your threads moved around
# SMP means the number of cores
# Number of actual messages was 122 * 8196 == 999912 
# This was for the 3 producers
#
# Note the client was launched w/ regular libevent and NOT DPDK
# although we *do* have a DPDK enabled client, we think most ppl won't
# actually use a DPDK-NIC-owning client, but will likely have a DPDK NIC owning service
#
#
sudo /smf/build_release/src/smfb/client/smfb_low_level_client \ 
                                 --ip x.x.x.x \
                                 --poll-mode \
                                 --req-num 122 \
                                 --batch-size 8196 \
                                 --cpuset 8,10,12 --smp 3

```
