// Copyright (c) 2016 Alexander Gallego. All rights reserved.
//
#pragma once
#include "flatbuffers/chain_replication.smf.fb.h"

namespace smf {
namespace chains {
class chain_replication_service : public chains::chain_replication {
 public:
  future<smf::rpc_envelope> mput(
    smf::rpc_recv_typed_context<tx_multiput> &&puts) final;
  future<smf::rpc_envelope> sput(
    smf::rpc_recv_typed_context<tx_put> &&put) final;
  future<smf::rpc_envelope> fetch(
    smf::rpc_recv_typed_context<tx_fetch_request> &&rec) final;
  // need an api for managing configuration updates
  // need an api for managing ip mappings of child
 protected:
};


}  // end namespace chains
}  // end namespace smf
