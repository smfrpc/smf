#pragma once

#include <core/temporary_buffer.hh>
#include <flatbuffers/flatbuffers.h>

#include "platform/macros.h"


namespace smf {


/// No validation done here. Just a buffer containing a type
///
template <typename T> class fbs_typed_buf {
 public:
  using type = T;
  fbs_typed_buf(seastar::temporary_buffer<char> body) : buf_(std::move(body)) {
    // TODO(agallego) - change to constexpr if
    // when we move to c++17 and gcc7.
    auto ptr = static_cast<void *>(buf_.get_write());
    if (std::is_base_of<flatbuffers::Table, T>::value) {
      cache_ = flatbuffers::GetMutableRoot<T>(ptr);
    } else {
      cache_ = reinterpret_cast<T *>(ptr);
    }
  }
  fbs_typed_buf(fbs_typed_buf &&o) noexcept
    : buf_(std::move(o.buf_)), cache_(std::move(o.cache_)) {}
  inline T *operator->() { return cache_; }
  inline T *get() { return cache_; }
  SMF_DISALLOW_COPY_AND_ASSIGN(fbs_typed_buf);

 private:
  seastar::temporary_buffer<char> buf_;
  T *                             cache_{nullptr};
};

}  // namespace smf
