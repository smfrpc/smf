// Copyright (c) 2016 Alexander Gallego. All rights reserved.
//
#pragma once
// std
#include <experimental/optional>
#include <memory>
#include <utility>
#include <vector>
// seastar
#include <core/shared_ptr.hh>
#include <net/api.hh>
// smf
#include "histogram/histogram.h"
#include "platform/macros.h"
#include "rpc/rpc_connection.h"
#include "rpc/rpc_envelope.h"
#include "rpc/rpc_filter.h"
#include "rpc/rpc_recv_typed_context.h"

namespace smf {
namespace stdx = std::experimental;

struct rpc_client_opts {
  seastar::ipv4_addr server_addr;

  /// `req_mem = basic_request_size + sizeof(letter.body) * bloat_factor`
  uint64_t basic_req_bloat_size = 0;
  double   bloat_mult           = 1.0;
  /// \ brief The default timeout PER connection body. After we parse the
  /// header
  /// of the connection we need to make sure that we at some point receive
  /// some
  /// bytes or expire the connection. Effectively infinite
  ///
  typename seastar::timer<>::duration recv_timeout = std::chrono::hours(36);
  /// \brief 2GB usually. After this limit, each connection to this
  /// server-core
  /// will block until there are enough bytes free in memory to continue
  ///
  uint64_t memory_avail_for_client = uint64_t(1) << 31 /*2GB per core*/;
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
/// Performance: We currently run at 26micros 100%-tile latency w/ a DPDK runtime
///       With the posix stack we run about 180 micros w/ a client at posix and server
///       running w/ dpdk. While both running on regular posix we run about 14ms.
///       Our p999 is 7 micros on the dpdk runtime and about 4ms w/ posix.
///
class rpc_client {
 public:
  struct work_item {
    using promise_t = seastar::promise<stdx::optional<rpc_recv_context>>;
    explicit work_item(uint16_t idx) : session(idx) {}
    ~work_item() {}

    SMF_DISALLOW_COPY_AND_ASSIGN(work_item);

    promise_t pr;
    uint16_t  session{0};
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

  virtual seastar::shared_ptr<rpc_client> parent_shared_from_this() = 0;

  /// \brief actually does the send to the remote location
  /// \param req - the bytes to send
  ///
  template <typename T>
  seastar::future<rpc_recv_typed_context<T>> send(rpc_envelope e) {
    LOG_THROW_IF(is_error_state, "Cannot send request in error state");

    using ret_type = rpc_recv_typed_context<T>;
    // create the work item
    ++session_idx_;
    ++read_counter;

    DLOG_THROW_IF(rpc_slots.find(session_idx_) != rpc_slots.end(),
                  "RPC slot already allocated");
    auto work    = seastar::make_lw_shared<work_item>(session_idx_);
    auto measure = is_histogram_enabled() ? hist_->auto_measure() : nullptr;

    rpc_slots.insert({session_idx_, work});
    // critical - without this nothing works
    e.letter.header.mutate_session(session_idx_);

    // apply the first set of outgoing filters, then return promise
    return stage_apply_outgoing_filters(std::move(e))
      .then([this, work](rpc_envelope e) {
        // dispatch the write concurrently!
        dispatch_write(std::move(e));
        return work->pr.get_future();
      })
      .then([this](stdx::optional<rpc_recv_context> opt_ctx) {
        if (!opt_ctx) {
          return seastar::make_ready_future<ret_type>(std::move(opt_ctx));
        }
        return stage_apply_incoming_filters(std::move(opt_ctx.value()))
          .then([](rpc_recv_context ctx) {
            return seastar::make_ready_future<ret_type>(
              stdx::optional<decltype(ctx)>(std::move(ctx)));
          });
      })
      .then([measure = std::move(measure)](auto opt_ctx) {
        return seastar::make_ready_future<ret_type>(std::move(opt_ctx));
      });
  }


  seastar::future<> connect();

  virtual seastar::future<> stop();

  virtual ~rpc_client();

  virtual void disable_histogram_metrics() final;
  virtual void enable_histogram_metrics() final;

  bool is_histogram_enabled() { return !!hist_; }

  seastar::lw_shared_ptr<histogram> get_histogram() { return hist_; }

  /// \brief use to enqueue or dequeue filters
  /// \code{.cpp}
  ///    client->incoming_filters().push_back(zstd_decompression_filter());
  /// \endcode
  std::vector<in_filter_t> &incoming_filters() { return in_filters_; }
  /// \brief use to enqueue or dequeue filters
  /// \code{.cpp}
  ///    client->outgoing_filters().push_back(zstd_compression_filter(1000));
  /// \endcode
  std::vector<out_filter_t> &outgoing_filters() { return out_filters_; }

  SMF_DISALLOW_COPY_AND_ASSIGN(rpc_client);

 public:
  const seastar::ipv4_addr                      server_addr;
  seastar::lw_shared_ptr<rpc_connection_limits> limits;

 public:
  // need to be public for parent_shared_from_this()
  bool                                   is_error_state{false};
  uint64_t                               read_counter{0};
  seastar::lw_shared_ptr<rpc_connection> conn;
  std::unordered_map<uint16_t, seastar::lw_shared_ptr<work_item>> rpc_slots;
  // needed public by execution stage
  seastar::future<rpc_recv_context> apply_incoming_filters(rpc_recv_context);
  seastar::future<rpc_envelope>     apply_outgoing_filters(rpc_envelope);


 private:
  seastar::future<> do_reads();
  seastar::future<> dispatch_write(rpc_envelope e);


  /// brief use SEDA pipelines for filtering
  seastar::future<rpc_recv_context> stage_apply_incoming_filters(
    rpc_recv_context);
  seastar::future<rpc_envelope> stage_apply_outgoing_filters(rpc_envelope);


  std::vector<in_filter_t>  in_filters_;
  std::vector<out_filter_t> out_filters_;

  seastar::semaphore                serialize_writes_{1};
  seastar::lw_shared_ptr<histogram> hist_ = nullptr;
  uint16_t                          session_idx_{0};
};


}  // namespace smf
