#include <gtest/gtest.h>
#include "rpc/rpc_request.h"

using namespace smf;
using namespace smf::fbs;
using namespace smf::fbs::rpc;
using namespace flatbuffers;


TEST(rpc_request, copy_buffer) {
  std::string test = "hello";
  auto p = std::make_unique<rpc_request>((uint8_t *)test.data(), test.length());
  p->finish();
  ASSERT_TRUE(p->is_finished());
  ASSERT_FALSE(p->is_oneway());


  const auto p_weak =
    flatbuffers::GetRoot<smf::fbs::rpc::Payload>((void *)p->buffer());

  // note at least bigger than smallest
  test = "~~~~~ some nonesense";
  ASSERT_TRUE(memcmp((void *)p_weak->body(), "hello", 5));
}

TEST(rpc_request, add_dynamic_header) {
  std::string test = "very long payload to copy";
  auto p = std::make_unique<rpc_request>((uint8_t *)test.data(), test.length());
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

int main(int argc, char **argv) {
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
