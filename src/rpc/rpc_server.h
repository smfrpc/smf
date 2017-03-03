// Copyright (c) 2016 Alexander Gallego. All rights reserved.
//
#pragma once
// std
#include <algorithm>
#include <type_traits>
// seastar
#include <core/distributed.hh>
// smf
#include "histogram/histogram.h"
#include "platform/macros.h"
#include "rpc/filters/zstd_filter.h"
#include "rpc/rpc_connection_limits.h"
#include "rpc/rpc_filter.h"
#include "rpc/rpc_handle_router.h"
#include "rpc/rpc_server_connection.h"

namespace smf {

// XXX(agallego)- add lineage failures to the connection parsing
enum rpc_server_flags : uint32_t { rpc_server_flags_none = 0 };

class rpc_server {
 public:
  rpc_server(distributed<rpc_server_stats> *stats,
             uint16_t                       port,
             uint32_t                       flags);
  ~rpc_server();

  void     start();
  future<> stop();

  future<smf::histogram> copy_histogram() {
    smf::histogram h(hist_->get());
    return make_ready_future<smf::histogram>(std::move(h));
  }

  template <typename T, typename... Args>
  void register_service(Args &&... args) {
    static_assert(std::is_base_of<rpc_service, T>::value,
                  "register_service can only be called with a derived class of "
                  "smf::rpc_service");
    routes_.register_service(std::make_unique<T>(std::forward<Args>(args)...));
  }
  template <typename Function, typename... Args>
  void register_incoming_filter(Args &&... args) {
    in_filters_.push_back(Function(std::forward<Args>(args)...));
  }


  template <typename Function, typename... Args>
  void register_outgoing_filter(Args &&... args) {
    out_filters_.push_back(Function(std::forward<Args>(args)...));
  }

  SMF_DISALLOW_COPY_AND_ASSIGN(rpc_server);

 private:
  future<> handle_client_connection(lw_shared_ptr<rpc_server_connection> conn);
  future<> dispatch_rpc(lw_shared_ptr<rpc_server_connection> conn,
                        rpc_recv_context &&                  ctx);

 private:
  lw_shared_ptr<server_socket>   listener_;
  distributed<rpc_server_stats> *stats_;
  const uint16_t                 port_;
  rpc_handle_router              routes_;

  using in_filter_t = std::function<future<rpc_recv_context>(rpc_recv_context)>;
  std::vector<in_filter_t> in_filters_;
  using out_filter_t = std::function<future<rpc_envelope>(rpc_envelope)>;
  std::vector<out_filter_t> out_filters_;

  std::unique_ptr<histogram> hist_ = std::make_unique<histogram>();
  uint32_t                   flags_;

  std::unique_ptr<rpc_connection_limits> limits_ =
    std::make_unique<rpc_connection_limits>();

 private:
  friend std::ostream &operator<<(std::ostream &, const smf::rpc_server &);
};

}  // namespace smf
