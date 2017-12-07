// Copyright 2018 Alexander Gallego
//

#pragma once

#include <memory>

#include "filesystem/page_cache_bucket_allocator.h"
#include "utils/stdx.h"

namespace smf {

using byte_alloc_page_ptr = typename page_cache_bucket_allocator::page_ptr_t;
/// \brief only for external consumption
struct user_page {
  user_page(std::size_t read_size, byte_alloc_page_ptr pg)
    : size(read_size), pg_(pg) {}
  user_page(const user_page &) = default;
  user_page(user_page &&)      = default;
  ~user_page()                 = default;

  const std::size_t size;

  const char *
  data() const {
    return pg_->get();
  }

 private:
  byte_alloc_page_ptr pg_;
};


/// Paper: https://goo.gl/3mSJ1d
/// Presentation: http://fr.slideshare.net/huliang64/clockpro
///
///
class clock_pro {
 public:
  struct page {
    enum page_type : uint8_t { hot_page, cold_page };

    page(uint64_t _idx, uint16_t sz, byte_alloc_page_ptr p)
      : idx(_idx), fs_read_size(sz), payload(p) {}

    bool        ref{false};
    page_type   pt{page_type::cold_page};
    std::size_t cold_ref_count{0};

    const uint64_t idx;
    const uint16_t fs_read_size;

    byte_alloc_page_ptr payload;
  };

  explicit clock_pro(uint32_t page_num) : clock_(page_num){};


  stdx::optional<user_page>
  get(uint64_t idx) {
    auto it = lookup_.find(idx);
    if (it == lookup_.end()) { return stdx::nullopt; }
    // update page metadata
    it->second->ref = true;
    return user_page(it->second->fs_read_size, it->second->payload);
  }

  void
  add_page(uint64_t idx, uint16_t fs_read_size, byte_alloc_page_ptr p) {
    clock_.emplace_back(std::make_unique<page>(idx, fs_read_size, p));
    lookup_[idx] = clock_.rbegin()->get();
  }

  auto
  clock_size() const {
    return clock_.size();
  }

  void
  run_hands() {
    run_hand_cold();
    run_hand_test();
    run_hand_hot();
  }

  /// \brief a) promotes cold pages to hot pages, b) remove non-resident pages!
  void
  run_hand_test() {
    //  b) remove non-resident pages and find the next slot of test_hand
    //
    bool removed = false;
    for (std::size_t i = 0u, index = test_idx_, max_idx = clock_.size() - 1;
         i < clock_.size(); ++i) {
      ++index;
      if (index > max_idx) { index = 0; }
      auto &p = clock_[index];
      if (p->pt == page::page_type::cold_page && !p->payload) {
        if (!removed) {
          removed = true;
          remove_nonresident_page(index);
        } else if (removed) {
          test_idx_ = index;
          break;
        }
      }
    }
  }

  void
  run_hand_cold() {
    auto &p = clock_[cold_idx_];
    if (p->pt != page::page_type::cold_page) {
      LOG_ERROR("Inconsistent indexing. run_hand_cold() should be pointing to "
                "valid cold page. idx: ",
                cold_idx_);
      next(page::page_type::cold_page);
    }

    if (p->ref == false) {
      // this makes this cold_page  == non-resident cold page
      clock_[cold_idx_]->payload = nullptr;
    } else if (p->ref == true) {
      p->pt = page::page_type::hot_page;
      // downgrade the ref bit
      p->ref = false;
    }

    next(page::page_type::cold_page);
  }

  /// \brief find hot pages and demote them into cold pages
  /// also tick the cold_ref_count
  void
  run_hand_hot() {
    for (std::size_t i = 0u, iter = hot_idx_; i < clock_.size(); ++iter) {
      if (iter >= clock_.size() /*must be dynamic, do not cache this value*/) {
        iter = 0;
      }
      auto &p = clock_[iter];
      // while running the hand_hot() and we encounter cold_pages()
      // we piggy back for maintenance of the index
      //
      if (p->pt == page::page_type::cold_page) {
        // check for test period
        //
        if (!p->payload) {
          // remove it!
          remove_nonresident_page(iter);
          continue;
        }
        // Whenever the hand encounters a cold page, it will terminate the
        // page's test period. The hand will also remove the cold page from
        // the clock if it is non-resident
        //
        ++p->cold_ref_count;
        if (p->cold_ref_count >= clock_.size() && !p->payload) {
          remove_nonresident_page(iter);
          continue;
        }
      } else if (p->pt == page::page_type::hot_page) {
        if (p->pt == false) {
          p->pt             = page::page_type::cold_page;
          p->cold_ref_count = 0;
          next(page::page_type::hot_page);
          return;
        } else if (p->ref == true) {
          p->ref = false;
        }
      }

      // we didn't find anything to do w/ the page
      // we didn't need to skip any loop
      // next
      ++i;
    }
  }


 private:
  void
  next(page::page_type ptype) {
    auto index = ptype == page::page_type::cold_page ? cold_idx_ : hot_idx_;

    for (std::size_t i = 0u, max_idx = clock_.size() - 1; i < clock_.size();
         ++i) {
      ++index;
      if (index > max_idx) { index = 0; }
      if (clock_[index]->pt == ptype) {
        switch (ptype) {
        case page::page_type::cold_page:
          cold_idx_ = index;
          return;
        case page::page_type::hot_page:
          hot_idx_ = index;
          return;
        }
      }
    }
  }
  void
  remove_nonresident_page(size_t idx) {
    auto &c = clock_;
    DLOG_THROW_IF(c[idx]->pt != page::page_type::cold_page,
                  "Tried to evict a non-resident page that was hot page?. "
                  "idx:{}, lookup sz: {}, clock sz: {}",
                  idx, lookup_.size(), clock_.size());

    // XXX - improve perf
    // suboptimal - but we could* give one page a lot more life (one extra
    // round of test in worst case) if we do this

    auto &ptr  = c[idx];
    auto &last = c[c.size() - 1];
    DLOG_THROW_IF(ptr->payload, "Can only delete non-resident cold pages");
    if (last.get() != ptr.get()) { std::swap(ptr, last); }
    clock_.pop_back();
    lookup_.erase(ptr->idx);

    // in case we just invalidated the index
    if (hot_idx_ >= clock_.size()) { next(page::page_type::hot_page); }
  }


  std::size_t
  head_idx_for(page::page_type ptype) const {
    auto inverse_index =
      ptype == page::page_type::cold_page ? cold_idx_ : hot_idx_;
    auto const sz = clock_.size();

    for (auto i = 0u; i < sz; ++i) {
      if (inverse_index <= 0) {
        inverse_index = sz - 1;
      } else {
        --inverse_index;
      }
      if (clock_[inverse_index]->pt == ptype) { return inverse_index; }
    }
    return inverse_index;
  }


 private:
  std::size_t cold_idx_{0};
  std::size_t hot_idx_{0};
  std::size_t test_idx_{0};

  /// \brief clock_ and lookup_ *MUST* be kept in sync manually.
  ///
  std::vector<std::unique_ptr<page>> clock_;
  /// \brief faster lookup vs searching the clock_
  /// when it gets larger.
  ///
  /// contains a mapping of page->page_idx to page*
  /// This class *must* keep this lookup_ map
  /// and the clock_ vector in sync
  smf::flat_hash_map<uint64_t, page *> lookup_{};
};

}  // namespace smf
