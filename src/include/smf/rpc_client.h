// Copyright (c) 2016 Alexander Gallego. All rights reserved.
//
#pragma once
// std
#include <memory>
#include <utility>
#include <vector>
#include <optional>
#include <map>

#include <seastar/core/gate.hh>
#include <seastar/net/tls.hh>
#include <seastar/core/shared_ptr.hh>
#include "smf/histogram.h"
#include "smf/macros.h"
#include "smf/rpc_connection.h"
#include "smf/rpc_envelope.h"
#include "smf/rpc_filter.h"
#include "smf/rpc_recv_typed_context.h"

namespace smf {

struct rpc_client_opts {
  seastar::ipv4_addr server_addr;
  /// \ brief rpc client tls trust certficate
  ///
  seastar::shared_ptr<seastar::tls::certificate_credentials> credentials;
  /// \ brief The default timeout PER connection body. After we
  /// parse the header of the connection we need to
  /// make sure that we at some point receive some
  /// bytes or expire the connection. Effectively infinite
  typename seastar::timer<>::duration recv_timeout = std::chrono::minutes(1);
  /// \brief 1GB. After this limit, each connection
  /// will block until there are enough bytes free in memory to continue
  uint64_t memory_avail_for_client = uint64_t(1) << 30 /*1GB*/;
};

/// \brief class intented for communicating with a remote host
///        the intended use case is single threaded, callback driven
///
/// Note: This class uses seastar::SEDA pipelines to process filters.
///       That is, it will effectively execute ALL filters (either incoming or
///       outgoing) as 'one' future. In seastar terms this means blocking.
///
///       We are trading off throughput for latency. So write filters
///       efficiently
///
///       This class after connect() initiates a background future<> that will
///       do the dispatching of the future *once* it receives the data from the
///       socket that it is continually polling.
///       We maintain a set of futures and sessions associated with responses
///       and will full fill them as they come in, out of order even!
///       This is coupled w/ a very specific server impl of processing one
///       request at a time per fiber
///
/// Performance: We currently run at 26micros 100%-tile latency w/ a DPDK
/// runtime
///       With the posix stack we run about 180 micros w/ a client at posix and
///       server
///       running w/ dpdk. While both running on regular posix we run about
///       14ms.
///       Our p999 is 7 micros on the dpdk runtime and about 4ms w/ posix.
///
class rpc_client {
 public:
  struct work_item {
    using promise_t = seastar::promise<std::optional<rpc_recv_context>>;
    explicit work_item(uint16_t idx) : session(idx) {}
    ~work_item() {}

    SMF_DISALLOW_COPY_AND_ASSIGN(work_item);

    promise_t pr;
    uint16_t session{0};
  };

  using in_filter_t =
    std::function<seastar::future<rpc_recv_context>(rpc_recv_context)>;
  using out_filter_t =
    std::function<seastar::future<rpc_envelope>(rpc_envelope)>;

 public:
  explicit rpc_client(seastar::ipv4_addr server_addr);
  explicit rpc_client(rpc_client_opts opts);
  rpc_client(rpc_client &&) noexcept;

  virtual const char *name() const = 0;

  /// \brief actually does the send to the remote location
  /// \param req - the bytes to send
  ///
  template <typename T>
  seastar::future<rpc_recv_typed_context<T>>
  send(rpc_envelope e) {
    using ret_type = rpc_recv_typed_context<T>;
    return seastar::with_gate(
      *dispatch_gate_, [this, e = std::move(e)]() mutable {
        return raw_send(std::move(e)).then([](auto opt_ctx) {
          return seastar::make_ready_future<ret_type>(std::move(opt_ctx));
        });
      });
  }

  virtual seastar::future<> connect() final;
  /// \brief if connection is open, it will
  /// 1. conn->disable()
  /// 2. fulfill all futures with exceptions
  /// 3. stop() the connection
  /// 4. connect()
  virtual seastar::future<> reconnect() final;
  virtual seastar::future<> stop() final;

  virtual ~rpc_client();

  virtual void disable_histogram_metrics() final;
  virtual void enable_histogram_metrics() final;

  SMF_ALWAYS_INLINE virtual bool
  is_histogram_enabled() const final {
    return !!hist_;
  }

  SMF_ALWAYS_INLINE virtual seastar::lw_shared_ptr<histogram>
  get_histogram() final {
    return hist_;
  }

  /// \brief use to enqueue or dequeue filters
  /// \code{.cpp}
  ///    client->incoming_filters().push_back(zstd_decompression_filter());
  /// \endcode
  SMF_ALWAYS_INLINE virtual std::vector<in_filter_t> &
  incoming_filters() final {
    return in_filters_;
  }
  /// \brief use to enqueue or dequeue filters
  /// \code{.cpp}
  ///    client->outgoing_filters().push_back(zstd_compression_filter(1000));
  /// \endcode
  SMF_ALWAYS_INLINE virtual std::vector<out_filter_t> &
  outgoing_filters() final {
    return out_filters_;
  }
  SMF_ALWAYS_INLINE virtual bool
  is_conn_valid() const final {
    return conn_ && conn_->is_valid();
  }

  SMF_DISALLOW_COPY_AND_ASSIGN(rpc_client);

 public:
  const seastar::ipv4_addr server_addr;

  // public for the stage pipelines
  seastar::future<rpc_recv_context> apply_incoming_filters(rpc_recv_context);
  seastar::future<rpc_envelope> apply_outgoing_filters(rpc_envelope);

 private:
  seastar::future<std::optional<rpc_recv_context>> raw_send(rpc_envelope e);
  seastar::future<> do_reads();
  seastar::future<> dispatch_write(rpc_envelope e);
  seastar::future<> process_one_request();
  void fail_outstanding_futures();
  // stage pipeline applications
  seastar::future<rpc_recv_context> stage_incoming_filters(rpc_recv_context);
  seastar::future<rpc_envelope> stage_outgoing_filters(rpc_envelope);

 private:
  seastar::lw_shared_ptr<rpc_connection_limits> limits_;
  seastar::shared_ptr<seastar::tls::certificate_credentials> creds_;
  // need to be public for parent_shared_from_this()
  uint64_t read_counter_{0};
  seastar::lw_shared_ptr<rpc_connection> conn_;
  std::unordered_map<uint16_t, seastar::lw_shared_ptr<work_item>> rpc_slots_;

  std::vector<in_filter_t> in_filters_;
  std::vector<out_filter_t> out_filters_;

  std::unique_ptr<seastar::gate> dispatch_gate_ = nullptr;
  seastar::semaphore serialize_writes_{1};
  seastar::lw_shared_ptr<histogram> hist_ = nullptr;
  uint16_t session_idx_{0};
};

}  // namespace smf
