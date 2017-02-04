// Copyright (c) 2016 Alexander Gallego. All rights reserved.
//
#include <gtest/gtest.h>
#include "filesystem/wal_name_parser.h"

TEST(wal_name_parser, basic) {
  smf::wal_name_parser parser;
  ASSERT_TRUE(parser("core:0:locked:smf0_0.wal"));
}

int main(int argc, char **argv) {
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
