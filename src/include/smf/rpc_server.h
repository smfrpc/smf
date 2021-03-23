// Copyright (c) 2016 Alexander Gallego. All rights reserved.
//
#pragma once

#include <algorithm>
#include <type_traits>
#include <unordered_map>
#include <optional>

#include <seastar/core/distributed.hh>
#include <seastar/core/gate.hh>
#include <seastar/core/metrics_registration.hh>
#include <seastar/core/timer.hh>
#include <seastar/http/httpd.hh>

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

  /// \brief filter type to process data *before* it hits main handle
  using in_filter_t =
    std::function<seastar::future<rpc_recv_context>(rpc_recv_context)>;

  /// \brief filter type for sending data back out to clients
  using out_filter_t =
    std::function<seastar::future<rpc_envelope>(rpc_envelope)>;

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
  handle_one_client_session(seastar::lw_shared_ptr<rpc_server_connection> conn);

  seastar::future<>
  dispatch_rpc(int32_t payload_size,
               seastar::lw_shared_ptr<rpc_server_connection> conn,
               std::optional<rpc_recv_context> ctx);

  /// \brief main difference between dispatch_rpc and do_dispatch_rpc
  /// is that the former just wraps the calls in a safe seastar::gate
  seastar::future<>
  do_dispatch_rpc(seastar::lw_shared_ptr<rpc_server_connection> conn,
                  rpc_recv_context &&ctx);

  seastar::future<>
  cleanup_dispatch_rpc(seastar::lw_shared_ptr<rpc_server_connection> conn);

  // SEDA piplines
  seastar::future<rpc_recv_context>
    stage_apply_incoming_filters(rpc_recv_context);

  seastar::future<rpc_envelope> stage_apply_outgoing_filters(rpc_envelope);

 private:
  const rpc_server_args args_;
  seastar::lw_shared_ptr<rpc_connection_limits> limits_;
  // -- the actual requests go through these
  rpc_handle_router routes_;
  std::vector<in_filter_t> in_filters_;
  std::vector<out_filter_t> out_filters_;
  // -- http & rpc sockets
  seastar::lw_shared_ptr<seastar::server_socket> listener_;
  seastar::lw_shared_ptr<seastar::http_server> admin_ = nullptr;
  // connection counting happens in different future
  // must survive this instance
  seastar::lw_shared_ptr<rpc_server_stats> stats_ =
    seastar::make_lw_shared<rpc_server_stats>();

  /// \brief keeps latency measurements per request flow
  seastar::lw_shared_ptr<histogram> hist_ = histogram::make_lw_shared();

  // this is needed for shutdown procedures
  uint64_t connection_idx_{0};
  std::unordered_map<uint64_t, seastar::lw_shared_ptr<rpc_server_connection>>
    open_connections_;

  /// \brief set once keep_listening has exited
  seastar::promise<> stopped_;
  /// \brief keeps server alive until all continuations have finished
  seastar::gate reply_gate_;
  /// \brief prometheus metrics exposed
  seastar::metrics::metric_groups metrics_{};
  /// \brief tls credentials
  seastar::shared_ptr<seastar::tls::server_credentials> creds_;

 private:
  friend std::ostream &operator<<(std::ostream &, const smf::rpc_server &);
};

}  // namespace smf
