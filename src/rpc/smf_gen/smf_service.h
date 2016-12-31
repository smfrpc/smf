#pragma once
#include "hashing_utils.h"
#include "rpc/smf_gen/smf_method.h"
#include <flatbuffers/idl.h>

namespace smf_gen {
class smf_service {
  public:
  smf_service(const flatbuffers::ServiceDef *service) : service_(service) {
    id_ = smf::crc_32(service_->name.c_str(), service_->name.length());
  }

  std::string name() const { return service_->name; }
  // hash
  uint32_t service_id() const { return id_; }
  int method_count() const {
    return static_cast<int>(service_->calls.vec.size());
  };

  std::unique_ptr<const smf_method> method(int i) const {
    return std::unique_ptr<const smf_method>(
      new smf_method(service_->calls.vec[i], name(), service_id()));
  };

  private:
  const flatbuffers::ServiceDef *service_;
  uint32_t id_{0};
};
}
