#include <gtest/gtest.h>
// seastar
#include <core/sstring.hh>
// smf
#include "filesystem/wal_head_file_functor.h"
using namespace smf;

TEST(wal_head_file_functor_min, comparator) {
  wal_head_file_min_comparator c;
  const sstring kMin("smf0_0.wal");
  for(uint32_t i = 1; i < 10; ++i) {
    ASSERT_FALSE(c(kMin, sstring("smf0_") + to_sstring(i * i) + ".wal"));
  }
}
TEST(wal_head_file_functor_min, nil) {
  wal_head_file_min_comparator c;
  ASSERT_TRUE(c("smf0_0.wal", ""));
}
TEST(wal_head_file_functor_max, comparator) {
  wal_head_file_max_comparator c;
  const sstring kMin("smf0_0.wal");
  for(uint32_t i = 1; i < 10; ++i) {
    ASSERT_TRUE(c(kMin, sstring("smf0_") + to_sstring(i * i) + ".wal"));
  }
}
TEST(wal_head_file_functor_max, nil) {
  wal_head_file_max_comparator c;
  ASSERT_TRUE(c("smf0_0.wal", ""));
}


int main(int argc, char **argv) {
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
