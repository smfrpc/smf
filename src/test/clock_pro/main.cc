// Copyright (c) 2016 Alexander Gallego. All rights reserved.
//
#include <gtest/gtest.h>
#include <memory>
#include "utils/caching/clock_pro/chunk.h"
#include "utils/caching/clock_pro/clock_pro.h"


TEST(clock_pro_util, int_initialization) {
  smf::clock_pro_cache<uint64_t, uint64_t> c(42);
  for (auto i = 0u; i < c.max_pages; ++i) {
    c.set(i, i * i);
    ASSERT_TRUE(c.contains(i));
    ASSERT_FALSE(c.get_chunk_ptr(i)->ref);
    // get_page modifies the ref count
    ASSERT_TRUE(c.get_page(i)->ref);
    ASSERT_TRUE(c.get_chunk_ptr(i)->ref);
  }
}

TEST(clock_pro_util, ptr_initialization) {
  using v_type = std::unique_ptr<uint64_t>;
  smf::clock_pro_cache<uint64_t, v_type> c(42);
}

static auto get_init_cache() {
  using v_type = std::unique_ptr<uint64_t>;
  using type   = smf::clock_pro_internal::chunk<uint64_t, v_type>;
  smf::clock_pro_cache<uint64_t, v_type> cache(42);

  cache.force_add_hot_chunk(type(24, std::make_unique<uint64_t>(24), false));
  cache.force_add_hot_chunk(type(23, std::make_unique<uint64_t>(23), false));
  cache.force_add_hot_chunk(type(22, std::make_unique<uint64_t>(22), true));
  cache.force_add_hot_chunk(type(21, std::make_unique<uint64_t>(21), false));
  cache.force_add_hot_chunk(type(20, std::make_unique<uint64_t>(20), false));
  cache.force_add_hot_chunk(type(19, std::make_unique<uint64_t>(19), true));
  cache.force_add_hot_chunk(type(18, std::make_unique<uint64_t>(18), false));
  cache.force_add_hot_chunk(type(17, std::make_unique<uint64_t>(17), false));
  cache.force_add_hot_chunk(type(15, std::make_unique<uint64_t>(15), false));
  cache.force_add_hot_chunk(type(13, std::make_unique<uint64_t>(13), false));
  cache.force_add_hot_chunk(type(9, std::make_unique<uint64_t>(9), false));
  cache.force_add_hot_chunk(type(2, std::make_unique<uint64_t>(2), false));


  cache.force_add_cold_chunk(type(5, std::make_unique<uint64_t>(5), false));
  cache.force_add_cold_chunk(type(4, std::make_unique<uint64_t>(4), false));
  cache.force_add_cold_chunk(type(3, std::make_unique<uint64_t>(3), false));
  cache.force_add_cold_chunk(type(1, std::make_unique<uint64_t>(1), false));

  return std::move(cache);
}

// https://www.slideshare.net/huliang64/clockpro
TEST(clock_pro_util, slide_share) {
  auto cache = get_init_cache();

  auto r23 = cache.get_page(23);
  ASSERT_TRUE(r23->ref);

  auto r4 = cache.get_page(4);
  ASSERT_TRUE(r4->ref);

  // MUST evict page 5
  ASSERT_FALSE(cache.contains(25));
  ASSERT_TRUE(cache.contains(5));
  cache.run_cold_hand();
  ASSERT_FALSE(cache.contains(5));
  cache.set(25, std::make_unique<uint64_t>(25));
  ASSERT_TRUE(cache.contains(25));
  ASSERT_FALSE(cache.get_chunk_ptr(25)->ref);  // ensure in cold cache
  cache.fix_hands();

  ASSERT_FALSE(cache.contains(26));  // missing
  cache.run_cold_hand();
  ASSERT_TRUE(cache.get_hot_list()->contains(4));  // 4 is now part of hot
  cache.run_hot_hand();
  ASSERT_TRUE(cache.get_cold_list()->contains(24));  // 24 is now part of hot
  cache.fix_hands();

  ASSERT_TRUE(cache.contains(3));
  cache.run_cold_hand();
  ASSERT_FALSE(cache.contains(3));  // missing
  cache.fix_hands();

  // test setting ref bigs to 0
  ASSERT_FALSE(cache.contains(7));
  cache.run_cold_hand();
  cache.run_hot_hand();
  ASSERT_FALSE(cache.get_hot_list()->get_chunk_ptr(23)->ref);
  ASSERT_FALSE(cache.get_hot_list()->get_chunk_ptr(22)->ref);
  cache.fix_hands();
}

int main(int argc, char **argv) {
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
