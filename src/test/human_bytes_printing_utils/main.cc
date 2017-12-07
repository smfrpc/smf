// Copyright (c) 2016 Alexander Gallego. All rights reserved.
//
#include <gtest/gtest.h>
#include <sstream>

#include "utils/human_bytes.h"

TEST(human_bytes, basic) {
  std::stringstream ss;
  ss << smf::human_bytes(4096);
  ASSERT_EQ(ss.str(), "4.000 KB");
}
TEST(human_bytes, basic_bloat) {
  std::stringstream ss;
  ss << smf::human_bytes(static_cast<double>(1.57));
  ASSERT_EQ(ss.str(), "1.570 bytes");
}
TEST(human_bytes, zero) {
  std::stringstream ss;
  ss << smf::human_bytes(0);
  ASSERT_EQ(ss.str(), "0.000 bytes");
}

TEST(human_bytes, negataive) {
  std::stringstream ss;
  ss << smf::human_bytes(-12341);
  ASSERT_EQ(ss.str(), "0.000 bytes");
}


TEST(human_bytes, MB) {
  std::stringstream ss;
  ss << smf::human_bytes(uint64_t(1 << 20));
  ASSERT_EQ(ss.str(), "1.000 MB");
}

TEST(human_bytes, GB) {
  std::stringstream ss;
  ss << smf::human_bytes(uint64_t(1 << 30));
  ASSERT_EQ(ss.str(), "1.000 GB");
}

TEST(human_bytes, TB) {
  std::stringstream ss;
  ss << smf::human_bytes(std::numeric_limits<uint64_t>::max());
  ASSERT_EQ(ss.str(), "17179869184.000 TB");
}


int
main(int argc, char **argv) {
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
