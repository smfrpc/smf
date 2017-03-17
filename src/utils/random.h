// Copyright 2017 Alexander Gallego
//
#pragma once
#include <random>
#include <string>

namespace smf {
class random {
 public:
  random();
  ~random();

  uint64_t next();

  std::basic_string<char> next_str(uint32_t size);

 private:
  std::mt19937 rand_;
  // by default range [0, MAX]
  std::uniform_int_distribution<uint64_t> dist_;
};

}  // namespace smf
