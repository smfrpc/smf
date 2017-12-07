// Copyright 2017 Alexander Gallego
//

#include "smfb/client/cr_client.h"

#include "smfb/client/client_topic_poll_strategy.h"

namespace smf {
namespace chains {

seastar::lw_shared_ptr<cr_client>
cr_client::make_cr_client(seastar::ipv4_addr server_addr) {
  return seastar::make_lw_shared<cr_client>(
    new cr_client(std::move(server_addr)));
}


seastar::lw_shared_ptr<cr_client_tx>
cr_client::begin_transaction() {
  return seastar::make_lw_shared<tx>(shared_from_this());
}

/*, strategy=round-robin?, exponential-backoff add strategy for polling*/
seastar::future<chain_get_reply>
cr_client::poll(const seastar::sstring &   topic,
                const poll_strategy_start &strategy) {
  if (read_world_.find(topic) == read_world_.end()) {
    chains_discover_requestT r;
    r.topic = topic;
    return safe_discover(
             rpc_envelope(
               rpc_letter::native_table_to_rpc_letter<chain_discover_request>(
                 r)))
      .then([this, topic](auto disc) {
        chain_discover_replyT t_world = disc->watermarks->Unpack();
        read_world_.insert({topic, t_world});
        return do_poll(topic);
      });
  } else {
    return do_poll(topic);
  }
}

cr_client::cr_client(seastar::ipv4_addr server_addr)
  : chain_replication_client(std::move(server_addr)) {}


seastar::future<chain_get_reply>
cr_client::do_poll(seastar::string topic) {}

}  // namespace chains
}  // namespace smf
