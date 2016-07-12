#include "rpc/rpc_client.h"
// seastar
#include <core/reactor.hh>
#include <net/api.hh>
// smf
#include "log.h"

namespace smf {
using namespace std::experimental;
rpc_client::rpc_client(ipv4_addr server_addr)
  : server_addr_(std::move(server_addr)) {}

future<> rpc_client::stop() { return make_ready_future(); }
rpc_client::~rpc_client() {
  if(conn_) {
    delete conn_;
  }
}

void rpc_client::enable_histogram_metrics(bool enable) {
  if(enable) {
    if(!hist_) {
      hist_ = std::make_unique<histogram>();
    }
  } else {
    hist_ = nullptr;
  }
}

future<> rpc_client::connect() {
  if(conn_ != nullptr) {
    return make_ready_future<>();
  }
  socket_address local =
    socket_address(::sockaddr_in{AF_INET, INADDR_ANY, {0}});
  return engine()
    .net()
    .connect(make_ipv4_address(server_addr_), local, seastar::transport::TCP)
    .then([this](connected_socket fd) mutable {
      conn_ = new rpc_client_connection(std::move(fd));
    });
}
} // namespace
