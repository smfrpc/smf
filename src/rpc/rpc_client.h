#pragma once
// seastar
#include <net/api.hh>
// smf
#include "rpc/rpc_recv_context.h"
#include "rpc/rpc_envelope.h"
#include "rpc/rpc_client_connection.h"

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
  rpc_client(ipv4_addr server_addr);

  /// \brief if on the send(oneway=false) then this is the callback
  ///        issued when the server replies
  virtual future<> recv(rpc_recv_context &&ctx);

  /// \brief actually does the send to the remote location
  /// \param req - the bytes to send
  /// \param oneway - if oneway, then, no recv request is issued
  ///        this is useful for void functions
  ///
  virtual future<> send(rpc_envelope &&req, bool oneway = false) final;
  virtual future<> stop();
  virtual ~rpc_client();

  private:
  future<> connect();
  future<> chain_send(rpc_envelope &&req, bool oneway);
  future<> chain_recv(bool oneway);

  private:
  ipv4_addr server_addr_;
  rpc_client_connection *conn_ = nullptr;
};
}
