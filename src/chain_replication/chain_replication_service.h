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

  virtual future<smf::rpc_typed_envelope<tx_put_reply>> put(
    smf::rpc_recv_typed_context<tx_put_request> &&) final;

  virtual future<smf::rpc_typed_envelope<tx_get_reply>> get(
    smf::rpc_recv_typed_context<tx_get_request> &&) final;

 private:
  distributed<smf::write_ahead_log> *wal_;
};

}  // end namespace chains
}  // end namespace smf
