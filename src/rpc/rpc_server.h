#pragma once
#include "core/distributed.hh"

class tcp_server {
  private:
  lw_shared_ptr<server_socket> _listener;
  sharded_cache &_cache;
  distributed<system_stats> &_system_stats;
  uint16_t _port;
  struct connection {
    connected_socket _socket;
    socket_address _addr;
    input_stream<char> _in;
    output_stream<char> _out;
    ascii_protocol _proto;
    distributed<system_stats> &_system_stats;
    connection(connected_socket &&socket,
               socket_address addr,
               sharded_cache &c,
               distributed<system_stats> &system_stats)
      : _socket(std::move(socket))
      , _addr(addr)
      , _in(_socket.input())
      , _out(_socket.output())
      , _proto(c, system_stats)
      , _system_stats(system_stats) {
      _system_stats.local()._curr_connections++;
      _system_stats.local()._total_connections++;
    }
    ~connection() { _system_stats.local()._curr_connections--; }
  };

  public:
  tcp_server(sharded_cache &cache,
             distributed<system_stats> &system_stats,
             uint16_t port = 11211)
    : _cache(cache), _system_stats(system_stats), _port(port) {}

  void start() {
    listen_options lo;
    lo.reuse_address = true;
    _listener = engine().listen(make_ipv4_address({_port}), lo);
    keep_doing([this] {
      return _listener->accept().then(
        [this](connected_socket fd, socket_address addr) mutable {
          auto conn = make_lw_shared<connection>(std::move(fd), addr, _cache,
                                                 _system_stats);
          do_until([conn] { return conn->_in.eof(); },
                   [this, conn] {
                     return conn->_proto.handle(conn->_in, conn->_out)
                       .then([conn] { return conn->_out.flush(); });
                   })
            .finally([conn] { return conn->_out.close().finally([conn] {}); });
        });
    })
      .or_terminate();
  }

  future<> stop() { return make_ready_future<>(); }
};

class stats_printer {
  private:
  timer<> _timer;
  sharded_cache &_cache;

  public:
  stats_printer(sharded_cache &cache) : _cache(cache) {}

  void start() {
    _timer.set_callback([this] {
      _cache.stats().then([this](auto stats) {
        auto gets_total = stats._get_hits + stats._get_misses;
        auto get_hit_rate =
          gets_total ? ((double)stats._get_hits * 100 / gets_total) : 0;
        auto sets_total = stats._set_adds + stats._set_replaces;
        auto set_replace_rate =
          sets_total ? ((double)stats._set_replaces * 100 / sets_total) : 0;
        std::cout << "items: " << stats._size << " " << std::setprecision(2)
                  << std::fixed << "get: " << stats._get_hits << "/"
                  << gets_total << " (" << get_hit_rate << "%) "
                  << "set: " << stats._set_replaces << "/" << sets_total << " ("
                  << set_replace_rate << "%)";
        std::cout << std::endl;
      });
    });
    _timer.arm_periodic(std::chrono::seconds(1));
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
//         return tcp_server.start(std::ref(cache), std::ref(system_stats), port);
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
//         [&] { return udp_server.invoke_on_all(&memcache::udp_server::start); })
//       .then([&stats, start_stats = config.count("stats")] {
//         if(start_stats) {
//           stats.start();
//         }
//       });
//   });
// }
