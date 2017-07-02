// Copyright 2017 Alexander Gallego
//

#pragma once

#include <core/shared_ptr.hh>

#include "flatbuffers/chain_replication.smf.fb.h"

namespace smf {
namespace chains {

// TODO(agallego) use streaming clients when avail. for now use safe* methods

static const uint32_t kMaxPartitions = 1024;

class cr_client : private chain_replication_client,
                  private seastar::enable_lw_shared_from_this<cr_client> {
 public:
  static seastar::lw_shared_ptr<cr_client> make_cr_client(
    seastar::ipv4_addr server_addr);

  seastar::lw_shared_ptr<tx> begin_transaction();

  /*, strategy=round-robin?, exponential-backoff add strategy for polling*/
  seastar::future<chain_get_reply> poll(const seastar::sstring &topic);

 private:
  explicit cr_client(seastar::ipv4_addr server_addr);
  seastar::future<chain_get_reply> do_poll(seastar::string topic);

 private:
  std::unordered_map<seastar::sstring, chain_discover_replyT> read_world_;
};
}  // namespace chains
}  // namespace smf
