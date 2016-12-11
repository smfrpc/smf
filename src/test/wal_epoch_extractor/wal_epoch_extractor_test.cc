#include <gtest/gtest.h>
#include "filesystem/wal_name_extractor_utils.h"

using namespace smf;

TEST(wal_epoch_extractor, basic) {
  ASSERT_EQ(extract_epoch("smf0_1234_2016-11-20T23:25:15.wal"), uint64_t(1234));
  ASSERT_EQ(extract_epoch("smf0_999999999_2016-11-20T23:25:15.wal"),
            uint64_t(999999999));
  ASSERT_EQ(extract_epoch("smf0_1234"), uint64_t(0));
}

TEST(wal_epoch_extractor, empty_string) {
  ASSERT_EQ(extract_epoch(""), uint64_t(0));
}

int main(int argc, char **argv) {
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
