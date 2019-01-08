// Copyright 2017 Alexander Gallego
//

#pragma once

#include <flatbuffers/flatbuffers.h>
#include <seastar/core/temporary_buffer.hh>

#include "smf/log.h"
#include "smf/macros.h"

namespace smf {

/// No validation done here. Just a buffer containing a type
///
template <typename T>
class fbs_typed_buf {
 public:
  static_assert(std::is_base_of<flatbuffers::Table, T>::value,
                "Should ONLY be Table derived classes");
  using type = T;
  explicit fbs_typed_buf(seastar::temporary_buffer<char> body)
    : buf_(std::move(body)) {
    DLOG_THROW_IF(buf_.size() == 0, "Empty flatbuffers buffer");
    auto ptr = static_cast<void *>(buf_.get_write());
    cache_ = flatbuffers::GetMutableRoot<T>(ptr);
  }
  fbs_typed_buf(fbs_typed_buf &&o) noexcept
    : buf_(std::move(o.buf_)), cache_(std::move(o.cache_)) {}
  fbs_typed_buf &
  operator=(fbs_typed_buf &&o) {
    buf_ = std::move(o.buf_);
    cache_ = std::move(o.cache_);
    return *this;
  }
  T *operator->() const { return cache_; }
  T &operator*() const { return *cache_; }
  T *
  get() const {
    return cache_;
  }

  // needed to share the payload at specific ranges - i.e.: internal
  // structures for saving to files, creating subranges for
  // nested flatbuffers types,etc.
  //
  seastar::temporary_buffer<char> &
  buf() {
    return buf_;
  }
  seastar::temporary_buffer<char> &&
  move_buf() {
    cache_ = nullptr;
    return std::move(buf_);
  }

  SMF_DISALLOW_COPY_AND_ASSIGN(fbs_typed_buf);

 private:
  seastar::temporary_buffer<char> buf_;
  T *cache_{nullptr};
};

}  // namespace smf
