---
layout: post
title: l0a (Limit0 Allocator) 
---

> WARNING: This code has no benchmarks, don't trust it! (wip)

l0a is an allocator that attempts to exploit memory access patterns for
queue-like reading processes. That is, typical consumers of queues move forward
at high speeds in 3 major patterns:

1. last hot pages (tailing iterators) - say 1MB
2. k-distance from tail - say 10MB from tail (per consumer)
3. scan-the-world queries

The take away is that they often reset their offset to a previous epoch 
(old page), but after that epoch they keep moving forward - slowly or not.

Tailing iterators are solved by simply caching the last 1MB per publishing page
since readers and writers are `google::consistent_jump`'ed to the same core. 
They are not convered in this allocator, they are addessed in the 
`topic_partition_manager` class, with the strategy mentioned above.


NOTE THIS ONLY WORKS BECAUSE THEY ARE LARGE CHUNKS

This algo is only good for 2,3



The `l0a` allocator is a multi tier allocation strategy.

* First level is the raw byte eviction strategy:
  * via bump allocators and
  * TCP-Like windowing
* Second level is the CLOCK-Pro allocator:
  * W leverage for page eviction and 
  * to be resilient against scan-the-world queries.

The description of `l0a` is on a `thread_local` basis since 
it was programmed to be executed under the *seastar* framework.

There is of course room for improvement if we were to take a global view,
which we don't cover.

At the time of writing, *smf* was 41X faster than Apache Kafka for writing data
to disk in a log structured way. Records on disk contain a header (
compression, sizes, checksums) and a payload which is just a bag of bytes.

Reading the data for smf was behind a cache-per-file-read strategy. Every file
reader pre-allocated 10 pages and even if the data 
was never read, it occupied memory.

Even if memory was allocated lazily
(and aligned to 4096 bytes) there was no opportunity for global (per core) 
cache eviction optimizations. 

The CLOCK-Pro [^1] page eviction algorithm was designed to support multiple 
workloads for the linux kernel and *specially* be resistant to 
scientific-data-scanning workloads. This covers *pages* and not raw byte allocations.

The specific use case of this memory allocator and page eviction strategy
is for reading with a queue-like (almost always forward progress)
access where there are stragglers. 
In practice, when queue's are used in the ideal scenario - are produced-to and 
consumed-from at a steady state - you have a few of hot pages (say 100)
that contain all the data you need. Apache Kafka, for example,
was optimized for this use case.

As with any software, use-cases mature/evolve (also the tech) and currently
Apache Kafka among others are used for almost every imaginable workloads. 
From disaster recovery [^2](read from the begining), 
to pub-sub for IoT[^3], 
to buffering spikes in web requests[^4]. 

All of these use cases have different memory usage patterns. As such,
the intrinsic design of using the system page-cache which has no application
level knowledge, cannot be used efficiently in a multi-use-case deployment - which is usually the case. 
This is exacerbated by the fact
that *data writes as well page reads* go through the system page cache. This means
that *both* your readers and writers can have non-trivial performance effects 
for each other.

This allocator is built on top of the seastar allocator which pre-allocates
the full system memory at startup and splits it per core. For example, assuming 10G and 2 cores,
each core would get 5G.

# Byte Allocation Strategy	

Bump allocators for types are at the heart of it all. The gist of a bump 
allocator is you create a memory region (pool) per type (and therefore size) 
of object. When you get a request to allocate, you bump the allocator by a 
predetermined size (usually `sizeof(T)`). At the end you wrap for free'd regions
or return `std::bad_alloc`, otherwise you return a valid pointer to the region.

Reading pages on the Linux kernel always happen on page-size multiples. The page
size is dependent on the filesystem. *smf* uses seastar for it's IO 
engine among other things. When using O_DIRECT all of the filesystem accesses must be aligned.
Pages for xfs (which is the filesystem in used) are either 512 or 4096. 
Here is a comment seastar's core:

```

seastar/core/file.hh:198

    // O_DIRECT reading requires that buffer, offset, and read length, are
    // all aligned. Alignment of 4096 was necessary in the past, but no longer
    // is - 512 is usually enough; But we'll need to use BLKSSZGET ioctl to
    // be sure it is really enough on this filesystem. 4096 is always safe.
    // In addition, if we start reading in things outside page boundaries,
    // we will end up with various pages around, some of them with
    // overlapping ranges. Those would be very challenging to cache.

```

## Byte Allocation Growth	

The raw strategy for the bump allocator is rather simple. We have 2 bump allocators. 
One per type, at least for the mental model. In practice we have a vector of each.
The reason for having a dynamic size of bump allocators is that we want to grow and
shrink the actual number of bytes allocated, and have the metadata and tracking
information not be too overwhelmingly large. We'll cover the TCP-Like windowing 
where this becomes a key point.

When a user asks us for a new page, we asks what it's underlying alignment is and
assign it to the pool. The pool gets the next free page, bumps the pointer and moves on.

Each page comes from a pool of pages. Each pool is of size 256KB and uses a custom deleter
to eventually mark the entire pool as *evictable* - when all the pages have been allocated, 
and subsequently freed. 

seastar comes with it's own shared pointers (a type of garbage collection strategy 
for pointer graphs in C++). They first is `seastar::shared_ptr<T>` which is a polymorphic 
(you can be casted to a parent shared_ptr<>) smart pointer. The seond is
`seastar::lw_shared_ptr<T>` which is a non-polymorphic type. The type declaration
of these smart pointers come with a custom deleter attribute like the standard containers.

We override this deallocator (since calling `free( ptr )` would crash) to inform 
the bump allocator that a `sizeof(T)` (either 512 bytes or 4096 pages) is free again.
We use this to keep track weather the bump allocator is evictable.

The attentive reader will be quick to point out that there *can* be a situation in which
a user *can* allocate one page per segment (bump allocator size of 256KB), free all but one
preventing the freeing of the bump allocator arena and do that for every single bump allocator
until we run out of memory.

For this specific use case we have major compactions (not implemented in the first relase). 
Minor compactions happen inline with the page allocation calls.


The pseudo code (in C++) would look like this:

```

seastar::file f...;
smf::page_cache_bucket_allocator allocator...;

seastar::lw_shared_ptr<page> p = allocator.alloc(f.dma_align_read_size);

...

// return the bytes to user socket usually


```

## Byte Allocation Eviction Strategy	

There are 3 main hooks for reclaiming memory (of allocators, 
which is different from CLOCK-Pro page eviction) in this design: 

### Inline with a page_fault()

On page fault we run a minor compaction or a reclaim of pages that are
marked as 'evictable':

```
  inline bool is_evictable() const {
    return is_empty() || free_count_ == kAllocatorSize;
  }

```

We only do this if we are exceeding our `max_bytes_` which is typically 60-70%
of the lcore's memory.

We note that we have a reserved min (`min_bytes_`) which is typically 30% of
the lcore's memory

```
 ptr_t alloc(size_t sz) {
    ....

    if (used_bytes() >= max_bytes_) { reclaim(); }

    ......
    
    
 }

```


### When the seastar::allocator is running out of space

This is actually driven by a TCP-Like windowing strategy. 
The high level idea is that you half ( `x/2` ) the difference between 
min and max bytes when you hit pressure, and increase the memory slowly
after some number of *CLOCK-Pro* page_faults!

It is important to note that the raw byte allocator does not drive 
this part of the memory reclamation.

We need more stats like the clock-pro cache-hits, faults, test-period hands (
do we have memory accounted for), etc. 


```

    memory::set_reclaim_hook([this] (std::function<void ()> reclaim_fn) {
        add_high_priority_task(make_task(default_scheduling_group(), [fn = std::move(reclaim_fn)] {
            fn();
        }));
    });

```

### When the seastar::reactor() is idling

Last, when the core's reactor has nothing to do and is about to `sleep()` 
(seastar's future scheduler is very similar to the CFS scheduler form the linux kernel)
and call for major compactions.

This major compaction is not implemented in the first patch.

```
     engine().set_idle_cpu_handler([] (auto check_for_work) {
         
         // run major compactions
         //
         return smf::page_cache_bucket_allocator::get().compact_on_idle(check_for_work);
         
     });

```


# TCP-Like windowing

Windowing for this allocator means trying to saturate
(always operate on the `max_bytes`) memory while allowing for an optimal use of

* background processes
* request-response flows
* writers with a write-behind cache

The intuition behind this 'windowing' is straight forward: when the system
needs memory reduce the window to half between min and max, but don't stay there for too long because the situation my be a short bursts for requests. 

At all times there is always a tradeoff of higher latency when we hit the deallocation strategy (forced compaction) and an optimal use of the hardware.

## Growth Strategy

Not implemented yet. We plan on adapting BBR for an sliding window of the 
last `max_super_pages - min_super_pages` difference, so that the system
is making better use of the memory for the workload of this core.

## Shrink Strategy	

Not implemented yet. We plan on using a BBR-like windowing algorithm
for adapting to the workloads.


# CLOCK-Pro cache

```

struct page {

   enum page_type { hot_page, cold_page };
   uint16_t size; 
   bool ref : 1 = false;
   page_type pt = cold_page;
   uint64_t page_idx = 0;
   uint32_t cold_ref_count = 0; 
   std::unique_ptr<char[]> payload = nullptr;

};

struct hand {
    hand(page_type pointer_type; std::vector<page> &pages): 
         type(pointer_type), 
         reserved (pages) {};
    size_t head_for(page_type ) {
        size_t inverse_index = index_;
        for( uint32_t i = 0u; i < pages.size(); ++i ) { 
               if( inverse_index == 0) { inverse_index = pages.size() - 1; } else { --inverse_index; }
               if ( pages[  inverse_index ].pt == type ) { return inverse_index; }
        }
       __abort();
    }  
    void next_index_for(page_type type) { 
        for( uint32_t i = 0u; i < pages.size(); ++i ) { 
               next_index();
               if ( pages[  index_ ].pt == type ) { return; }
        }
       __abort();
    }
        
   Inline void next_index() { 
            while(true) { if index_ >= pages.size(); index_ = 0; return;} else ++index_; }
   Inline swap_index_and_remove() {
           // remove it!
   std::swap ( pages_[ index_ ] , pages_.rbegin() );
  pages_.pop_back(); 
    }
   inline swap_and_next_index() {
          swap_index_and_remove();
          next_index();
   }
    size_t index = 0;
    std::vector<page> &pages;
};

```         



* m = total memory in page multiples 
* Preallocate 2m metadata
* m_h = hot pages
* m_c = cold pages
* m_m =  pages whose metadata is cached but their data is *not* in memory. 
* m_h + m_c + m_m = m
* hand_hot = points to the last hot page / last accessed page
* hand_cold  = points to the last *resident* cold page - pointer to the next page that will be replaced

## Hand Movement

### hand_cold -> used for resident cold page replacement
If the reference bit (via `hdr.ref == false`) of the hand_cold is unset, we replace this for ‘free space’ ( call delete via `page.pyload = nullptr; ` and keep the header information)

```

// assume page_ = std::unique_ptr<page>(...)
void
ttrun_hand_cold() {
if (p.hdr.ref == false) { 
   // keep page_->hdr 
   // this makes this cold_page  == non-resident cold page
   p->payload = nullptr;
}
```

The replaced cold page will remain in the list as a non-resident cold page until it runs out of its test period, if it is in its test period. If not, we move it out of the clock (for us it means delete the header information too)
if its bit is set (via `hdr.ref == true`) and it is in its test period, we turn the cold page into a hot page and ask hand_hot for its actions (run hand_hot() ). The intuition is that accessing a page during a test period means a  competitive small reuse distance.
If the bit is set and is not in the test period, do nothing.
In both cases the ref bit is set (via `header.ref = true`) and we move the page to the list head ? 
We keep moving hand_cold until we find a page eligible for replacement and we stop at the next cold_page.
hand_hot -> used to evicting old hot pages into cold pages
hand_hot -> is only moved by the actions of the hand_cold. Precisely, it is finding a cold_page being accessed on the test_period. This action can - transitively - move a hot_page to the head/tail? of the cold_page list. 
If the ref bit is unset, we can simply change its status and then move the hand forward
void
run_hand_hot() {
while(true)
            auto &p = pages_[ index_ ];
// while running the hand_hot() and we encounter cold_pages() 
// we piggy back for maintenance of the index
//
if ( p.type == cold_page ) {
// check for test period
//
if( p.type == cold_page && p.payload == nullptr ) { 
   // remove it!
   swan_and_next_index()
   continue;
}
// Whenever the hand encounters a cold page, it will terminate the page's test period.
// The hand will also remove the cold page from the clock if it is non-resident
// (the most probable case).
//
++p.hdr.ref_count;
if ( p.type == cold_page ) p.hdr.ref_count >= pages_.size() && p.payload == nullptr ) { 
   swap_and_next_index();
   continue;
}
}
if ( p.type == hot_page ) {
if ( p.hdr.ref == false ) { 
    p.hdr.pt = cold_page; 
    p.cold_ref_count = 0;
    next_index_for_type();
    break;
}  else if ( hand_hot_->hdr.ref == true) { 
   hand_hot_->hdr.ref = false; 
   hand_hot_;
} 
}
Whenever the hand encounters a cold page, it will terminate the page's test period. The hand will also remove the cold page from the clock if it is non-resident (the most probable case).
----
How these hands fit together:
Cache_t 
// When there is a page fault, the faulted page must be a cold page. We first ask? run_hand_cold() for a free space. If the faulted cold page is not in the list, its reuse distance is highly likely to be larger than the recency of hot pages 4. So the page is still categorized as a cold page and is placed at the list head. The page also initiates its test period. If the number of cold pages is larger than the threshold ($m_c+m$), we run HAND$_{test}$. If the cold page is in the list 5, the faulted page turns into a hot page and is placed at the head of the list. We run HAND$_{hot}$ to turn a hot page with a large recency into a cold page.
Page get(k) {
    If ( hot_.contains(key) ) {
         auto & p = hot_.get(key);
         p.ref = true;
         return p; 
    }
   // else {}
   page_fault(key);
}
Void
page_fault( key ) {
   // do we have enough space ? means are the pages_.size() < capacity()
   //
   // add at head of the list - means insert at specific index (TODO)
   //
   Get the page from filesystem and insert as cold_page;
}
Void 
run_hand_test() {
}
… 
The strategy of this allocator is this: 
We allocate memory slowly - but deallocate quickly  - binary deallocation 
We have a min and max 
Min == 40% of the memory
Max = 2* min = 80%
On memory pressure, we half the pages that are non-resident  and then fallback to hand_test ? then* keep deleting memory until a target. 
On idle-cpu we check if should grow or should shrink .. via timestamps? 


[^1]: (CLOCK-Pro)[ https://www.usenix.org/legacy/publications/library/proceedings/usenix05/tech/general/full_papers/jiang/jiang_html/html.html ] 

[^2]: add the video of kafka used for disaster recovery 

[^3]: add link to kafka being used in IoT by cisco 

[^4]: add link for kafka buffering web requests for web applications 

[^5]: (BBR and other TCP congestion algorithms)[ https://blog.apnic.net/2017/05/09/bbr-new-kid-tcp-block/ ]


---------

Special thanks to:

My friends Chris Heller and Rob Bird gave this strategy the name l0a which I liked =).
In addition Duarte Nunes for giving me the brilliant idea of BBR-like 
(TCP windowing and congestion control) manipulation between min and max.
