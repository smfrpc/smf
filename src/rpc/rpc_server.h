// Copyright (c) 2016 Alexander Gallego. All rights reserved.
//
#pragma once
// std
#include <algorithm>
#include <type_traits>
// seastar
#include <core/distributed.hh>
#include <core/metrics_registration.hh>
#include <http/httpd.hh>
// smf
#include "histogram/histogram.h"
#include "platform/macros.h"
#include "rpc/filters/zstd_filter.h"
#include "rpc/rpc_connection_limits.h"
#include "rpc/rpc_filter.h"
#include "rpc/rpc_handle_router.h"
#include "rpc/rpc_server_connection.h"
#include "rpc/rpc_server_stats.h"

namespace smf {

enum rpc_server_flags : uint32_t { rpc_server_flags_disable_http_server = 1 };

struct rpc_server_args {
  uint16_t rpc_port  = 11225;
  uint16_t http_port = 33140;
  uint32_t flags     = 0;
};

class rpc_server {
 public:
  explicit rpc_server(rpc_server_args args);
  ~rpc_server();

  void              start();
  seastar::future<> stop();

  seastar::future<smf::histogram> copy_histogram() {
    smf::histogram h(hist_->get());
    return seastar::make_ready_future<smf::histogram>(std::move(h));
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
  seastar::future<> handle_client_connection(
    seastar::lw_shared_ptr<rpc_server_connection> conn);
  seastar::future<> dispatch_rpc(
    seastar::lw_shared_ptr<rpc_server_connection> conn, rpc_recv_context &&ctx);

 private:
  const rpc_server_args args_;

  seastar::lw_shared_ptr<seastar::server_socket> listener_;
  rpc_handle_router                              routes_;

  using in_filter_t =
    std::function<seastar::future<rpc_recv_context>(rpc_recv_context)>;
  std::vector<in_filter_t> in_filters_;
  using out_filter_t =
    std::function<seastar::future<rpc_envelope>(rpc_envelope)>;
  std::vector<out_filter_t> out_filters_;

  std::unique_ptr<histogram> hist_ = std::make_unique<histogram>();
  uint32_t                   flags_;

  std::unique_ptr<rpc_connection_limits> limits_ =
    std::make_unique<rpc_connection_limits>();

  seastar::lw_shared_ptr<seastar::http_server> admin_ = nullptr;

  seastar::metrics::metric_groups metrics_{};
  rpc_server_stats                stats_;

 private:
  friend std::ostream &operator<<(std::ostream &, const smf::rpc_server &);
};

}  // namespace smf
