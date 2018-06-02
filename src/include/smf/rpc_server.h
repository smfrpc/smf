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
#include "smf/rpc_server_connection.h"
#include "smf/rpc_server_stats.h"
#include "smf/zstd_filter.h"

namespace smf {
enum rpc_server_flags : uint32_t { rpc_server_flags_disable_http_server = 1 };

struct rpc_server_args {
  seastar::sstring ip = "";
  uint16_t rpc_port = 11225;
  uint16_t http_port = 33140;

  /// \brief rpc_server_flags are bitwise flags.
  ///
  uint32_t flags = 0;


  /// \brief basic number of bytes for incoming RPC request.
  /// used to block when out of memory
  ///
  uint64_t basic_req_size = 256;
  /// \ brief  used to compute memory requirement formula.
  /// `req_mem = basic_request_size + sizeof(serialized_request) *
  /// bloat_factor`
  ///
  double bloat_mult = 1.57;
  /// \ brief The default timeout PER connection body. After we parse the
  /// header of the connection we need to make sure that we at some point
  /// receive some bytes or expire the connection.
  ///
  typename seastar::timer<>::duration recv_timeout = std::chrono::minutes(1);
  /// \brief 4GB usually. After this limit, each connection to this
  /// server-core will block until there are enough bytes free in memory to
  /// continue
  ///
  uint64_t memory_avail_per_core = uint64_t(1) << 31 /*2GB per core*/;
};


class rpc_server {
 public:
  explicit rpc_server(rpc_server_args args);
  ~rpc_server();

  void start();
  seastar::future<> stop();

  seastar::future<std::unique_ptr<smf::histogram>>
  copy_histogram() {
    auto h = smf::histogram::make_unique(hist_->get());
    return seastar::make_ready_future<std::unique_ptr<smf::histogram>>(
      std::move(h));
  }

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
  seastar::future<> handle_client_connection(
    seastar::lw_shared_ptr<rpc_server_connection> conn);

  seastar::future<> dispatch_rpc(
    seastar::lw_shared_ptr<rpc_server_connection> conn, rpc_recv_context &&ctx);

  // SEDA piplines
  seastar::future<rpc_recv_context> stage_apply_incoming_filters(
    rpc_recv_context);
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
