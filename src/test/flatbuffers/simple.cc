// Copyright (c) 2016 Alexander Gallego. All rights reserved.
//
#include <gtest/gtest.h>

#include "flatbuffers/rpc_generated.h"


TEST(HeaderBufferTest, build_read_header) {
  flatbuffers::FlatBufferBuilder fbb;
  smf::fbs::rpc::Header hdr(987565, smf::fbs::rpc::Flags::Flags_ZSTD, 23423);

  void *                 buf  = reinterpret_cast<void *>(&hdr);
  smf::fbs::rpc::Header *hdr2 = reinterpret_cast<smf::fbs::rpc::Header *>(buf);

  ASSERT_EQ(hdr.size(), hdr2->size());
  ASSERT_EQ(hdr.flags(), hdr2->flags());
  ASSERT_EQ(hdr.checksum(), hdr2->checksum());
}

int main(int argc, char **argv) {
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
