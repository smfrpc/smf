#include <iostream>
// #include <boost/endian/conversion.hpp>
#include "rpc/rpc.hh"


// should be stateless. becaused it's basically
// passed in as the serialization to ALL methods. Hold no state!!!
// - maybe good for metrics?
struct person_serializer {};
struct person {
  sstring name;
};
// For each serializable type T, implement, read and write
// the type_tag is used to dissambiguate in the compiler, not really used
template <typename Output>
void write(const person_serializer &, Output &output, const person &data) {
  //[int32(size_of_payload)][payload]
  uint32_t size = data.name.size();
  out.write(reinterpret_cast<const char *>(&size), sizeof(size));
  output.write(data.name.c_str(), data.name.size());
}
template <typename Input>
person
read(const person_serializer &, Input &input, rpc::type<person> type_tag) {
  person p;
  uint32_t size = 0;
  input.read(reinterpret_cast<char*>(&size), sizeof(size));
  p.name = sstring(sstring::initialized_later(), size);
  input.read(p.name.begin(), size);
  return p;
}
