// Copyright (c) 2016 Alexander Gallego. All rights reserved.
//
#pragma once
#include <memory>
#include <string>

#include <flatbuffers/idl.h>

#include "crc.h"
#include "smf_method.h"

namespace smf_gen {
class smf_service {
 public:
  explicit smf_service(const flatbuffers::ServiceDef *service)
    : service_(service) {
    id_ = crc32(service_->name.c_str(), service_->name.length());

    // populate methods
    for (auto i = 0u; i < service_->calls.vec.size(); ++i) {
      methods_.emplace_back(std::make_unique<smf_method>(service_->calls.vec[i],
                                                         name(), service_id()));
    }
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

  const std::vector<std::unique_ptr<smf_method>> &
  methods() const {
    return methods_;
  }

  const flatbuffers::ServiceDef *
  raw_service() const {
    return service_;
  };

 private:
  const flatbuffers::ServiceDef *service_;
  uint32_t id_{0};
  std::vector<std::unique_ptr<smf_method>> methods_;
};
}  // namespace smf_gen
