// Copyright 2019 SMF Authors
//

#pragma once
#include <chrono>
#include <type_traits>

#include <seastar/core/sleep.hh>

#include <smf/log.h>
#include <smf/random.h>
#include <smf/rpc_client.h>

namespace smf {
enum class reconnect_backoff : uint16_t {
  none = 0,
  wait_1_sec = 1,
  wait_3_sec = 3,
  wait_5_sec = 5,
  wait_10_sec = 10,
  wait_20_sec = 20,
  wait_30_sec = 30,
  wait_60_sec = 60,
  wait_300_sec = 300,    // 5min
  wait_600_sec = 600,    // 10min
  wait_1800_sec = 1800,  // 30min
  // always update this one
  max = wait_1800_sec
};

template <typename T>
/// allows any smf::rpc_client to have an exponential backoff
/// background reconnect.
class reconnect_client {
 public:
  using type = std::enable_if_t<std::is_base_of<smf::rpc_client, T>::value, T>;
  SMF_DISALLOW_COPY_AND_ASSIGN(reconnect_client);
  explicit reconnect_client(seastar::ipv4_addr server_addr)
    : client_(seastar::make_shared<type>(std::move(server_addr))) {}
  explicit reconnect_client(smf::rpc_client_opts o)
    : client_(seastar::make_shared<type>(std::move(o))) {}
  reconnect_client(reconnect_client &&o) noexcept
    : client_(std::move(o.client_)), bo_(o.bo_), rand_(std::move(o.rand_)),
      reconnect_gate_(std::move(o.reconnect_gate_)) {}

  /// \brief main method
  seastar::future<> connect();
  seastar::future<>
  stop() {
    return reconnect_gate_.close().then([this] { return client_->stop(); });
  }
  /// \brief returns pointer to client (this) if
  /// everything is good.
  seastar::shared_ptr<T>
  get() {
    return client_;
  }

  reconnect_backoff
  backoff() const {
    return bo_;
  }

 private:
  seastar::shared_ptr<T> client_;
  reconnect_backoff bo_{reconnect_backoff::none};
  random rand_;
  seastar::gate reconnect_gate_;
};

/// \brief prefix operator i.e.: `++backoff;`
inline reconnect_backoff &
operator++(reconnect_backoff &b) {
  const auto as_num = static_cast<std::underlying_type_t<reconnect_backoff>>(b);
  // increment always, so clamp table below works on 'next'
  b = static_cast<reconnect_backoff>(as_num + 1);
  // clamp table
  constexpr std::array<reconnect_backoff, 11> arr{
    reconnect_backoff::none,         reconnect_backoff::wait_1_sec,
    reconnect_backoff::wait_3_sec,   reconnect_backoff::wait_5_sec,
    reconnect_backoff::wait_10_sec,  reconnect_backoff::wait_20_sec,
    reconnect_backoff::wait_30_sec,  reconnect_backoff::wait_60_sec,
    reconnect_backoff::wait_300_sec, reconnect_backoff::wait_600_sec,
    reconnect_backoff::wait_1800_sec};
  for (const reconnect_backoff &x : arr) {
    if (b <= x) {
      b = x;
      return b;
    }
  }
  b = reconnect_backoff::wait_1800_sec;
  return b;
}
template <typename T>
seastar::future<>
reconnect_client<T>::connect() {
  if (client_->is_conn_valid()) { return seastar::make_ready_future<>(); }
  if (reconnect_gate_.is_closed()) { return seastar::make_ready_future<>(); }
  return seastar::with_gate(reconnect_gate_, [this] {
    return client_->reconnect()
      .then([this] { bo_ = reconnect_backoff::none; })
      .handle_exception([this](auto eptr) {
        LOG_INFO("Recovering from '{}' for {}", eptr, client_->server_addr);
        // perform in background; increase the backoff to next version
        (void)seastar::with_gate(reconnect_gate_, [this] {
          // ensure 100ms random jitter
          auto secs =
            std::chrono::milliseconds(rand_.next() % 100) +
            std::chrono::seconds(
              static_cast<std::underlying_type_t<reconnect_backoff>>(++bo_));
          DLOG_TRACE("Sleeping for {}", secs.count());
          return seastar::sleep(secs)
            .then([this] { return connect(); })
            .discard_result();
        });
      });
  });
}

}  // namespace smf

namespace std {
inline ostream &
operator<<(ostream &o, smf::reconnect_backoff b) {
  return o << "smf::reconnect_backoff{ "
           << static_cast<std::underlying_type_t<smf::reconnect_backoff>>(b)
           << "secs }";
}
}  // namespace std
