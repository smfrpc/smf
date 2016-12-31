#include "filesystem/wal_name_parser.h"
#include <gtest/gtest.h>

using namespace smf;

TEST(wal_name_parser, basic) {
  wal_name_parser parser("smf");
  ASSERT_TRUE(parser("smf0_0.wal"));
}

int main(int argc, char **argv) {
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
