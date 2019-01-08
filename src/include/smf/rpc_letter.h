// Copyright (c) 2017 Alexander Gallego. All rights reserved.
//
#pragma once

#include <seastar/core/sstring.hh>
#include <seastar/core/temporary_buffer.hh>

#include "smf/macros.h"
#include "smf/rpc_generated.h"

namespace smf {

struct rpc_letter {
  rpc_letter();
  rpc_letter(rpc::header,
             std::unordered_map<seastar::sstring, seastar::sstring>,
             seastar::temporary_buffer<char>);
  rpc_letter &operator=(rpc_letter &&l) noexcept;
  rpc_letter(rpc_letter &&) noexcept;
  ~rpc_letter();
  SMF_DISALLOW_COPY_AND_ASSIGN(rpc_letter);

  /// \brief returns a share'd copy of the body payload
  /// Do not modify the body itself - replace it
  /// and make an explicit copy instead
  ///
  rpc_letter share();
  /// \brief size including headers
  size_t size() const;
  /// \brief does it have a valid body
  bool empty() const;

  rpc::header header;
  std::unordered_map<seastar::sstring, seastar::sstring> dynamic_headers;
  seastar::temporary_buffer<char> body;
};

}  // namespace smf
