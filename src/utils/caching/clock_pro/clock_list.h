// Copyright 2017 Alexander Gallego
//
#pragma once
#include <experimental/optional>

#include <boost/intrusive/options.hpp>
#include <boost/intrusive/set.hpp>
#include <boost/intrusive/unordered_set.hpp>

#include "platform/macros.h"
#include "utils/caching/clock_pro/chunk.h"

namespace smf {
namespace clock_pro_internal {

template <typename Key, typename Value> struct clock_list {
  using chunk_t   = clock_pro_internal::chunk<Key, Value>;
  using list_iter = typename std::list<chunk_t>::iterator;
  struct chunk_key {
    using type = Key;
    const type &operator()(const chunk_t &v) const { return v.key; }
  };
  using intrusive_key = boost::intrusive::key_of_value<chunk_key>;
  using intrusive_map = boost::intrusive::set<chunk_t, intrusive_key>;
  using map_iter      = typename intrusive_map::iterator;

  SMF_DISALLOW_COPY_AND_ASSIGN(clock_list);

  clock_list() { clock_hand = allocated.end(); }

  clock_list(clock_list &&l) noexcept
    : lookup(std::move(l.lookup))
    , allocated(std::move(l.allocated))
    , clock_hand(l.clock_hand) {}
  ~clock_list() { lookup.clear(); }

  /// \brief - push at the back of the allocated list.
  /// update index for fast lookup
  void emplace_back(chunk_t &&c) {
    allocated.emplace_back(std::move(c));
    lookup.insert(allocated.back());
  }

  /// \brief - push at the front of the allocated list.
  /// update index for fast lookup
  void emplace_front(chunk_t &&c) {
    allocated.emplace_front(std::move(c));
    lookup.insert(allocated.front());
  }

  /// \brief - returns true if we have the page in memory
  ///
  bool contains(uint32_t page) const {
    return lookup.find(page) != lookup.end();
  }
  /// \brief - use when you know that you contain the element
  /// check - contains() first
  map_iter get(uint32_t page) { return lookup.find(page); }

  /// \brief - use when you know that you contain the element
  /// check - contains() first
  chunk_t *get_chunk_ptr(uint32_t page) { return lookup.find(page)->self(); }

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
  std::experimental::optional<chunk_t> hand_cold() {
    std::experimental::optional<chunk_t> ret;
    if (allocated.empty()) { return ret; }
    if (clock_hand == allocated.end()) {
      clock_hand = allocated.begin();  // loop the hand
    }
    if (clock_hand->ref == true) {
      chunk_t &&c = *std::move_iterator<decltype(clock_hand)>(clock_hand);
      ret.emplace(std::move(c));
    }

    // Note:
    // if (clock_hand->ref == true)
    // is a redundant check and not needed. The only interesting thing
    // to do is to move a mcold_ to an mhot_ page list. By returning the chunk
    // we tell the caller to move it to the hmot_;
    //
    auto it = clock_hand;
    ++clock_hand;
    lookup.erase(it->key);  // O(log (n))
    allocated.erase(it);    // O(1)
    return ret;
  }

  /// \brief - required for the edge case of the FIRST insertion if init w/
  /// empty list which is our case
  void fix_hand() {
    if (!allocated.empty() && clock_hand == allocated.end()) {
      clock_hand = allocated.begin();  // loop the hand
    }
  }

  /// \brief run hand-hot:
  /// reclaim first met cold page with ref-bit to 0
  /// reclaim make it a mcold page - resident, in memory
  std::experimental::optional<chunk_t> hand_hot() {
    std::experimental::optional<chunk_t> ret;

    if (allocated.empty()) { return ret; }
    for (auto total = allocated.size(); total > 0; --total, ++clock_hand) {
      if (clock_hand == allocated.end()) {
        clock_hand = allocated.begin();  // loop the hand
      }
      if (clock_hand->ref == false) {
        auto it = clock_hand;
        ++clock_hand;
        chunk_t &&c = *std::move_iterator<list_iter>(it);
        ret.emplace(std::move(c));
        lookup.erase(it->key);  // O(log (n))
        allocated.erase(it);    // O(1)
        break;
      } else if (clock_hand->ref == true) {
        clock_hand->ref = false;  // degrade
      }
    }
    return ret;
  }


  intrusive_map      lookup{};     // log(n) look up of allocation
  std::list<chunk_t> allocated{};  // memory pages
  list_iter          clock_hand;   // hand needed for clock cache
};
}  // namespace clock_pro_internal
}  // namespace smf
