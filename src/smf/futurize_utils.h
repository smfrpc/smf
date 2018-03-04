// Copyright 2017 Alexander Gallego
//
#pragma once

namespace smf {

/// \brief used in almost all RPC applications like
/// \code
///    if (!record) {
///       return futurize_status_for_type<smf::chains::tx_put_reply>(400);
///    }
/// \endcode
template <typename T>
auto
futurize_status_for_type(uint32_t status) {
  using t = smf::rpc_typed_envelope<T>;
  t data;
  data.envelope.set_status(status);
  return seastar::make_ready_future<t>(std::move(data));
}

}  // namespace smf
