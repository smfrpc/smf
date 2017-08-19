// Copyright (c) 2016 Alexander Gallego. All rights reserved.
//
#include <core/sstring.hh>
#include <gtest/gtest.h>

#include "flatbuffers/rpc_generated.h"

TEST(HeaderBufferTest, build_read_header) {
  flatbuffers::FlatBufferBuilder fbb;

  smf::rpc::Header hdr(987565, smf::rpc::compression_flags_zstd,
                       smf::rpc::validation_flags_checksum, 23423);

  void *buf = reinterpret_cast<void *>(&hdr);

  smf::rpc::Header *hdr2 = reinterpret_cast<smf::rpc::Header *>(buf);

  ASSERT_EQ(hdr.size(), hdr2->size());
  ASSERT_EQ(hdr.validation_flags(), hdr2->validation_flags());
  ASSERT_EQ(hdr.compression_flags(), hdr2->compression_flags());
  ASSERT_EQ(hdr.checksum(), hdr2->checksum());
}

int main(int argc, char **argv) {
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
