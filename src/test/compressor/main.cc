// Copyright (c) 2016 Alexander Gallego. All rights reserved.
//
#include <gtest/gtest.h>
#include <memory>

#include <core/byteorder.hh>
#include <core/sstring.hh>

#include "utils/compression.h"

using namespace smf;  // NOLINT

static const char *kPoem = "How do I love thee? Let me count the ways."
                           "I love thee to the depth and breadth and height"
                           "My soul can reach, when feeling out of sight"
                           "For the ends of being and ideal grace."
                           "I love thee to the level of every day's"
                           "Most quiet need, by sun and candle-light."
                           "I love thee freely, as men strive for right."
                           "I love thee purely, as they turn from praise."
                           "I love thee with the passion put to use"
                           "In my old griefs, and with my childhood's faith."
                           "I love thee with a love I seemed to lose"
                           "With my lost saints. I love thee with the breath,"
                           "Smiles, tears, of all my life; and, if God choose,"
                           "I shall but love thee better after death."
                           "How do I love thee? Let me count the ways."
                           "I love thee to the depth and breadth and height"
                           "My soul can reach, when feeling out of sight"
                           "For the ends of being and ideal grace."
                           "I love thee to the level of every day's"
                           "Most quiet need, by sun and candle-light."
                           "I love thee freely, as men strive for right."
                           "I love thee purely, as they turn from praise."
                           "I love thee with the passion put to use"
                           "In my old griefs, and with my childhood's faith."
                           "I love thee with a love I seemed to lose"
                           "With my lost saints. I love thee with the breath,"
                           "Smiles, tears, of all my life; and, if God choose,"
                           "I shall but love thee better after death.";

TEST(lz4, compresion_decompression) {
  auto compressor =
    codec::make_unique(codec_type::lz4, compression_level::fastest);
  auto c_payload = compressor->compress(kPoem, strlen(kPoem));
  auto d_payload = compressor->uncompress(c_payload);
  ASSERT_TRUE(std::memcmp(d_payload.get(), kPoem, strlen(kPoem)) == 0);
}
TEST(zstd, compresion_decompression) {
  auto compressor =
    codec::make_unique(codec_type::zstd, compression_level::fastest);
  auto c_payload = compressor->compress(kPoem, strlen(kPoem));
  auto d_payload = compressor->uncompress(c_payload);
  ASSERT_TRUE(std::memcmp(d_payload.get(), kPoem, strlen(kPoem)) == 0);
}


int
main(int argc, char **argv) {
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
