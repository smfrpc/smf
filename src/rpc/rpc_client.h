#pragma once

namespace smf {
// distributed<client> clients;
// transport protocol = transport::TCP;
class client {
  public:
  class connection {
    connected_socket socket_;
    input_stream<char> read_buf_;
    output_stream<char> write_buf_;
    rpc_client_stats &stats_;

    public:
    connection(connected_socket &&fd, stats)
      : socket_(std::move(fd))
      , read_buf_(socket_.input())
      , write_buf_(socket_.output())
      , stats_(stats) {}

    future<> do_read() {
      return _read_buf.read_exactly(rx_msg_size)
        .then([this](temporary_buffer<char> buf) {
          _bytes_read += buf.size();
          if(buf.size() == 0) {
            return make_ready_future();
          } else {
            return do_read();
          }
        });
    }

    future<> do_write(int end) {
      if(end == 0) {
        return make_ready_future();
      }
      return _write_buf.write(str_txbuf)
        .then([this, end] {
          _bytes_write += tx_msg_size;
          return _write_buf.flush();
        })
        .then([this, end] { return do_write(end - 1); });
    }


    future<> start(ipv4_addr server_addr, std::string test, unsigned ncon) {
      _server_addr = server_addr;
      _concurrent_connections = ncon * smp::count;
      _total_pings = _pings_per_connection * _concurrent_connections;
      _test = test;

      for(unsigned i = 0; i < ncon; i++) {
        socket_address local =
          socket_address(::sockaddr_in{AF_INET, INADDR_ANY, {0}});
        engine()
          .net()
          .connect(make_ipv4_address(server_addr), local, protocol)
          .then([this, server_addr, test](connected_socket fd) {
            auto conn = new connection(std::move(fd));
            (this->*tests.at(test))(conn).then_wrapped([this, conn](auto &&f) {
              delete conn;
              try {
                f.get();
              } catch(std::exception &ex) {
                fprint(std::cerr, "request error: %s\n", ex.what());
              }
            });
          });
      }
      return make_ready_future();
    }
    future<> stop() { return make_ready_future(); }

    private:
    ipv4_addr server_addr_;
    rpc_client_stats stats_;
  };
}
