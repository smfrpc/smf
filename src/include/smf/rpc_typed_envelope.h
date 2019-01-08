// Copyright (c) 2017 Alexander Gallego. All rights reserved.
//
#pragma once

#include "smf/macros.h"
#include "smf/native_type_utils.h"
#include "smf/rpc_envelope.h"
#include "smf/rpc_header_utils.h"

namespace smf {
template <typename RootType>
struct rpc_typed_envelope {
  using type = RootType;
  using native_type = typename RootType::NativeTableType;

  rpc_envelope envelope;
  std::unique_ptr<native_type> data;

  rpc_typed_envelope() : data(std::make_unique<native_type>()) {}
  ~rpc_typed_envelope() {}
  explicit rpc_typed_envelope(std::unique_ptr<native_type> &&ptr) noexcept
    : data(std::move(ptr)) {}
  rpc_typed_envelope &
  operator=(rpc_typed_envelope<RootType> &&te) noexcept {
    envelope = std::move(te.envelope);
    data = std::move(te.data);
    return *this;
  }
  rpc_typed_envelope(rpc_typed_envelope<RootType> &&te) noexcept {
    *this = std::move(te);
  }
  /// \brief this copies this->data into this->envelope
  /// and retuns a *moved* copy of the envelope. That is
  /// the envelope will be invalid after this method call
  rpc_envelope &&
  serialize_data() {
    envelope.letter.body =
      std::move(smf::native_table_as_buffer<RootType>(*(data.get())));
    smf::checksum_rpc(envelope.letter.header, envelope.letter.body.get(),
                      envelope.letter.body.size());
    data = nullptr;
    return std::move(envelope);
  }
  /// \brief copy ctor deleted
  SMF_DISALLOW_COPY_AND_ASSIGN(rpc_typed_envelope);
};
}  // namespace smf
