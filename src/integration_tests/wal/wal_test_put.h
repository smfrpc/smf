// Copyright 2017 Alexander Gallego
//

#pragma once

#include <algorithm>
#include <memory>

#include <core/reactor.hh>
#include <core/sstring.hh>

#include <fastrange/fastrange.h>

#include "filesystem/wal_lcore_map.h"
#include "filesystem/wal_requests.h"
#include "flatbuffers/chain_replication_generated.h"
#include "flatbuffers/fbs_typed_buf.h"
#include "flatbuffers/native_type_utils.h"
#include "flatbuffers/wal_generated.h"
#include "hashing/hashing_utils.h"
#include "platform/log.h"
#include "rpc/rpc_letter.h"
#include "seastar_io/priority_manager.h"
#include "utils/random.h"
#include "utils/time_utils.h"

namespace smf {

class wal_test_put {
 public:
  static constexpr uint32_t max_rand_bytes =
    static_cast<uint32_t>(std::numeric_limits<uint8_t>::max());


  explicit wal_test_put(uint32_t _batch_size, uint32_t _fragment_count)
    : batch_size(_batch_size), fragment_count(_fragment_count) {
    partition_             = static_cast<uint32_t>(rand_.next());
    seastar::sstring key   = rand_.next_str(rand_.next() % max_rand_bytes);
    seastar::sstring value = rand_.next_str(rand_.next() % max_rand_bytes);
    seastar::sstring topic = "dummy_topic_test";

    DLOG_INFO("Size of key: {}", key.size());
    DLOG_INFO("Size of value: {}", value.size());
    DLOG_INFO("Topic name size: {}", topic.size());


    auto ptr   = std::make_unique<smf::wal::tx_put_requestT>();
    ptr->topic = topic;
    ptr->data.push_back(std::make_unique<smf::wal::tx_put_partition_tupleT>());
    auto &tuple      = *ptr->data.begin();
    tuple->partition = partition_;

    for (auto j = 0u; j < fragment_count; ++j) {
      auto f = std::make_unique<smf::wal::tx_put_fragmentT>();
      // data + commit
      f->op       = smf::wal::tx_put_operation::tx_put_operation_full;
      f->epoch_ms = time_now_millis();
      f->type     = smf::wal::tx_put_fragment_type::tx_put_fragment_type_kv;
      f->key.resize(key.size());
      f->value.resize(value.size());
      std::memcpy(&f->key[0], &key[0], key.size());
      std::memcpy(&f->value[0], &value[0], value.size());
      tuple->txs.push_back(std::move(f));
    }

    auto body = smf::native_table_as_buffer<smf::wal::tx_put_request>(*ptr);
    DLOG_INFO("Encoded request is of size: {}", body.size());
    orig_ = std::make_unique<smf::fbs_typed_buf<smf::wal::tx_put_request>>(
      std::move(body));
  }

  smf::wal_write_request
  get_request(const uint32_t assigned_core, const uint32_t runner_core) {
    auto p = orig_->get();
    return wal_write_request(
      p, smf::priority_manager::get().streaming_write_priority(), assigned_core,
      runner_core, {partition_});
  }

  uint32_t
  get_partition() const {
    return partition_;
  }

  const uint32_t batch_size;
  const uint32_t fragment_count;


 private:
  uint32_t                                                      partition_;
  smf::random                                                   rand_;
  std::unique_ptr<smf::fbs_typed_buf<smf::wal::tx_put_request>> orig_ = nullptr;
};

}  // namespace smf
