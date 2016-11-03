#pragma once
#include "flatbuffers/chain_replication.smf.fb.h"
namespace smf {
namespace chains {
class chain_replication_service : public chains::chain_replication {
  public:
  virtual future<smf::rpc_envelope>
  mput(smf::rpc_recv_typed_context<tx_multiput> &&puts) override final;
  virtual future<smf::rpc_envelope>
  sput(smf::rpc_recv_typed_context<tx_put> &&put) override final;
  virtual future<smf::rpc_envelope>
  fetch(smf::rpc_recv_typed_context<tx_fetch_request> &&rec) override final;
  // need an api for managing configuration updates
  // need an api for managing ip mappings of child
  protected:
};


} // end namespace chains
} // end namespace smf
