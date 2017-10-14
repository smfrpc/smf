---
layout: page
title: write ahead log 
---

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
files. Typically each SSD device will have a series of queues, say **44**
and each queue will have a depth of **11** 4K pages. If your
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
