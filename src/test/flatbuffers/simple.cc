#include <gtest/gtest.h>

#include "flatbuffers/rpc_generated.h"

using namespace smf;
using namespace smf::fbs;
using namespace smf::fbs::rpc;
using namespace flatbuffers;


TEST(HeaderBufferTest, build_read_header) {
  FlatBufferBuilder fbb;
  Header hdr(987565, Flags::Flags_ZSTD, 23423);
  void *buf = (void *)&hdr;
  std::cout << "sizeof smf::fbs::rpc::Header: " << sizeof(Header) << std::endl;
  Header *hdr2 = (Header *)(buf);
  ASSERT_EQ(hdr.size(), hdr2->size());
  ASSERT_EQ(hdr.flags(), hdr2->flags());
  ASSERT_EQ(hdr.checksum(), hdr2->checksum());
}

int main(int argc, char **argv) {
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
