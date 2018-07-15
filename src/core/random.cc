// Copyright 2017 Alexander Gallego
//

#include "smf/random.h"

namespace smf {

random::random() {
  std::random_device rd;
  rand_.seed(rd());
}
random::~random() {}

uint64_t
random::next() {
  return dist_(rand_);
}

seastar::sstring
randstr(const seastar::sstring &dict,
        std::uniform_int_distribution<uint32_t> &dist, std::mt19937 &rand,
        uint32_t size) {
  seastar::sstring retval;
  retval.resize(size);
  // this could use a simd
  for (uint32_t i = 0; i < size; ++i) {
    uint32_t idx = dist(rand) % dict.size();
    retval[i] = dict[idx];
  }
  return retval;
}

seastar::sstring
random::next_str(uint32_t size) {
  static const seastar::sstring kDict = "abcdefghijklmnopqrstuvwxyz"
                                        "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
                                        "1234567890"
                                        "!@#$%^&*()"
                                        "`~-_=+[{]}|;:'\",<.>/?";
  static std::uniform_int_distribution<uint32_t> kDictDist(0, kDict.size());
  return randstr(kDict, kDictDist, rand_, size);
}
seastar::sstring
random::next_alphanum(uint32_t size) {
  static const seastar::sstring kDict = "abcdefghijklmnopqrstuvwxyz"
                                        "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
                                        "1234567890";
  static std::uniform_int_distribution<uint32_t> kDictDist(0, kDict.size());
  return randstr(kDict, kDictDist, rand_, size);
}
}  // namespace smf
