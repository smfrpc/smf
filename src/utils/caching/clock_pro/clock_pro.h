// Copyright 2017 Alexander Gallego
//
#pragma once
#include <type_traits>

#include "platform/log.h"
#include "platform/macros.h"
#include "utils/caching/clock_pro/clock_list.h"

namespace smf {
/// \brief stats to keep and eye out for usage of hits, cold-to-hot ratios
/// memory movement between both pools and the total number of pages
/// fetched from disk
///
struct clock_pro_stats {
  uint64_t cold_hits{0};
  uint64_t hot_hits{0};
  uint64_t cold_to_hot{0};
  uint64_t hot_to_cold{0};
  uint64_t total_allocation{0};
};

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
template <typename Key, typename Value> class clock_pro_cache {
 public:
  using chunk_t    = clock_pro_internal::chunk<Key, Value>;
  using page_ptr   = typename std::add_pointer<chunk_t>::type;
  using list_t     = clock_pro_internal::clock_list<Key, Value>;
  using list_t_ptr = typename std::add_pointer<list_t>::type;

  explicit clock_pro_cache(uint32_t pages) : max_pages(pages) {}

  clock_pro_cache(clock_pro_cache &&c) noexcept
    : max_pages(std::move(c.max_pages))
    , mcold_(std::move(c.mcold_))
    , mhot_(std::move(c.mhot_)) {}

  const uint32_t         max_pages;
  const size_t           size() const { return mhot_->size() + mcold_->size(); }
  const clock_pro_stats &get_stats() const { return stats_; }

  void run_cold_hand() {
    // it's a miss. relclaim the first met cold page w/ ref bit to 0
    auto maybe_hot = mcold_->hand_cold();

    // Note:
    // next run hand_test() -> no need, we don't use that list
    // This is the main difference of the algorithm.
    // next: reorganize_list() -> no need. self organizing dt.

    // we might need to emplace_front on the mhot_ if the ref == 1
    if (maybe_hot) {
      ++stats_.cold_to_hot;
      mhot_->emplace_front(std::move(maybe_hot.value()));
    }
  }

  void run_hot_hand() {
    auto maybe_cold = mhot_->hand_hot();
    // we might need to emplace_back on the mcold_ if the ref == 0
    if (maybe_cold) {
      ++stats_.hot_to_cold;
      mcold_->emplace_back(std::move(maybe_cold.value()));
    }
  }

  bool contains(Key k) const {
    return mhot_->contains(k) || mcold_->contains(k);
  }
  page_ptr get_chunk_ptr(Key k) {
    if (mhot_->contains(k)) { return mhot_->get_chunk_ptr(k); }
    if (mcold_->contains(k)) { return mcold_->get_chunk_ptr(k); }
    LOG_THROW("Please check clock_pro_cache::contains({}) before getting", k);
  }
  page_ptr get_page(Key k) {
    if (mhot_->contains(k)) {
      ++stats_.hot_hits;
      // it's a hit, just set the reference bit to 1
      auto h = mhot_->get(k);
      h->ref = true;  // replace w/ the roaring bitmap
      return mhot_->get_chunk_ptr(k);
    }
    if (mcold_->contains(k)) {
      ++stats_.cold_hits;
      // it's a hit, just set the reference bit to 1
      auto c = mcold_->get(k);
      c->ref = true;
      return mcold_->get_chunk_ptr(k);
    }
    LOG_THROW("Please check clock_pro_cache::contains({}) before getting", k);
  }

  void set(chunk_t &&c) { mcold_->emplace_back(std::move(c)); }

  void set(Key k, Value v) {
    LOG_THROW_IF(size() > max_pages, "Trying to allocate memory without "
                                     "claiming cold pages first, current size: "
                                     "{}, max_size:{}",
                 size(), max_pages);
    ++stats_.total_allocation;
    mcold_->emplace_back(chunk_t(std::move(k), std::move(v)));
  }

  void fix_hands() {
    mhot_->fix_hand();
    mcold_->fix_hand();
  }

  void force_add_hot_chunk(chunk_t &&c) {
    LOG_THROW_IF((size() + 1) > max_pages,
                 "Bad allocation. Bypassing max_pages size ({}) of cache. "
                 "Current size: {} + 1",
                 size());
    mhot_->emplace_back(std::move(c));
  }
  void force_add_cold_chunk(chunk_t &&c) {
    LOG_THROW_IF((size() + 1) > max_pages,
                 "Bad allocation. Bypassing max_pages size ({}) of cache. "
                 "Current size: {} + 1",
                 size());
    mcold_->emplace_back(std::move(c));
  }
  const list_t_ptr get_cold_list() { return mcold_.get(); }
  const list_t_ptr get_hot_list() { return mhot_.get(); }

 private:
  std::unique_ptr<list_t> mcold_ = std::make_unique<list_t>();
  std::unique_ptr<list_t> mhot_  = std::make_unique<list_t>();
  clock_pro_stats         stats_{};
  SMF_DISALLOW_COPY_AND_ASSIGN(clock_pro_cache);
};
}  // namespace smf
