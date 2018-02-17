// Copyright 2017 Alexander Gallego
//

#pragma once

#include <cstdint>
#include <list>

#include <core/byteorder.hh>
#include <core/shared_ptr.hh>
#include <core/sstring.hh>

#include "flatbuffers/fbs_typed_buf.h"
#include "flatbuffers/wal_generated.h"
#include "platform/macros.h"

namespace smf {

struct wal_write_projection {
  /// \brief takes in what the user expected to write, and converts it to a
  /// format that will actually be flushed to disk, which may or may not
  /// include compression, encryption, etc.
  ///
  static std::unique_ptr<smf::wal::tx_put_binary_fragmentT> xform(
    const smf::wal::tx_put_fragmentT &f);
};

}  // namespace smf
