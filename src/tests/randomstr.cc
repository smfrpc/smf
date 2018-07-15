// Copyright 2018 SMF Authors
//

#include <cctype>
#include <utility>

#include <gtest/gtest.h>

#include "smf/random.h"

TEST(bloom, random_alphanum) {
  smf::random r;
  for (auto i = 0u; i < 1000; ++i) {
    auto x = r.next_alphanum(i);
    for (char c : x) {
      ASSERT_TRUE(std::isalnum(c));
    }
  }
}

int
main(int argc, char **argv) {
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
