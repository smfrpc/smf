#pragma once
#include <flatbuffers/idl.h>
#include "hashing_utils.h"
namespace smf_gen {
class smf_method {
  public:
  enum Streaming { kNone, kClient, kServer, kBiDi };
  smf_method(const flatbuffers::RPCCall *method, uint32_t service_id)
    : method_(method), service_id_(service_id) {
    streaming_ = kNone;
    id_ = smf::crc_32(method_->name.c_str(), method_->name.length());
  }
  std::string name() const { return method_->name; }
  uint32_t method_id() const { return id_; }
  uint32_t service_id() const { return service_id_; }
  std::string type(const flatbuffers::StructDef &sd) const {
    return sd.name;
  }
  std::string input_type_name() const { return type(*method_->request); }
  std::string output_type_name() const { return type(*method_->response); }
  private:
  const flatbuffers::RPCCall *method_;
  Streaming streaming_;
  uint32_t service_id_;
  uint32_t id_;
};
}
