#pragma once
// std
#include <type_traits>
#include <algorithm>
// seastar
#include <core/distributed.hh>
// smf
#include "rpc/rpc_server_connection.h"
#include "rpc/rpc_handle_router.h"
#include "rpc/rpc_incoming_filter.h"
#include "rpc/rpc_outgoing_filter.h"
#include "rpc/rpc_connection_limits.h"
#include "histogram.h"

namespace smf {

enum RPCFLAGS : uint32_t {
  RPCFLAGS_NONE = 0,
  RPCFLAGS_PRINT_HISTOGRAM_ON_EXIT = 1
};

class rpc_server {
  public:
  rpc_server(distributed<rpc_server_stats> &stats,
             uint16_t port,
             uint32_t flags);

  void start();
  future<> stop();

  template <typename T> void register_service() {
    static_assert(std::is_base_of<rpc_service, T>::value,
                  "register_service can only be called with a derived class of "
                  "smf::rpc_service");
    routes_.register_service(std::make_unique<T>());
  }
  template <typename T> void register_incoming_filter() {
    static_assert(std::is_base_of<rpc_incoming_filter, T>::value,
                  "can only be called with a derived class of "
                  "smf::rpc_incoming_filter");
    in_filters_.push_back(std::make_unique<T>());
  }
  template <typename T> void register_outgoing_filter() {
    static_assert(std::is_base_of<rpc_outgoing_filter, T>::value,
                  "can only be called with a derived class of "
                  "smf::rpc_outgoing_filter");
    out_filters_.push_back(std::make_unique<T>());
  }

  private:
  future<> handle_client_connection(lw_shared_ptr<rpc_server_connection> conn);
  future<> dispatch_rpc(lw_shared_ptr<rpc_server_connection> conn,
                        rpc_recv_context &&ctx);

  future<rpc_recv_context> apply_incoming_filters(rpc_recv_context &&ctx);
  future<rpc_envelope> apply_outgoing_filters(rpc_envelope &&e);

  private:
  lw_shared_ptr<server_socket> listener_;
  distributed<rpc_server_stats> &stats_;
  const uint16_t port_;
  rpc_handle_router routes_;
  std::vector<std::unique_ptr<rpc_incoming_filter>> in_filters_;
  std::vector<std::unique_ptr<rpc_outgoing_filter>> out_filters_;
  std::unique_ptr<histogram> hist_ = std::make_unique<histogram>();
  uint32_t flags_;
  std::unique_ptr<rpc_connection_limits> limits_ =
    std::make_unique<rpc_connection_limits>();
};

} // namespace smf
