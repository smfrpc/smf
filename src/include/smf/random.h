// Copyright 2017 Alexander Gallego
//
#pragma once

#include <cstdint>
#include <random>

#include <seastar/core/sstring.hh>

namespace smf {
class random {
 public:
  random();
  ~random();

  uint64_t next();

  seastar::sstring next_str(uint32_t size);
  seastar::sstring next_alphanum(uint32_t size);

 private:
  std::mt19937 rand_;
  // by default range [0, MAX]
  std::uniform_int_distribution<uint64_t> dist_;
};

}  // namespace smf
