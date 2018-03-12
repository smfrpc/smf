// Copyright (c) 2016 Alexander Gallego. All rights reserved.
//
#pragma once
#include <flatbuffers/idl.h>

#include "smfc/crc.h"
#include "smfc/smf_method.h"

namespace smf_gen {
class smf_service {
 public:
  explicit smf_service(const flatbuffers::ServiceDef *service)
    : service_(service) {
    id_ = crc32(service_->name.c_str(), service_->name.length());
  }

  std::string
  name() const {
    return service_->name;
  }
  // hash
  uint32_t
  service_id() const {
    return id_;
  }
  int
  method_count() const {
    return static_cast<int>(service_->calls.vec.size());
  }

  std::unique_ptr<const smf_method>
  method(int i) const {
    return std::unique_ptr<const smf_method>(
      new smf_method(service_->calls.vec[i], name(), service_id()));
  }

 private:
  const flatbuffers::ServiceDef *service_;
  uint32_t id_{0};
};
}  // namespace smf_gen
