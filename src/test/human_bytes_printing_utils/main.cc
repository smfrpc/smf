// Copyright (c) 2016 Alexander Gallego. All rights reserved.
//
#include <gtest/gtest.h>
#include <sstream>
#include "utils/human_bytes_printing_utils.h"

TEST(human_bytes_printing_utils, basic) {
  std::stringstream ss;
  smf::human_bytes<int64_t>(ss, 4096);
  ASSERT_EQ(ss.str(), "4 KB");
}

TEST(human_bytes_printing_utils, zero) {
  std::stringstream ss;
  smf::human_bytes<double>(ss, 0);
  ASSERT_EQ(ss.str(), "0.000 bytes");
}

TEST(human_bytes_printing_utils, negataive) {
  std::stringstream ss;
  smf::human_bytes<double>(ss, -12341);
  ASSERT_EQ(ss.str(), "0.000 bytes");
}


TEST(human_bytes_printing_utils, MB) {
  std::stringstream ss;
  smf::human_bytes<int64_t>(ss, uint64_t(1 << 20));
  ASSERT_EQ(ss.str(), "1 MB");
}

TEST(human_bytes_printing_utils, GB) {
  std::stringstream ss;
  smf::human_bytes<uint64_t>(ss, uint64_t(1 << 30));
  ASSERT_EQ(ss.str(), "1 GB");
}

TEST(human_bytes_printing_utils, TB) {
  std::stringstream ss;
  smf::human_bytes<uint64_t>(ss, std::numeric_limits<uint64_t>::max());
  ASSERT_EQ(ss.str(), "17179869183 TB");
}


int main(int argc, char **argv) {
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
