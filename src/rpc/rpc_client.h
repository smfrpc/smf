#pragma once
// seastar
#include <core/future-util.hh>
// smf
#include "log.h"
#include "hashing_utils.h"
#include "rpc/rpc_recv_context.h"

namespace smf {
class rpc_client {
  public:
  rpc_client(ipv4_addr server_addr) : server_addr_(server_addr) {}

  /// \brief rpc clients should override this method
  virtual future<> recv(std::uniqe_ptr<rpc_recv_context> ctx) {}

  /// \brief
  /// \return
  virtual future<> send(rpc_request &&req) final {
    if(!conn_) {
      return engine()
        .net()
        .connect(make_ipv4_address(server_addr_), local_, transport::TCP)
        .then([this](connected_socket fd) {
          conn_ = std::move(std::make_unique<connection>(std::move(fd)));
          return chain_send(std::move(req));
        });
    }
    return chain_send(std::move(req));
  }
  virtual future<> stop() { return make_ready_future(); }
  virtual ~rpc_client() {}

  private:
  future<> chain_send(rpc_request &&req) {
    req.finish();
    return rpc_send_context(conn_->ostream(), buf, len)
      .send()
      .then([this, oneway] { return chain_recv(oneway); });
  }
  future<> chain_recv(bool oneway) {
    if(oneway) {
      return make_ready_future<>();
    }
    auto ctx = std::make_unique<rpc_recv_context>(conn_->istream());
    return ctx->parse().then([this, ctx = std::move(ctx)] {
      if(!recv_ctx->is_parsed()) {
        log.error("Invalid response");
        return make_ready_future<>();
      }
      return recv(std::move(ctx));
    });
  }

  private:
  ipv4_addr server_addr_;
  socket_address local_ =
    socket_address(::sockaddr_in{AF_INET, INADDR_ANY, {0}});
  std::unique_ptr<connection> conn_ = nullptr;

  private:
  class connection {
    public:
    connection(connected_socket &&fd)
      : socket_(std::move(fd)), in_(socket_.input()), out_(socket_.output()) {}
    input_stream<char> &istream() { return in_; };
    onput_stream<char> &ostream() { return out_; };

    private:
    connected_socket socket_;
    input_stream<char> in_;
    output_stream<char> out_;
    // TODO(agallego) - add stats
    // rpc_client_stats &stats_;
  };
};
}
