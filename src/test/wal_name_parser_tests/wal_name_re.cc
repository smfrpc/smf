#include <gtest/gtest.h>
#include "filesystem/wal_name_parser.h"

using namespace smf;

TEST(wal_name_parser, basic) {
  wal_name_parser parser("smf");
  ASSERT_TRUE(parser("smf0_0_2016-11-20T23:25:15.wal"));
}

int main(int argc, char **argv) {
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
