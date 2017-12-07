// Copyright (c) 2016 Alexander Gallego. All rights reserved.
//
#include <gtest/gtest.h>

#include "filesystem/file_size_utils.h"


TEST(file_size_utils, page_size) {
  uint64_t offset    = (4096 * 3) + 234;
  uint64_t alignment = 4096;
  ASSERT_EQ(smf::offset_to_page(offset, alignment), 3);
}
TEST(file_size_utils, first_page) {
  uint64_t offset    = 4096;
  uint64_t alignment = 4096;
  ASSERT_EQ(smf::offset_to_page(offset, alignment), 1);
}

TEST(file_size_utils, zero_page) {
  uint64_t offset    = 0;
  uint64_t alignment = 4096;
  ASSERT_EQ(smf::offset_to_page(offset, alignment), 0);
}

TEST(file_size_utils, front_buffer) {
  uint64_t offset    = 234;  // random offset less than a page
  uint64_t alignment = 4096;
  ASSERT_EQ(smf::offset_to_page(offset, alignment), 0);
}

TEST(file_size_utils, large_page_size) {
  uint64_t offset    = (4096 * 239487) + 234;
  uint64_t alignment = 4096;
  ASSERT_EQ(smf::offset_to_page(offset, alignment), 239487);
}

int
main(int argc, char **argv) {
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
