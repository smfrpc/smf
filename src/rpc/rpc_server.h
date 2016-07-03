#pragma once
// std
#include <algorithm>
// seastar
#include <core/distributed.hh>
// smf
#include "rpc/rpc_server_connection.h"
#include "rpc/rpc_handle_router.h"

namespace smf {

class rpc_server {
  public:
  rpc_server(distributed<rpc_server_stats> &stats, uint16_t port);
  void start();
  future<> stop();
  void register_router(std::unique_ptr<rpc_handle_router> r);

  private:
  future<> handle_client_connection(lw_shared_ptr<rpc_server_connection> conn);
  future<> dispatch_rpc(lw_shared_ptr<rpc_server_connection> conn,
                        rpc_recv_context &&ctx);
  rpc_handle_router *find_router(const rpc_recv_context &ctx);


  private:
  lw_shared_ptr<server_socket> listener_;
  distributed<rpc_server_stats> &stats_;
  const uint16_t port_;

  std::vector<std::unique_ptr<rpc_handle_router>> routers_{};
  // missing incoming_filer vector
  // missing outgoing_filter vector
};

} /* namespace memcache */
