// Copyright (c) 2016 Alexander Gallego. All rights reserved.
//
#pragma once

#include <algorithm>
#include <type_traits>

#include <bytell_hash_map.hpp>
#include <core/distributed.hh>
#include <core/metrics_registration.hh>
#include <core/timer.hh>
#include <http/httpd.hh>

#include "smf/histogram.h"
#include "smf/macros.h"
#include "smf/rpc_connection_limits.h"
#include "smf/rpc_filter.h"
#include "smf/rpc_handle_router.h"
#include "smf/rpc_server_args.h"
#include "smf/rpc_server_connection.h"
#include "smf/rpc_server_stats.h"
#include "smf/zstd_filter.h"

namespace smf {

class rpc_server {
 public:
  explicit rpc_server(rpc_server_args args);
  ~rpc_server();

  void start();

  seastar::future<> stop();

  /// \brief copy histogram. Cannot be made const due to seastar::map_reduce
  /// const-ness bugs
  seastar::future<std::unique_ptr<smf::histogram>> copy_histogram();

  template <typename T, typename... Args>
  void
  register_service(Args &&... args) {
    static_assert(std::is_base_of<rpc_service, T>::value,
                  "register_service can only be called with a derived class of "
                  "smf::rpc_service");
    routes_.register_service(std::make_unique<T>(std::forward<Args>(args)...));
  }
  template <typename Function, typename... Args>
  void
  register_incoming_filter(Args &&... args) {
    in_filters_.push_back(Function(std::forward<Args>(args)...));
  }

  template <typename Function, typename... Args>
  void
  register_outgoing_filter(Args &&... args) {
    out_filters_.push_back(Function(std::forward<Args>(args)...));
  }

  SMF_DISALLOW_COPY_AND_ASSIGN(rpc_server);

  seastar::future<rpc_recv_context> apply_incoming_filters(rpc_recv_context);
  seastar::future<rpc_envelope> apply_outgoing_filters(rpc_envelope);

 private:
  seastar::future<>
  handle_client_connection(seastar::lw_shared_ptr<rpc_server_connection> conn);

  seastar::future<>
  dispatch_rpc(seastar::lw_shared_ptr<rpc_server_connection> conn,
               rpc_recv_context &&ctx);

  // SEDA piplines
  seastar::future<rpc_recv_context>
    stage_apply_incoming_filters(rpc_recv_context);
  seastar::future<rpc_envelope> stage_apply_outgoing_filters(rpc_envelope);

 private:
  const rpc_server_args args_;

  seastar::lw_shared_ptr<seastar::server_socket> listener_;
  rpc_handle_router routes_;

  using in_filter_t =
    std::function<seastar::future<rpc_recv_context>(rpc_recv_context)>;
  std::vector<in_filter_t> in_filters_;
  using out_filter_t =
    std::function<seastar::future<rpc_envelope>(rpc_envelope)>;
  std::vector<out_filter_t> out_filters_;

  seastar::lw_shared_ptr<histogram> hist_ = histogram::make_lw_shared();
  seastar::lw_shared_ptr<rpc_connection_limits> limits_ = nullptr;
  seastar::lw_shared_ptr<seastar::http_server> admin_ = nullptr;

  seastar::metrics::metric_groups metrics_{};
  // connection counting happens in different future
  // must survive this instance
  seastar::lw_shared_ptr<rpc_server_stats> stats_ =
    seastar::make_lw_shared<rpc_server_stats>();

  // this is needed for shutdown procedures
  uint64_t connection_idx_{0};
  ska::bytell_hash_map<uint64_t, seastar::lw_shared_ptr<rpc_server_connection>>
    open_connections_;

 private:
  friend std::ostream &operator<<(std::ostream &, const smf::rpc_server &);
};

}  // namespace smf
