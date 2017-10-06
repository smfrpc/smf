// Copyright (c) 2016 Alexander Gallego. All rights reserved.
//
#pragma once
#include <utility>
// seastar
#include <core/iostream.hh>
#include <core/shared_ptr.hh>
#include <net/api.hh>

#include "platform/macros.h"
#include "rpc/rpc_connection_limits.h"

namespace smf {

class rpc_connection {
 public:
  explicit rpc_connection(
    seastar::connected_socket                     fd,
    seastar::lw_shared_ptr<rpc_connection_limits> conn_limits = nullptr)
    : socket(std::move(fd))
    , istream(socket.input())
    , ostream(socket.output())
    , limits(conn_limits) {}

  seastar::connected_socket                     socket;
  seastar::input_stream<char>                   istream;
  seastar::output_stream<char>                  ostream;
  seastar::lw_shared_ptr<rpc_connection_limits> limits;
  uint32_t                                      istream_active_parser{0};

  void disable() { enabled_ = false; }
  bool is_enabled() const { return enabled_; }
  bool is_valid() { return !istream.eof() && !has_error() && enabled_; }
  bool has_error() const { return error_.operator bool(); }
  void set_error(const char *e) { error_ = seastar::sstring(e); }

  virtual seastar::sstring get_error() const { return error_.value(); }

  virtual ~rpc_connection() {}

  SMF_DISALLOW_COPY_AND_ASSIGN(rpc_connection);

 private:
  std::experimental::optional<seastar::sstring> error_;
  bool                                          enabled_{true};
};
}  // namespace smf
