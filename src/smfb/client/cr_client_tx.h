// Copyright 2017 Alexander Gallego
//

#pragma once

#include <core/shared_ptr.hh>

#include "flatbuffers/chain_replication.smf.fb.h"

namespace smf {
namespace chains {
class cr_client_tx {
 public:
  explicit cr_client_tx(seastar::lw_shared_ptr<cr_client> c);
  ~cr_client_tx();

  void produce(const seastar::sstring &topic,
               const seastar::sstring &key,
               std::vector<uint8_t> && data);


  seastar::future<chain_put_reply> commit();
  seastar::future<chain_put_reply> abort();

 private:
  seastar::lw_shared_ptr<cr_client> client_;
  smf::chains::tx_put_requestT      put;

  // either commit or abort is called
  bool isFinished_ = false;
  // for streaming puts
  bool isStreamingPut_ = false;
};
}  // namespace chains
}  // namespace smf
