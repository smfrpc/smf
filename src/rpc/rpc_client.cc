// Copyright (c) 2016 Alexander Gallego. All rights reserved.
//
#include "rpc/rpc_client.h"
#include <memory>
#include <utility>
// seastar
#include <core/reactor.hh>
#include <net/api.hh>
// smf
#include "platform/log.h"

namespace smf {

rpc_client::rpc_client(seastar::ipv4_addr addr)
  : server_addr(std::move(addr)) {}

seastar::future<> rpc_client::stop() {
  if (conn_) {
    return conn_->istream.close().then([this] {
      return conn_->ostream.flush().then([this]() {
        return conn_->ostream.close().finally([this] {
          conn_->socket.shutdown_output();
          conn_->socket.shutdown_input();
        });
      });
    });
  }
  return seastar::make_ready_future<>();
}
rpc_client::~rpc_client() {}

void rpc_client::enable_histogram_metrics(bool enable) {
  if (enable) {
    if (!hist_) {
      hist_ = std::make_unique<histogram>();
    }
  } else {
    hist_ = nullptr;
  }
}

seastar::future<> rpc_client::connect() {
  LOG_THROW_IF(
    conn_,
    "Client already connected to server: `{}'. connect called more than once.",
    server_addr);

  auto local = seastar::socket_address(sockaddr_in{AF_INET, INADDR_ANY, {0}});
  return seastar::engine()
    .net()
    .connect(seastar::make_ipv4_address(server_addr), local,
             seastar::transport::TCP)
    .then([this](seastar::connected_socket fd) mutable {
      conn_ = std::make_unique<rpc_connection>(std::move(fd));
    });
}
}  // namespace smf
