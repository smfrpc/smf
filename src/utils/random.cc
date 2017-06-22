// Copyright 2017 Alexander Gallego
//

#include "utils/random.h"

namespace smf {

random::random() {
  std::random_device rd;
  rand_.seed(rd());
}
random::~random() {}

uint64_t random::next() { return dist_(rand_); }

seastar::sstring random::next_str(uint32_t size) {
  static const std::string kDict = "abcdefghijklmnopqrstuvwxyz"
                                   "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
                                   "1234567890"
                                   "!@#$%^&*()"
                                   "`~-_=+[{]}\\|;:'\",<.>/? ";
  static std::uniform_int_distribution<uint32_t> kDictDist(0, kDict.size());

  seastar::sstring retval;
  retval.resize(size);
  // this could use a simd
  for (uint32_t i = 0; i < size; ++i) {
    uint32_t idx = kDictDist(rand_);
    retval[i]    = kDict[idx];
  }
  return std::move(retval);
}

}  // namespace smf
