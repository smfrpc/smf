#pragma once
// std
#include <memory>
// seastar
#include <core/reactor.hh>
#include <core/future-util.hh>
#include <net/api.hh> // todo - forward decl

// smf
#include "log.h"
#include "hashing_utils.h"
#include "rpc/rpc_recv_context.h"
#include "rpc/rpc_request.h"
#include "rpc/rpc_client_connection.h"
#include "rpc/rpc_send_context.h"

namespace smf {
class rpc_client {
  public:
  rpc_client(ipv4_addr server_addr) : server_addr_(std::move(server_addr)) {
    LOG_INFO("constructed rpc_client");
  }

  /// \brief rpc clients should override this method
  virtual future<> recv(rpc_recv_context &&ctx) {
    LOG_INFO("HOORAY GOT A REPLY");
    return make_ready_future<>();
  }

  /// \brief
  /// \return
  virtual future<> send(rpc_request &&req, bool oneway = false) final {
    return connect().then([this, r = std::move(req), oneway]() mutable {
      return chain_send(std::move(r), oneway);
    });
  }
  virtual future<> stop() { return make_ready_future(); }
  virtual ~rpc_client() {
    if(conn_) {
      delete conn_;
    }
  }

  private:
  future<> connect() {
    LOG_INFO("connecting");
    if(conn_ != nullptr) {
      LOG_INFO("socket non null");
      return make_ready_future<>();
    }
    LOG_INFO("Not connected, creating new tcp connection");
    socket_address local =
      socket_address(::sockaddr_in{AF_INET, INADDR_ANY, {0}});
    return engine()
      .net()
      .connect(make_ipv4_address(server_addr_), local, seastar::transport::TCP)
      .then([this](connected_socket fd) mutable {
        LOG_INFO("creating a new rpc_client_connection");
        conn_ = new rpc_client_connection(std::move(fd));
        LOG_INFO("finished creating rpc client connection");
      });
  }
  future<> chain_send(rpc_request &&req, bool oneway) {
    LOG_INFO("chain_send");
    return rpc_send_context(conn_->ostream(), std::move(req))
      .then([this, oneway] { return chain_recv(oneway); });
  }
  future<> chain_recv(bool oneway) {
    LOG_INFO("chain_recv");
    if(oneway) {
      return make_ready_future<>();
    }

    return parse_rpc_recv_context(conn_->istream())
      .then([this](std::experimental::optional<rpc_recv_context> ctx) mutable {
        if(!ctx) {
          LOG_ERROR("Invalid response");
          return make_ready_future<>();
        }
        return recv(std::move(ctx.value()));
      });
  }

  private:
  ipv4_addr server_addr_;
  // std::unique_ptr<rpc_client_connection> conn_;
  rpc_client_connection *conn_ = nullptr;
};
}
