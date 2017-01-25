// Copyright (c) 2016 Alexander Gallego. All rights reserved.
//
#include <gtest/gtest.h>
#include "filesystem/wal_name_extractor_utils.h"


TEST(wal_epoch_extractor, basic) {
  ASSERT_EQ(smf::extract_epoch("smf0_1234.wal"), uint64_t(1234));
  ASSERT_EQ(smf::extract_epoch("smf0_999999999.wal"), uint64_t(999999999));
}

TEST(wal_epoch_extractor, empty_string) {
  ASSERT_EQ(smf::extract_epoch(""), uint64_t(0));
}

int main(int argc, char **argv) {
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
