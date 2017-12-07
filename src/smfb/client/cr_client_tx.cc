// Copyright 2017 Alexander Gallego
//

#pragma once

#include "smfb/client/cr_client_tx.h"

namespace smf {
namespace chains {
cr_client_tx::cr_client_tx(seastar::lw_shared_ptr<cr_client> c) : client_(c) {}

cr_client_tx::~cr_client_tx() {}

void
cr_client_tx::produce(const seastar::sstring &topic,
                      const seastar::sstring &key,
                      std::vector<uint8_t> && data) {}

seastar::future<chain_put_reply>
cr_client_tx::commit() {
  LOG_THROW_IF(isFinished_, "Transaction already commit()'ed or abort()'ed");
  isFinished_ = true;
  // need to go around and add the 'commit' op w/ empty k=v pairs as the
  // data.
  return rpc_envelope(
    rpc_letter::native_table_to_rpc_letter<tx_put_request>(put));
}
seastar::future<chain_put_reply>
cr_client_tx::abort() {
  LOG_THROW_IF(isFinished_, "Transaction already commit()'ed or abort()'ed");
  isFinished_ = true;
}

}  // namespace chains
}  // namespace smf
