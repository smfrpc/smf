// Copyright (c) 2016 Alexander Gallego. All rights reserved.
//
#pragma once
#include <cmath>
#include <list>
#include <utility>
// third party
#include <boost/intrusive/options.hpp>
#include <boost/intrusive/set.hpp>
#include <boost/intrusive/unordered_set.hpp>
#include <core/aligned_buffer.hh>
#include <core/file.hh>
// smf
#include "filesystem/wal_requests.h"
#include "platform/macros.h"

// Paper: https://goo.gl/3mSJ1d
// Presentation: http://fr.slideshare.net/huliang64/clockpro
//
// 4.1 Main Idea
//
// CLOCK-Pro takes the same principle as that of LIRS - it uses the reuse
// distance (called IRR in LIRS) rather than recency in its replacement
// decision. When a page is accessed, the reuse distance is the period of time
// in terms of the number of other distinct pages accessed since its last
// access. Although there is a reuse distance between any two consecutive
// references to a page, only the most current distance is relevant in the
// replacement decision. We use the reuse distance of a page at the time of its
// access to categorize it either as a cold page if it has a large reuse
// distance, or as a hot page if it has a small reuse distance. Then we mark its
// status as being cold or hot. We place all the accessed pages, either hot or
// cold, into one single list 2 in the order of their accesses 3. In the list,
// the pages with small recencies are at the list head, and the pages with large
// recencies are at the list tail.
//
// To give the cold pages a chance to compete with the hot pages and to ensure
// their cold/hot statuses accurately reflect their current access behavior, we
// grant a cold page a test period once it is accepted into the list. Then, if
// it is re-accessed during its test period, the cold page turns into a hot
// page. If the cold page passes the test period without a re-access, it will
// leave the list. Note that the cold page in its test period can be replaced
// out of memory, however, its page metadata remains in the list for the test
// purpose until the end of the test period or being re-accessed. When it is
// necessary to generate a free space, we replace a resident cold page.
//
// The key question here is how to set the time of the test period. When a cold
// page is in the list and there is still at least one hot page after it (i.e.,
// with a larger recency), it should turn into a hot page if it is accessed,
// because it has a new reuse distance smaller than the hot page(s) after it.
// Accordingly, the hot page with the largest recency should turn into a cold
// page. So the test period should be set as the largest recency of the hot
// pages. If we make sure that the hot page with the largest recency is always
// at the list tail, and all the cold pages that pass this hot page terminate
// their test periods, then the test period of a cold page is equal to the time
// before it passes the tail of the list. So all the non-resident cold pages can
// be removed from the list right after they reach the tail of the list. In
// practice, we could shorten the test period and limit the number of cold pages
// in the test period to reduce space cost. By implementing this testing
// mechanism, we make sure that ``cold/hot'' are defined based on relativity and
// by constant comparison in one clock, not on a fixed threshold that are used
// to separate the pages into two lists. This makes CLOCK-Pro distinctive from
// prior work including 2Q and CAR, which attempt to use a constant threshold to
// distinguish the two types of pages, and to treat them differently in their
// respective lists (2Q has two queues, and CAR has two clocks), which
// unfortunately causes these algorithms to share some of LRU's performance
// weakness.
//
// * CLOCK-Pro is an approx of LIRS based on the CLOCK infrastructure (corbato)
//
// * Pages categorized into: cold-pages and hot-pages (based on IRR)
// * Three hands: hand-{cold,hot,test}, see CLOCK infrastructure.
// * Allocation is adaptive. It is relative. Memory usage M =
//       M = (Mhot + Mcold)
//   Mhot = hot memory pages allocation
//   Mcold = cold memory pages allocation
// * ALL hot pages are resident == Mhot - LIR blocks
// * SOME cold pages are resident == Mcold - HIR blocks
// * REST are cold hon-resident - metadata tracked still
//
// * all hands move clockwise
// * hand-hot: find a hot page to demote to cold page
// * hand-test:
//   1) determine if a page is promoted to hot
//   2) remove non-resident cold pages out of the clock
// * hand-cold: ???
//
// * effectively keep 3 deques/lists: hot, cold (resident), cold(rest)
//
// This implementation differs in subtle ways, semantic preserving:
//   1) our page no can be very large, so we do not keep a list of
//   non-resident-cold pages.
//   2) instead of circular lists, we use a list and manually wrap it around
//   achieving the same result. Note that this is different from open impls in
//   that it does not attempt to use a single storage and simply tick the hands
//   of the clocks as it would consume too much memory, even w/ empty slots for
//   the cold-non-resident (clock) hand
//   3) algorithm assumes that you start w/ a working set or circular buffer
//   since we dynamically page things into memory, we add an extra check to add
//   things directly into mhot_ if we have yet to reach our limit of pages to
//   fetch. -> aka: fill up a basic list first before eviction starts.
//   4) the clock::hand_cold returns* the item that needs to be emplace in
//   "front" on the mhot_ list.
//
namespace smf {
class wal_clock_pro_cache {
 public:
  /// \brief stats to keep and eye out for usage of hits, cold-to-hot ratios
  /// memory movement between both pools and the total number of pages
  /// fetched from disk
  ///
  struct stats {
    uint64_t cold_hits{0};
    uint64_t hot_hits{0};
    uint64_t cold_to_hot{0};
    uint64_t hot_to_cold{0};
    uint64_t total_allocation{0};
  };
  /// \brief represents a page. part of the clock-pro caching structure
  /// mostly a container around a bag of bytes. It is an intrusive container
  /// so that allocations and lookups are both optimized.
  /// Lookups:          O( log(n) )
  /// Insertsions:      O( 1 )
  /// Deletions:        O( 1 )
  ///
  using bufptr_t = std::unique_ptr<char[], free_deleter>;
  struct cache_chunk : public boost::intrusive::set_base_hook<>,
                       public boost::intrusive::unordered_set_base_hook<> {
    cache_chunk(uint32_t page_no, uint32_t data_size, bufptr_t _data) noexcept
      : page(page_no),
        buf_size(data_size),
        data(std::move(_data)) {}
    cache_chunk(cache_chunk &&c) noexcept : page(c.page),
                                            buf_size(c.buf_size),
                                            data(std::move(c.data)),
                                            ref(c.ref) {}
    const cache_chunk *self() const { return this; }
    const uint32_t     page;
    const uint32_t     buf_size;
    bufptr_t           data;
    bool               ref{false};
    SMF_DISALLOW_COPY_AND_ASSIGN(cache_chunk);
  };

  /// \brief internal datastructure supporting the boost intrusive, ordered
  /// containers
  ///
  struct cache_chunk_key {
    typedef uint32_t type;
    const type &operator()(const cache_chunk &v) const { return v.page; }
  };

  /// \brief - provides fast lookups
  /// Lookups:          O( log(n) )
  ///
  using intrusive_key = boost::intrusive::key_of_value<cache_chunk_key>;
  using intrusive_map = boost::intrusive::set<cache_chunk, intrusive_key>;
  static_assert(std::is_same<intrusive_map::key_type, uint32_t>::value,
                "bad key for intrusive map");


  struct clock_list {
    clock_list() {}
    ~clock_list() { lookup.clear(); }
    using list_iter = std::list<cache_chunk>::iterator;
    intrusive_map          lookup{};      // log(n) look up of allocation
    std::list<cache_chunk> allocated{};   // memory pages
    list_iter              clock_hand{};  // hand needed for clock cache
    SMF_DISALLOW_COPY_AND_ASSIGN(clock_list);
    clock_list(clock_list &&l) noexcept : lookup(std::move(l.lookup)),
                                          allocated(std::move(l.allocated)),
                                          clock_hand(l.clock_hand) {}

    /// \brief - push at the back of the allocated list.
    /// update index for fast lookup
    void emplace_back(cache_chunk &&c) {
      allocated.emplace_back(std::move(c));
      lookup.insert(allocated.back());
    }
    /// \brief - push at the front of the allocated list.
    /// update index for fast lookup
    void emplace_front(cache_chunk &&c) {
      allocated.emplace_front(std::move(c));
      lookup.insert(allocated.front());
    }
    /// \brief - remove page and update index for fast lookup
    ///
    void remove(uint32_t page) {
      allocated.remove_if([page](auto const &i) { return i.page == page; });
      lookup.erase(page);
    }
    /// \brief - returns true if we have the page in memory
    ///
    bool contains(uint32_t page) const {
      return lookup.find(page) != lookup.end();
    }
    /// \brief - use when you know that you contain the element
    /// check - contains() first
    intrusive_map::iterator get(uint32_t page) { return lookup.find(page); }
    /// \brief - use when you know that you contain the element
    /// check - contains() first
    const cache_chunk *get_cache_chunk_ptr(uint32_t page) {
      const cache_chunk *r = lookup.find(page)->self();
      return r;
    }
    /// \brief - return size in constant time
    ///
    size_t size() const {
      // since c++11 this op is O(1)
      return allocated.size();
    }
    /// \brief run hand-cold:
    /// reclaim first met cold page with ref-bit to 0
    /// reclaim cold page from it and remove resident cold page list
    /// in this implementation, we simply remove it. there is no
    /// cold-non-resident list
    std::experimental::optional<cache_chunk> hand_cold() {
      std::experimental::optional<cache_chunk> ret;
      if (allocated.empty()) {
        return ret;
      }
      if (clock_hand == allocated.end()) {
        clock_hand = allocated.begin();  // loop the hand
      }
      if (clock_hand->ref == true) {
        cache_chunk &&c = *std::move_iterator<decltype(clock_hand)>(clock_hand);
        ret.emplace(std::move(c));
      }

      // Note:
      // if (clock_hand->ref == true)
      // is a redundant check and not needed. The only interesting thing
      // to do is to move a mcold_ to an mhot_ page list. By returning the chunk
      // we tell the caller to move it to the hmot_;
      //

      lookup.erase(clock_hand->page);  // O(log (n))
      allocated.erase(clock_hand);     // O(1)
      ++clock_hand;                    // advance to next position

      return ret;
    }
    /// \brief run hand-hot:
    /// reclaim first met cold page with ref-bit to 0
    /// reclaim make it a mcold page - resident, in memory
    std::experimental::optional<cache_chunk> hand_hot() {
      std::experimental::optional<cache_chunk> ret;
      if (size() == 0) {
        return ret;
      }
      for (auto total = allocated.size(); total > 0; --total, ++clock_hand) {
        if (clock_hand == allocated.end()) {
          clock_hand = allocated.begin();  // loop the hand
        }
        if (clock_hand->ref == false) {
          cache_chunk &&c =
            *std::move_iterator<decltype(clock_hand)>(clock_hand);
          ret.emplace(std::move(c));
          lookup.erase(clock_hand->page);  // O(log (n))
          allocated.erase(clock_hand);     // O(1)
          ++clock_hand;                    // advance to next position
          return ret;
        } else if (clock_hand->ref == true) {
          clock_hand->ref = false;  // degrade
        }
      }
      return ret;
    }
  };

 public:
  /// \brief - designed to be a cache from the direct-io layer
  /// and pages fetched. It understands the wal_writer format
  wal_clock_pro_cache(lw_shared_ptr<file> f,
                      // size of the `f` file from the fstat() call
                      // need this to figure out how many pages to allocate
                      int64_t size,
                      // max_pages_in_memory = 0, allows us to make a decision
                      // impl defined. right now, it chooses the max of 10% of
                      // the file or 10 pages. each page is 4096 bytes
                      uint32_t max_pages_in_memory = 0);

  const int64_t              file_size;
  const uint32_t             number_of_pages;
  wal_clock_pro_cache::stats get_stats() const { return stats_; }
  /// \brief - return buffer for offset with size
  future<wal_read_reply> read(wal_read_request r);
  /// \brief - closes lw_share_ptr<file>
  future<> close();
  /// \brief size of ALL allocated pages in memory
  size_t size() const;

 private:
  /// \brief - returns a temporary buffer of size. similar to
  /// isotream.read_exactly()
  /// different than a wal_read_request for arguments since it returns **one**
  /// temp buffer
  ///
  future<temporary_buffer<char>> read_exactly(int64_t                  offset,
                                              int64_t                  size,
                                              const io_priority_class &pc);
  /// \brief actual reader func
  /// recursive
  future<lw_shared_ptr<wal_read_reply>> do_read(wal_read_request r);

  /// \breif fetch exactly one page from disk w/ dma_alignment() so that
  /// we don't pay the penalty of fetching 2 pages and discarding one
  future<cache_chunk> fetch_page(const uint32_t &         page,
                                 const io_priority_class &pc);

  /// \brief perform the clock-pro caching and eviction techniques
  /// and then return a ptr to the page
  /// intrusive containers::iterators are not default no-throw move
  /// constructible, so we have to get the ptr to the data which is lame
  ///
  future<const cache_chunk *> clock_pro_get_page(uint32_t                 page,
                                                 const io_priority_class &pc);

 private:
  lw_shared_ptr<file> file_;  // uses direct io for fetching
  uint32_t            max_resident_pages_;

  clock_list                 mhot_{};   // hot memory pages, resident
  clock_list                 mcold_{};  // cold memory pages, resident
  wal_clock_pro_cache::stats stats_{};
};


}  // namespace smf
