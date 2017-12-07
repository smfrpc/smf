// Copyright (c) 2016 Alexander Gallego. All rights reserved.
//
#include <gtest/gtest.h>

#include "filesystem/wal_name_extractor_utils.h"


TEST(wal_name_extractor_utils_epoch_extractor, basic) {
  ASSERT_EQ(smf::wal_name_extractor_utils::extract_epoch("1234.wal"),
            uint64_t(1234));
  ASSERT_EQ(smf::wal_name_extractor_utils::extract_epoch("999999999.wal"),
            uint64_t(999999999));
}

TEST(wal_name_extractor_utils_epoch_extractor, empty_string) {
  ASSERT_EQ(smf::wal_name_extractor_utils::extract_epoch(""), uint64_t(0));
}


TEST(wal_name_extractor_utils_is_wal_name, basic) {
  ASSERT_TRUE(smf::wal_name_extractor_utils::is_wal_file_name("1.wal"));
}

int
main(int argc, char **argv) {
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
