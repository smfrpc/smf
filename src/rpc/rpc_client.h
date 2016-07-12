#pragma once
// std
#include <experimental/optional>
// seastar
#include <net/api.hh>
// smf
#include "rpc/rpc_recv_context.h"
#include "rpc/rpc_envelope.h"
#include "rpc/rpc_client_connection.h"
#include "log.h"
#include "histogram.h"

namespace smf {
template <typename T> struct rpc_recv_ctx_t {
  using type = T;
  using opt_recv_ctx_t = std::experimental::optional<rpc_recv_context>;

  rpc_recv_ctx_t() : ctx(std::experimental::nullopt) {}
  rpc_recv_ctx_t(opt_recv_ctx_t t) : ctx(std::move(t)) {}
  rpc_recv_ctx_t(rpc_recv_ctx_t &&o) noexcept : ctx(std::move(o.ctx)) {}

  T *get() {
    if(ctx) {
      return flatbuffers::GetMutableRoot<T>(
        ctx.value().payload->mutable_body()->Data());
    }
    return nullptr;
  }

  /// \brief returns true of this obj
  /// so that it works with
  /// \code
  ///     if(obj){}
  /// \endcode
  /// simply forward the bool operator to the option
  operator bool() const { return ctx && ctx != std::experimental::nullopt; }
  opt_recv_ctx_t ctx;
};


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
  rpc_client(ipv4_addr server_addr);
  rpc_client(const rpc_client &) = delete;
  /// \brief actually does the send to the remote location
  /// \param req - the bytes to send
  /// \param oneway - if oneway, then, no recv request is issued
  ///        this is useful for void functions
  ///
  template <typename T>
  future<rpc_recv_ctx_t<T>> send(temporary_buffer<char> buf,
                                 bool oneway = false) {
    return this->connect().then([
      this,
      b = std::move(buf),
      oneway,
      mesure = is_histogram_enabled() ? hist_->auto_measure() : nullptr
    ]() mutable { return this->chain_send<T>(std::move(b), oneway); });
  }

  virtual future<> stop();
  virtual ~rpc_client();
  virtual void enable_histogram_metrics(bool enable = true) final;
  bool is_histogram_enabled() { return hist_.operator bool(); }
  histogram *get_histogram() {
    if(hist_) {
      return hist_.get();
    }
    return nullptr;
  }

  private:
  future<> connect();

  template <typename T>
  future<rpc_recv_ctx_t<T>> chain_send(temporary_buffer<char> buf,
                                       bool oneway) {
    return smf::rpc_envelope::send(conn_->ostream, std::move(buf))
      .then([this, oneway] { return this->chain_recv<T>(oneway); });
  }

  template <typename T> future<rpc_recv_ctx_t<T>> chain_recv(bool oneway) {
    using retval_t = rpc_recv_ctx_t<T>;
    if(oneway) {
      return make_ready_future<retval_t>();
    }
    return rpc_recv_context::parse(conn_->istream)
      .then([this](auto ctx) {
        return make_ready_future<retval_t>(std::move(ctx));
      });
  }


  private:
  ipv4_addr server_addr_;
  rpc_client_connection *conn_ = nullptr;
  std::unique_ptr<histogram> hist_;
};
}
