#include <gtest/gtest.h>
#include "rpc/rpc_envelope.h"

using namespace smf;
using namespace smf::fbs;
using namespace smf::fbs::rpc;
using namespace flatbuffers;


TEST(rpc_envelope, copy_buffer) {
  std::string test = "hello";
  auto p =
    std::make_unique<rpc_envelope>((uint8_t *)test.data(), test.length());
  p->finish();
  ASSERT_TRUE(p->is_finished());
  ASSERT_FALSE(p->is_oneway());


  const auto p_weak =
    flatbuffers::GetRoot<smf::fbs::rpc::Payload>((void *)p->buffer());

  // note at least bigger than smallest
  test = "~~~~~ some nonesense";
  ASSERT_TRUE(memcmp((void *)p_weak->body(), "hello", 5));
}

TEST(rpc_envelope, add_dynamic_header) {
  std::string test = "very long payload to copy";
  auto p =
    std::make_unique<rpc_envelope>((uint8_t *)test.data(), test.length());
  p->add_dynamic_header("User-Agent", "FooBar32");
  p->finish();
  ASSERT_TRUE(p->is_finished());
  ASSERT_FALSE(p->is_oneway());
  auto p_weak =
    flatbuffers::GetRoot<smf::fbs::rpc::Payload>((void *)p->buffer());
  ASSERT_EQ(
    p_weak->dynamic_headers()->LookupByKey("User-Agent")->value()->str(),
    "FooBar32");
}

TEST(rpc_envelope, invalid_payload_size) {
  auto p = std::make_unique<rpc_envelope>(nullptr);
  p->finish();
  ASSERT_TRUE(p->is_finished());
  ASSERT_FALSE(p->is_oneway());
}

TEST(rpc_envelope, header) {
  fbs::rpc::Header hdr(1, (smf::fbs::rpc::Flags)2, 3);
  char hdrcopy[sizeof(hdr)]{0};
  std::memcpy(hdrcopy, (char *)&hdr, sizeof(hdr));
  fbs::rpc::Header *hdr2 = (fbs::rpc::Header *)hdrcopy;
  ASSERT_EQ(hdr2->size(), 1);
  ASSERT_EQ(hdr2->crc32(), 3);
  ASSERT_EQ((int)hdr2->flags(), 2);
}


int main(int argc, char **argv) {
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
