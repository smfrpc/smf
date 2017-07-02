// Copyright (c) 2016 Alexander Gallego. All rights reserved.
//


/*
  // TODO (add back when we finish the chain replication tests)
#include <core/app-template.hh>

#include <flatbuffers/flatbuffers.h>

#include "filesystem/wal_lcore_map.h"
#include "platform/log.h"
#include "utils/random.h"

template <typename T>
seastar::temporary_buffer<char> type_as_array(
  const typename T::NativeTableType &t) {
  flatbuffers::FlatBufferBuilder fbb{};

  fbb.Finish(T::Pack(fbb, &t, nullptr));

  seastar::temporary_buffer<char> vec(fbb.GetSize());
  const char *ptr = reinterpret_cast<const char *>(fbb.GetBufferPointer());
  std::copy(ptr, ptr + fbb.GetSize(), vec.get_write());
  return std::move(vec);
}


smf::chains::tx_put_requestT get_put(seastar::sstring topic,
                                     uint32_t         partition) {
  smf::chains::tx_put_requestT p;
  p.topic     = topic;
  p.partition = partition;
  p.chain.push_back(uint32_t(2130706433));
p.txs.clear();
return std::move(p);
}

smf::chains::tx_get_requestT get_read(seastar::sstring topic,
                                      uint32_t         partition) {
  smf::chains::tx_get_requestT r;
  r.topic     = topic;
  r.offset    = 0;
  r.partition = partition;
  r.max_bytes = 1 << 20;
  return std::move(r);
}

int main(int argc, char **argv) {
  seastar::app_template app;
  return app.run(argc, argv, [&]() mutable -> seastar::future<int> {
    smf::random rand;
    auto        topic     = rand.next_str(32);
    auto        partition = static_cast<uint32_t>(rand.next());

    auto r = get_read(topic, partition);
    auto p = get_put(topic, partition);

    auto read  = typeAsArray<smf::chains::tx_get_request>(r);
    auto write = typeAsArray<smf::chains::tx_put_request>(p);


    LOG_INFO("Read size: {}", read.size());
    LOG_INFO("Write size: {}", write.size());

    auto put_req = flatbuffers::GetMutableRoot<smf::chains::tx_put_request>(
      write.get_write());
    auto get_req = flatbuffers::GetMutableRoot<smf::chains::tx_get_request>(
      read.get_write());

    if (smf::chains::put_to_lcore(put_req)
        != smf::chains::get_to_lcore(get_req)) {
      throw std::runtime_error("Get and Put hashing did not match");
    }

    return seastar::make_ready_future<int>(0);
  });
}
*/
int main(int argc, char **argv) { return 0; }
