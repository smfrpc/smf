// Copyright (c) 2016 Alexander Gallego. All rights reserved.
//
#pragma once
// std
#include <experimental/optional>
#include <memory>
#include <utility>
#include <vector>
// seastar
#include <net/api.hh>
// smf
#include "histogram/histogram.h"
#include "platform/macros.h"
#include "rpc/rpc_connection.h"
#include "rpc/rpc_envelope.h"
#include "rpc/rpc_filter.h"
#include "rpc/rpc_recv_typed_context.h"

// NOTE:
// Currently there is a limitation of 2GB per payload
// This is a flatbuffer's limination that we can bypass quickly if we
// simply read in 3 steps. HEADER + BODY_METADATA + BODY.
// Since we don't have any clients sending more than 2GB, we can skip this for
// now.
// This would be a backward incompatible change

namespace smf {
/// \brief class intented for communicating with a remote host
///        this class is relatively cheap outside of the socket cost
///        the intended use case is single threaded, callback driven
///
/// Call send() to initiate the protocol. If the request is oneway=true
/// then no recv() callbcak will be issued. Otherwise, you can
/// parse the contents of the server response on recv() callback
///
class rpc_client {
 public:
  using in_filter_t = std::function<future<rpc_recv_context>(rpc_recv_context)>;
  using out_filter_t = std::function<future<rpc_envelope>(rpc_envelope)>;

  // TODO(agallego) - add options for ipv6, ssl, buffering, etc.
  explicit rpc_client(ipv4_addr server_addr);

  template <typename ReturnType, typename NativeTable>
  future<rpc_recv_typed_context<ReturnType>> send(const NativeTable &t,
                                                  bool oneway = false) {
    static_assert(std::is_base_of<flatbuffers::NativeTable, NativeTable>::value,
                  "argument `t' must extend flatbuffers::NativeTable");

#define FBB_FN_BUILD(T) Create##T

    // need to get the basic builder for payload
    rpc_envelope e(nullptr);
    FBB_FN_BUILD(NativeTable)(*e.mutable_builder(), &t);
    return send<ReturnType>(std::move(e), oneway);
  }

  /// \brief actually does the send to the remote location
  /// \param req - the bytes to send
  /// \param oneway - if oneway, then, no recv request is issued
  ///        this is useful for void functions
  ///
  template <typename T>
  future<rpc_recv_typed_context<T>> send(rpc_envelope e, bool oneway = false) {
    using ret_type = rpc_recv_typed_context<T>;
    e.finish();  // make sure that the buff is ready
    auto measure = is_histogram_enabled() ? hist_->auto_measure() : nullptr;
    return rpc_filter_apply(&out_filters_, std::move(e))
      .then([this, oneway](rpc_envelope &&e) {
        return chain_send(std::move(e), oneway);
      })
      .then([this](auto opt_ctx) {
        if (!opt_ctx) {
          return make_ready_future<ret_type>(std::move(opt_ctx));
        }
        return rpc_filter_apply(&in_filters_, std::move(opt_ctx.value()))
          .then([](auto ctx) {
            return make_ready_future<ret_type>(
              std::experimental::optional<decltype(ctx)>(std::move(ctx)));
          });
      })
      .then([measure = std::move(measure)](auto opt_ctx) {
        return make_ready_future<ret_type>(std::move(opt_ctx));
      });
  }

  future<std::experimental::optional<rpc_recv_context>> chain_send(
    rpc_envelope e, bool oneway) {
    return smf::rpc_envelope::send(&conn_->ostream, std::move(e))
      .then([this, oneway]() {
        if (oneway) {
          using t = std::experimental::optional<rpc_recv_context>;
          return make_ready_future<t>(std::experimental::nullopt);
        }
        return smf::rpc_recv_context::parse(conn_.get(), nullptr);
      });
  }

  future<> connect();

  virtual future<> stop();

  virtual ~rpc_client();

  virtual void enable_histogram_metrics(bool enable = true) final;

  bool is_histogram_enabled() { return hist_.operator bool(); }

  histogram *get_histogram() {
    if (hist_) {
      return hist_.get();
    }
    return nullptr;
  }

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

  bool is_semaphore_empty() { return limit_.current() == 0; }

  SMF_DISALLOW_COPY_AND_ASSIGN(rpc_client);

 public:
  const ipv4_addr server_addr;

 protected:
  /// \brief - semaphore for `safe_` codegen calls
  ///
  /// seastar ONLY allows for one receiving context per call
  /// so the recv call has to parse the response payload before
  /// moving to the next one. - used by the codegen for `safe_##method_name()'
  /// calls
  /// Note that this should only be used by `safe_` calls, it is otherwise
  /// mus slower and not in line w/ the seastar model. This was added here
  /// for usability. This increases the latency, on a highly contended core
  /// the future<> scheduler may yield
  /// (or if the scheduler has executed enough ready_future<>'s)
  ///
  semaphore limit_{1};

 private:
  std::unique_ptr<rpc_connection> conn_;
  std::unique_ptr<histogram>      hist_;

  std::vector<in_filter_t>  in_filters_;
  std::vector<out_filter_t> out_filters_;
};


}  // namespace smf
