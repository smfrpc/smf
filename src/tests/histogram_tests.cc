// Copyright 2018 SMF Authors
//

#include <cctype>
#include <utility>

#include <gtest/gtest.h>

#include "smf/histogram.h"
#include "smf/random.h"

static constexpr const int64_t kMaxValue = 10240;

TEST(histogram, simple) {
  smf::random r;
  auto h = smf::histogram::make_unique(kMaxValue);
  for (auto i = 0u; i < 1000; ++i) {
    auto x = r.next() % kMaxValue;
    h->record_corrected(x, kMaxValue);
  }
}

int
main(int argc, char **argv) {
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
