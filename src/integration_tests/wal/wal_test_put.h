// Copyright 2017 Alexander Gallego
//

#pragma once

#include <algorithm>
#include <memory>

#include <core/reactor.hh>
#include <fastrange.h>

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
  explicit wal_test_put(uint32_t batch_size = 10) {
    uint32_t max_rand_bytes =
      static_cast<uint32_t>(std::numeric_limits<uint8_t>::max());

    auto key   = random().next_str(random().next() % max_rand_bytes);
    auto value = random().next_str(random().next() % max_rand_bytes);
    seastar::sstring topic = "dummy_topic_test";

    DLOG_INFO("Size of key: {}", key.size());
    DLOG_INFO("Size of value: {}", value.size());
    DLOG_INFO("Topic name size: {}", topic.size());


    smf::chains::chain_put_requestT p;
    p.chain_index = 0;
    p.chain.push_back(uint32_t(2130706433));
    p.put        = std::make_unique<smf::wal::tx_put_requestT>();
    p.put->topic = topic;

    const uint64_t fragment_count =
      std::max(uint64_t(1), random().next() % batch_size);
    const uint64_t puts_per_partition =
      std::max(uint64_t(1),
               static_cast<uint64_t>(std::ceil(batch_size / fragment_count)));

    DLOG_INFO("Fragment count: {}, puts_per_partition: {}", fragment_count,
              puts_per_partition);

    for (auto i = 0u; i < puts_per_partition; ++i) {
      auto ptr       = std::make_unique<smf::wal::tx_put_partition_pairT>();
      ptr->partition = fastrange32(static_cast<uint32_t>(random().next()), 8);

      for (auto j = 0u; j < fragment_count; ++j) {
        auto f = std::make_unique<smf::wal::tx_put_fragmentT>();
        // data + commit
        f->op       = smf::wal::tx_put_operation::tx_put_operation_full;
        f->epoch_ms = lowres_time_now_millis();
        f->type     = smf::wal::tx_put_fragment_type::tx_put_fragment_type_kv;
        f->key.resize(key.size());
        f->value.resize(value.size());
        std::memcpy(&f->key[0], &key[0], key.size());
        std::memcpy(&f->value[0], &value[0], value.size());
        ptr->txs.push_back(std::move(f));
      }
      p.put->data.push_back(std::move(ptr));
    }

    auto body = smf::native_table_as_buffer<smf::chains::chain_put_request>(p);
    DLOG_INFO("Encoded request is of size: {}", body.size());
    orig_ =
      std::make_unique<smf::fbs_typed_buf<smf::chains::chain_put_request>>(
        std::move(body));

    // verify_data();
  }

  void verify_data() {
    flatbuffers::Verifier v(
      reinterpret_cast<const uint8_t *>(orig_->buf().get()),
      orig_->buf().size());

    LOG_THROW_IF(!orig_->mutable_ptr()->Verify(v),
                 "the buffer is unverifyable");
  }

  smf::wal_write_request get_request() {
    // verify_data();
    std::set<uint32_t> partitions;
    auto               p = orig_->mutable_ptr()->mutable_put();
    LOG_THROW_IF(p->data() == nullptr, "null pointer to transactions");
    LOG_THROW_IF(p->data()->Length() == 0, "There are no partitions available");

    std::for_each(p->data()->begin(), p->data()->end(),
                  [&partitions, p](const smf::wal::tx_put_partition_pair *it) {
                    // make sure only one core does this
                    auto core = smf::put_to_lcore(p->topic()->c_str(), it);
                    if (core == seastar::engine().cpu_id()) {
                      partitions.insert(it->partition());
                    }
                  });

    return smf::wal_write_request(
      p, smf::priority_manager::thread_local_instance().default_priority(),
      partitions);
  }


 private:
  smf::random                                                         rand_;
  std::unique_ptr<smf::fbs_typed_buf<smf::chains::chain_put_request>> orig_ =
    nullptr;
};

}  // namespace smf
