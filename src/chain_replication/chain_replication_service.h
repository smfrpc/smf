// Copyright (c) 2016 Alexander Gallego. All rights reserved.
//
#pragma once

#include "filesystem/wal.h"
#include "flatbuffers/chain_replication.smf.fb.h"

namespace smf {
namespace chains {
class chain_replication_service : public chains::chain_replication {
 public:
  explicit chain_replication_service(distributed<smf::write_ahead_log> *w)
    : wal_(w) {}
  using env_t = future<smf::rpc_envelope>;
  env_t put(smf::rpc_recv_typed_context<tx_put_request> &&) final;
  env_t get(smf::rpc_recv_typed_context<tx_get_request> &&) final;

 protected:
  distributed<smf::write_ahead_log> *wal_;
};

}  // end namespace chains
}  // end namespace smf
