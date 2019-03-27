// Copyright (c) 2016 Alexander Gallego. All rights reserved.
//
#pragma once
#include <deque>
#include <memory>

#include <boost/algorithm/string/join.hpp>
#include <flatbuffers/idl.h>

#include "crc.h"
#include "language.h"

namespace smf_gen {
class smf_method {
 public:
  enum Streaming { kNone, kClient, kServer, kBiDi };

  smf_method(const flatbuffers::RPCCall *method, std::string service_name,
             uint32_t service_id)
    : method_(method), service_name_(service_name), service_id_(service_id) {
    streaming_ = kNone;
    // you can have the same method name w/ different arguments, so in that
    // case you should change the hash id
    std::string method_id_str =
      method_->name + ":" + input_type_name() + ":" + output_type_name();
    id_ = crc32(method_id_str.c_str(), method_id_str.length());
  }
  std::string
  name() const {
    return method_->name;
  }
  uint32_t
  method_id() const {
    return id_;
  }

  std::string
  service_name() const {
    return service_name_;
  }
  uint32_t
  service_id() const {
    return service_id_;
  }

  std::string
  type(const flatbuffers::StructDef &sd, language lang) const {
    switch (lang) {
    case language::cpp: {
      std::deque<std::string> tmp(sd.defined_namespace->components.begin(),
                                  sd.defined_namespace->components.end());
      tmp.push_back(sd.name);
      return boost::algorithm::join(tmp, "::");
    }
    case language::go:
      return sd.name;
    case language::python: {
      std::deque<std::string> tmp(sd.defined_namespace->components.begin(),
                                  sd.defined_namespace->components.end());
      tmp.push_back(sd.name);
      tmp.push_back(sd.name);  // name is inside module
      return boost::algorithm::join(tmp, ".");
    }
    default:
      throw std::runtime_error("Unknown language for the method.h");
    }
  }

  std::string
  input_type_name(language l = language::cpp) const {
    return type(*method_->request, l);
  }
  std::string
  output_type_name(language l = language::cpp) const {
    return type(*method_->response, l);
  }

 private:
  const flatbuffers::RPCCall *method_;
  const std::string service_name_;
  const uint32_t service_id_;
  Streaming streaming_;

  uint32_t id_;
};
}  // namespace smf_gen
