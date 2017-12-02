// Copyright (c) 2016 Alexander Gallego. All rights reserved.
//
#include <gtest/gtest.h>

#include "filesystem/wal_name_extractor_utils.h"


TEST(wal_name_extractor_utils_epoch_extractor, basic) {
  ASSERT_EQ(smf::wal_name_extractor_utils::extract_epoch("1:0:1234.wal"),
            uint64_t(1234));
  ASSERT_EQ(smf::wal_name_extractor_utils::extract_epoch("1:0:999999999.wal"),
            uint64_t(999999999));
}

TEST(wal_name_extractor_utils_epoch_extractor, empty_string) {
  ASSERT_EQ(smf::wal_name_extractor_utils::extract_epoch(""), uint64_t(0));
}


TEST(wal_name_extractor_utils_epoch_extractor, with_locked_file) {
  ASSERT_EQ(smf::wal_name_extractor_utils::extract_epoch("locked:1:0:1234.wal"),
            uint64_t(1234));
  ASSERT_EQ(smf::wal_name_extractor_utils::extract_epoch("locked:1:0:9999.wal"),
            uint64_t(9999));
}

TEST(wal_name_extractor_utils_core_extractor, basic) {
  ASSERT_EQ(smf::wal_name_extractor_utils::extract_core("1:0:1234.wal"),
            uint64_t(0));
  ASSERT_EQ(smf::wal_name_extractor_utils::extract_core("1:9999:0.wal"),
            uint64_t(9999));
}

TEST(wal_name_extractor_utils_core_extractor, empty) {
  ASSERT_EQ(smf::wal_name_extractor_utils::extract_core(""), uint64_t(0));
}


TEST(wal_name_extractor_utils_core_extractor, with_locked_file) {
  ASSERT_EQ(smf::wal_name_extractor_utils::extract_core("locked:1:0:1234.wal"),
            uint64_t(0));
  ASSERT_EQ(smf::wal_name_extractor_utils::extract_core("locked:1:9999:0.wal"),
            uint64_t(9999));
}


TEST(wal_name_extractor_utils_prefix_cleaner, with_locked_file) {
  ASSERT_EQ(
    smf::wal_name_extractor_utils::name_without_prefix("locked:1:0:1234.wal"),
    seastar::sstring("1:0:1234.wal"));
  ASSERT_EQ(
    smf::wal_name_extractor_utils::name_without_prefix("locked:1:9999:0.wal"),
    seastar::sstring("1:9999:0.wal"));
}
TEST(wal_name_extractor_utils_is_wal_name, basic) {
  ASSERT_TRUE(
    smf::wal_name_extractor_utils::is_wal_file_name("locked:1:0:0.wal"));
}

int main(int argc, char **argv) {
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
