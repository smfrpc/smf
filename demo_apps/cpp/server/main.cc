// Copyright (c) 2017 Alexander Gallego. All rights reserved.
//

#include <chrono>
#include <iostream>

#include <seastar/core/app-template.hh>
#include <seastar/core/distributed.hh>
#include <seastar/core/reactor.hh>
#include <seastar/net/api.hh>

#include "smf/histogram_seastar_utils.h"
#include "smf/log.h"
#include "smf/rpc_filter.h"
#include "smf/rpc_server.h"
#include "smf/unique_histogram_adder.h"
#include "smf/zstd_filter.h"

#include "demo_service.smf.fb.h"

class storage_service final : public smf_gen::demo::SmfStorage {
  virtual seastar::future<smf::rpc_typed_envelope<smf_gen::demo::Response>>
  Get(smf::rpc_recv_typed_context<smf_gen::demo::Request> &&rec) final {
    smf::rpc_typed_envelope<smf_gen::demo::Response> data;
    // return the same payload
    if (rec) { data.data->name = rec->name()->c_str(); }
    data.envelope.set_status(200);
    return seastar::make_ready_future<
      smf::rpc_typed_envelope<smf_gen::demo::Response>>(std::move(data));
  }
};

void
cli_opts(boost::program_options::options_description_easy_init o) {
  namespace po = boost::program_options;
  o("ip", po::value<std::string>()->default_value("127.0.0.1"),
    "ip to connect to");
  o("port", po::value<uint16_t>()->default_value(20776), "port for service");
  o("httpport", po::value<uint16_t>()->default_value(20777),
    "port for http stats service");
  o("key",
    po::value<std::string>()->default_value(""),
    "key for TLS seccured connection");
  o("cert",
    po::value<std::string>()->default_value(""),
      "cert for TLS seccured connection");
}

int
main(int args, char **argv, char **env) {
  std::setvbuf(stdout, nullptr, _IOLBF, 1024);
  seastar::distributed<smf::rpc_server> rpc;
  seastar::app_template app;
  cli_opts(app.add_options());

  return app.run_deprecated(args, argv, [&] {
    seastar::engine().at_exit([&] {
      return rpc
        .map_reduce(smf::unique_histogram_adder(),
                    &smf::rpc_server::copy_histogram)
        .then([](auto h) {
          LOG_INFO("Writing server histograms");
          return smf::histogram_seastar_utils::write("server_latency.hgrm",
                                                     std::move(h));
        })
        .then([&rpc] { return rpc.stop(); });
    });

    auto &cfg = app.configuration();

    smf::rpc_server_args args;
    args.ip = cfg["ip"].as<std::string>().c_str();
    args.rpc_port = cfg["port"].as<uint16_t>();
    args.http_port = cfg["httpport"].as<uint16_t>();

    auto key = cfg["key"].as<std::string>();
    auto cert = cfg["cert"].as<std::string>();
    if (key != "" && cert != "") {
        auto builder = seastar::tls::credentials_builder();
        builder.set_dh_level(seastar::tls::dh_params::level::MEDIUM);
        builder
          .set_x509_key_file(cert, key, seastar::tls::x509_crt_format::PEM)
          .get();
        args.credentials
          = builder.build_reloadable_server_credentials().get0();
    }
 
    args.memory_avail_per_core =
      static_cast<uint64_t>(0.9 * seastar::memory::stats().total_memory());

    return rpc.start(args)
      .then([&rpc] {
        LOG_INFO("Registering smf_gen::demo::storage_service");
        return rpc.invoke_on_all(
          &smf::rpc_server::register_service<storage_service>);
      })
      .then([&rpc] {
        return rpc.invoke_on_all(&smf::rpc_server::register_incoming_filter<
                                 smf::zstd_decompression_filter>);
      })
      .then([&rpc] {
        LOG_INFO("Invoking rpc start on all cores");
        return rpc.invoke_on_all(&smf::rpc_server::start);
      });
  });
}
