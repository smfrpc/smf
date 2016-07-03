#pragma once
// smf
#include "rpc/rpc_envelope.h"
#include "rpc/rpc_recv_context.h"
#include "rpc/rpc_service.h"

namespace smf {
/// \brief used to host many services
/// multiple services can use this class to handle the routing for them
///
class rpc_handle_router {
  public:
  /// \brief, MUST BE FAST - blocks the thread
  /// there will be a header specifying the requested resource
  /// use LookupByKey - as headers are guaranteed to be sorted
  /// feel free to use std::binary_search otherwise
  /// \code
  ///     dynamic_headers()->LookupByKey("User-Agent")->value()->str()
  /// \endcode
  /// \param request_id - the request from the rpc_envelope&
  /// \param hdrs - a pointer at the header metadata associated w/ this request
  ///
  virtual bool can_handle_request(
    const uint32_t &request_id,
    const flatbuffers::Vector<flatbuffers::Offset<fbs::rpc::DynamicHeader>>
      *hdrs);
  /// \brief actually makes the router dispatch
  virtual future<rpc_envelope> handle(rpc_recv_context &&recv);

  /// \brief multiple rpc_services can register w/ this  handle router
  void register_rpc_service(rpc_service *s);

  private:
  std::unordered_map<uint32_t, rpc_service_method_handle> dispatch_{};
  std::vector<std::unique_ptr<rpc_service>> services_{};
};
}
