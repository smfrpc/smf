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
#include <core/app-template.hh>
#include <core/distributed.hh>
#include <core/sleep.hh>
#include <net/api.hh>
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

// For a network connected host we should always have at least an lo and eth0 interface 
// (or whatever the connected interface is called) ... so capping the maximum test clients
// at 2 will cover this. Going larger doesn't improve the accuracy of this test.
constexpr static const size_t kMaxClients = 2;

struct client_info {
    explicit client_info(seastar::ipv4_addr ip): client(smf_gen::demo::SmfStorageClient::make_shared(ip)), ip(ip.ip) {};
    client_info(client_info &&o) noexcept
        : client(std::move(o.client)), ip(o.ip) {
            std::move(std::begin(o.ip_str), std::end(o.ip_str), &o.ip_str[0]); // can't move static array, copy
        };
    ~client_info(){};
    seastar::shared_ptr<smf_gen::demo::SmfStorageClient> client = nullptr;
    uint32_t ip = 0;
    char ip_str[INET_ADDRSTRLEN]{};
    SMF_DISALLOW_COPY_AND_ASSIGN(client_info);
};

struct ifaddrs_t {
    ifaddrs_t() { LOG_THROW_IF(getifaddrs(&ifaddr_) == -1, "failed to getifaddrs()"); }
    ~ifaddrs_t() { freeifaddrs(ifaddr_); }
    struct ifaddrs *ifaddr_ = nullptr;
};

class clients {
public:
    explicit clients(uint16_t port) {
        auto ifaddr = ifaddrs_t();
        struct ifaddrs *ifa = clients::next_valid_interface(ifaddr.ifaddr_);
        for (int i = 0; i < kMaxClients; ifa = ifa->ifa_next, i++) {
            ifa = clients::next_valid_interface(ifa);
            THROW_IFNULL(ifa);
            auto sa_in = reinterpret_cast<struct sockaddr_in*>(ifa->ifa_addr);
            auto ip = ntohl(sa_in->sin_addr.s_addr);
            clients_.push_back(client_info(seastar::make_ipv4_address(ip, port)));
            THROW_IFNULL(inet_ntop(AF_INET, &sa_in->sin_addr, clients_[i].ip_str, INET_ADDRSTRLEN));
            DLOG_DEBUG("Added client ip {}", clients_[i].ip_str);
        }
    }

    auto begin() { return clients_.begin(); }
    auto end() { return clients_.end(); }
private:
    static struct ifaddrs* next_valid_interface(struct ifaddrs* ifa) {
        while (ifa && !(ifa->ifa_addr->sa_family == AF_INET && ifa->ifa_flags & IFF_RUNNING)) {
            ifa = ifa->ifa_next;
        }
        DLOG_DEBUG("Next valid interface {}", ifa->ifa_name);
        return ifa;
    }
    std::vector<struct client_info> clients_;
};

class storage_service : public smf_gen::demo::SmfStorage {
  virtual seastar::future<smf::rpc_typed_envelope<smf_gen::demo::Response>>
  Get(smf::rpc_recv_typed_context<smf_gen::demo::Request> &&rec) final {
    char address_str[INET_ADDRSTRLEN] = {};
    smf::rpc_typed_envelope<smf_gen::demo::Response> data;
    if (rec) {
        DLOG_DEBUG("Got request name {}", rec->name()->str());
        inet_ntop(AF_INET, &rec.ctx->remote_address.as_posix_sockaddr_in().sin_addr, address_str, INET_ADDRSTRLEN);
        data.data->name = seastar::to_sstring(address_str);
    }
    data.envelope.set_status(200);
    return seastar::make_ready_future<
        smf::rpc_typed_envelope<smf_gen::demo::Response>>(std::move(data));
  }
};

int
main(int args, char **argv, char **env) {
  DLOG_DEBUG("About to start the RPC test");
  seastar::distributed<smf::rpc_server> rpc;

  seastar::app_template app;

  smf::random rand;
  uint16_t random_port =
    smf::non_root_port(rand.next() % std::numeric_limits<uint16_t>::max());

  return app.run(args, argv, [&]() -> seastar::future<int> {
    DLOG_DEBUG("Setting up at_exit hooks");
    seastar::engine().at_exit([&] { return rpc.stop(); });

    smf::random r;
    smf::rpc_server_args sargs;
    sargs.ip = "0.0.0.0";
    sargs.rpc_port = random_port;
    sargs.http_port =
      smf::non_root_port(rand.next() % std::numeric_limits<uint16_t>::max());
    sargs.flags |= smf::rpc_server_flags::rpc_server_flags_disable_http_server;
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
        return seastar::do_with(clients(random_port), [] (auto& clients) {
          return seastar::do_for_each(clients, [] (auto& client) {
            DLOG_DEBUG("Connecting client to {}", client.client->server_addr);
            return client.client->connect()
            .then([&client] {
              DLOG_DEBUG("Connected to {}", client.ip_str);
              smf::rpc_typed_envelope<smf_gen::demo::Request> req;
              req.data->name = seastar::sstring(client.ip_str, INET_ADDRSTRLEN);
              return client.client->Get(req.serialize_data());
            })
            .then([&client] (auto reply) {
              DLOG_DEBUG("Got reply {}", reply->name()->str());
              if (reply->name()->str() != client.ip_str) {
                  LOG_THROW("Server did not see our IP {} != {}", reply->name()->str(), client.ip_str);
              }
              return seastar::make_ready_future<>();
            })
            .then([&client] { 
              return client.client->stop(); 
            });
          });
        });
      })
      .then([] {
        DLOG_DEBUG("Exiting");
        return seastar::make_ready_future<int>(0);
      });
  });
}
