// Copyright (c) 2016 Alexander Gallego. All rights reserved.
//
// std
#include <chrono>
#include <iostream>
// linux
#include <arpa/inet.h>
#include <ifaddrs.h>
#include <net/if.h>
// seastar
#include <seastar/core/app-template.hh>
#include <seastar/core/distributed.hh>
#include <seastar/core/sleep.hh>
#include <seastar/core/sstring.hh>
#include <seastar/net/api.hh>
// smf
#include "integration_tests/non_root_port.h"
#include "smf/log.h"
#include "smf/random.h"
#include "smf/rpc_generated.h"
#include "smf/rpc_handle_router.h"
#include "smf/rpc_recv_context.h"
#include "smf/rpc_server.h"
// templates
#include "integration_tests/demo_service.smf.fb.h"

// For a network connected host we should always have at least an lo and eth0
// interface (or whatever the connected interface is called) ... so capping the
// maximum test clients at 2 will cover this. Going larger doesn't improve the
// accuracy of this test.
constexpr static const size_t kMaxClients = 2;

static inline seastar::sstring
ipv4_addr_to_ip_str(seastar::ipv4_addr addr) {
  std::stringstream addr_;
  addr_ << addr;
  return seastar::to_sstring(addr_.str().substr(0, addr_.str().find(":")));
}

struct client_info {
  explicit client_info(seastar::ipv4_addr ip)
    : client(seastar::make_shared<smf_gen::demo::SmfStorageClient>(ip)),
      ip(ip.ip){};
  client_info(client_info &&o) noexcept : client(o.client), ip(o.ip){};
  ~client_info() = default;
  seastar::shared_ptr<smf_gen::demo::SmfStorageClient> client = nullptr;
  uint32_t ip = 0;
  SMF_DISALLOW_COPY_AND_ASSIGN(client_info);
};

struct ifaddrs_t {
  ifaddrs_t() {
    LOG_THROW_IF(getifaddrs(&addr) == -1, "failed to getifaddrs()");
  }
  ifaddrs_t(ifaddrs_t &&o) noexcept : addr(std::move(o.addr)) {}
  ~ifaddrs_t() {
    if (addr) { freeifaddrs(addr); }
  }
  struct ifaddrs *addr = nullptr;
  SMF_DISALLOW_COPY_AND_ASSIGN(ifaddrs_t);
};

class client_dispatcher {
 public:
  explicit client_dispatcher(uint16_t port) {
    ifaddrs_t ifaddr;
    struct ifaddrs *ifa = client_dispatcher::next_valid_interface(ifaddr.addr);
    for (int i = 0; i < kMaxClients; ifa = ifa->ifa_next, i++) {
      ifa = THROW_IFNULL(client_dispatcher::next_valid_interface(ifa));
      auto sa_in = reinterpret_cast<struct sockaddr_in *>(ifa->ifa_addr);
      auto ip = ntohl(sa_in->sin_addr.s_addr);
      clients_.push_back(client_info(seastar::make_ipv4_address(ip, port)));
      DLOG_DEBUG("Added client {}", clients_[i].client->server_addr);
    }
  }

  auto
  begin() {
    return clients_.begin();
  }
  auto
  end() {
    return clients_.end();
  }

 private:
  static struct ifaddrs *
  next_valid_interface(struct ifaddrs *ifa) {
    while (ifa && !(ifa->ifa_addr->sa_family == AF_INET &&
                    ifa->ifa_flags & IFF_RUNNING)) {
      ifa = ifa->ifa_next;
    }
    LOG_INFO("Next valid interface {}", ifa->ifa_name);
    return ifa;
  }
  std::vector<client_info> clients_;
};

class storage_service final : public smf_gen::demo::SmfStorage {
  virtual seastar::future<smf::rpc_typed_envelope<smf_gen::demo::Response>>
  Get(smf::rpc_recv_typed_context<smf_gen::demo::Request> &&rec) final {
    smf::rpc_typed_envelope<smf_gen::demo::Response> data;
    if (rec) {
      DLOG_DEBUG("Got request name {}", rec->name()->str());
      data.data->name = ipv4_addr_to_ip_str(rec.ctx->remote_address);
    }
    data.envelope.set_status(200);
    return seastar::make_ready_future<
      smf::rpc_typed_envelope<smf_gen::demo::Response>>(std::move(data));
  }
};

int
main(int args, char **argv, char **env) {
  std::cout.setf(std::ios::unitbuf);
  seastar::distributed<smf::rpc_server> rpc;
  seastar::app_template app;

  try {
    return app.run(args, argv, [&]() -> seastar::future<int> {
      smf::app_run_log_level(seastar::log_level::trace);
      DLOG_DEBUG("Setting up at_exit hooks");
      seastar::engine().at_exit([&] { return rpc.stop(); });

      smf::random r;
      const uint16_t random_port =
        smf::non_root_port(r.next() % std::numeric_limits<uint16_t>::max());

      smf::rpc_server_args sargs;
      sargs.ip = "0.0.0.0";
      sargs.rpc_port = random_port;
      sargs.flags |=
        smf::rpc_server_flags::rpc_server_flags_disable_http_server;
      return rpc.start(sargs)
        .then([&rpc] {
          return rpc.invoke_on_all(
            &smf::rpc_server::register_service<storage_service>);
        })
        .then([&rpc] {
          DLOG_DEBUG("Invoking rpc start on all cores");
          return rpc.invoke_on_all(&smf::rpc_server::start);
        })
        .then([random_port] {
          return seastar::do_with(
            client_dispatcher(random_port), [](auto &clients) {
              return seastar::do_for_each(clients, [](auto &client) {
                LOG_INFO("Connecting client to {}", client.client->server_addr);
                return client.client->connect()
                  .then([&client] {
                    DLOG_DEBUG("Connected to {}", client.client->server_addr);
                    smf::rpc_typed_envelope<smf_gen::demo::Request> req;
                    req.data->name =
                      ipv4_addr_to_ip_str(client.client->server_addr);
                    return client.client->Get(req.serialize_data());
                  })
                  .then([&client](auto reply) {
                    std::string name = reply->name()->str();
                    DLOG_DEBUG("Got reply {}", name);
                    if (seastar::to_sstring(name) !=
                        ipv4_addr_to_ip_str(client.client->server_addr)) {
                      LOG_THROW("Server did not see our IP {} != {}", name,
                                client.client->server_addr);
                    }
                    return seastar::make_ready_future<>();
                  })
                  .then([&client] {
                    return client.client->stop().finally(
                      [c = client.client] {});
                  });
              });
            });
        })
        .then([] {
          DLOG_DEBUG("Exiting");
          return seastar::make_ready_future<int>(0);
        });
    });
  } catch (const std::exception &e) {
    LOG_ERROR("Exception running app: {}", e);
    return 1;
  }
}
