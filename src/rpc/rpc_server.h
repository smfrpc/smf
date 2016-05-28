#pragma once
#include "core/distributed.hh"

// smf
#include "rpc/rpc_server.h"
namespace smf {
class rpc_server {
  private:
  lw_shared_ptr<server_socket> listener_;
  distributed<rpc_stats> &stats_;
  uint16_t port_;
  struct connection {
    connected_socket socket_;
    socket_address addr_;
    input_stream<char> in_;
    output_stream<char> out;
    prc_size_based_parser proto_;
    distributed<rpc_stats> &stats_;
    connection(connected_socket &&socket,
               socket_address addr,
               distributed<rpc_stats> &stats)
      : socket_(std::move(socket))
      , addr_(addr)
      , in_(socket_.input())   // has no alternate ctor
      , out_(socket_.output()) // has no alternate ctor
      , stats_(stats) {
      stats_.local().active_connections++;
      stats_.local().total_connections++;
    }
    ~connection() { stats_.local().active_connections--; }
  };

  public:
  tcp_server(distributed<rpc_stats> &stats, uint16_t port = 11225)
    : stats_(stats), port_(port) {}

  void start() {
    listen_options lo;
    lo.reuse_address = true;
    listener_ = engine().listen(make_ipv4_address({port_}), lo);
    keep_doing([this] {
      return listener_->accept().then(
        [this](connected_socket fd, socket_address addr) mutable {
          auto conn =
            make_lw_shared<connection>(std::move(fd), addr, _cache, stats_);

          // keep open forever until the client shuts down the socket
          // no need to close the connction
          do_until([conn] { return conn->_in.eof(); },
                   [this, conn] {
                     return conn->proto_.handle(conn->_in, conn->_out)
                       .then([conn] { return conn->out_.flush(); });
                   })
            .finally([conn] { return conn->_out.close().finally([conn] {}); });
        });
    })
      .or_terminate();
  }

  future<> stop() { return make_ready_future<>(); }
};



} /* namespace memcache */

// int main(int ac, char **av) {
//   distributed<memcache::cache> cache_peers;
//   memcache::sharded_cache cache(cache_peers);
//   distributed<memcache::system_stats> system_stats;
//   distributed<memcache::udp_server> udp_server;
//   distributed<memcache::tcp_server> tcp_server;
//   memcache::stats_printer stats(cache);

//   namespace bpo = boost::program_options;
//   app_template app;
//   app.add_options()("max-datagram-size",
//                     bpo::value<int>()->default_value(
//                       memcache::udp_server::default_max_datagram_size),
//                     "Maximum size of UDP datagram")(
//     "max-slab-size", bpo::value<uint64_t>()->default_value(
//                        memcache::default_per_cpu_slab_size / MB),
//     "Maximum memory to be used for items (value in megabytes) (reclaimer is "
//     "disabled if set)")("slab-page-size",
//                         bpo::value<uint64_t>()->default_value(
//                           memcache::default_slab_page_size / MB),
//                         "Size of slab page (value in megabytes)")(
//     "stats", "Print basic statistics periodically (every second)")(
//     "port", bpo::value<uint16_t>()->default_value(11211),
//     "Specify UDP and TCP ports for memcached server to listen on");

//   return app.run_deprecated(ac, av, [&] {
//     engine().at_exit([&] { return tcp_server.stop(); });
//     engine().at_exit([&] { return udp_server.stop(); });
//     engine().at_exit([&] { return cache_peers.stop(); });
//     engine().at_exit([&] { return system_stats.stop(); });

//     auto &&config = app.configuration();
//     uint16_t port = config["port"].as<uint16_t>();
//     uint64_t per_cpu_slab_size = config["max-slab-size"].as<uint64_t>() * MB;
//     uint64_t slab_page_size = config["slab-page-size"].as<uint64_t>() * MB;
//     return cache_peers.start(std::move(per_cpu_slab_size),
//                              std::move(slab_page_size))
//       .then([&system_stats] {
//         return system_stats.start(memcache::clock_type::now());
//       })
//       .then([&] {
//         std::cout << PLATFORM << " memcached " << VERSION << "\n";
//         return make_ready_future<>();
//       })
//       .then([&, port] {
//         return tcp_server.start(std::ref(cache), std::ref(system_stats),
//         port);
//       })
//       .then([&tcp_server] {
//         return tcp_server.invoke_on_all(&memcache::tcp_server::start);
//       })
//       .then([&, port] {
//         if(engine().net().has_per_core_namespace()) {
//           return udp_server.start(std::ref(cache), std::ref(system_stats),
//                                   port);
//         } else {
//           return udp_server.start_single(std::ref(cache),
//                                          std::ref(system_stats), port);
//         }
//       })
//       .then([&] {
//         return udp_server.invoke_on_all(
//           &memcache::udp_server::set_max_datagram_size,
//           (size_t)config["max-datagram-size"].as<int>());
//       })
//       .then(
//         [&] { return udp_server.invoke_on_all(&memcache::udp_server::start);
//         })
//       .then([&stats, start_stats = config.count("stats")] {
//         if(start_stats) {
//           stats.start();
//         }
//       });
//   });
// }
}
