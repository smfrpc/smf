// Copyright (c) 2016 Alexander Gallego. All rights reserved.
//
// seastar
#include <core/app-template.hh>
#include <core/distributed.hh>
// smf
#include "filesystem/wal_pretty_print_utils.h"
#include "filesystem/wal_reader.h"
#include "filesystem/wal_requests.h"
#include "filesystem/wal_writer.h"
#include "platform/log.h"
#include "seastar_io/priority_manager.h"

smf::wal_write_request gen_payload() {
  static const sstring kPayload =
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
  temporary_buffer<char> buf(kPayload.size());
  std::copy(kPayload.begin(), kPayload.end(), buf.get_write());
  return smf::wal_write_request(
    0, std::move(buf),
    smf::priority_manager::thread_local_instance().default_priority());
}
future<> write(uint32_t max) {
  auto wstats = make_lw_shared<smf::writer_stats>();
  auto writer = make_lw_shared<smf::wal_writer>(".", wstats.get());

  return writer->open()
    .then([wstats, writer, max] {
      return do_for_each(
        boost::counting_iterator<uint32_t>(0),
        boost::counting_iterator<uint32_t>(max), [wstats, writer](auto i) {
          return writer->append(gen_payload()).then([writer](auto epoch) {
            return make_ready_future<>();
          });
        });
    })
    .then([writer, wstats] {
      return writer->close().finally([writer, wstats] {});
    });
}

smf::wal_read_request gen_read_request() {
  static const ::io_priority_class &pc =
    smf::priority_manager::thread_local_instance().default_priority();
  return smf::wal_read_request(uint64_t(0), uint64_t(1000000), uint32_t(0), pc);
}


future<> read(uint32_t expected_heder_count) {
  auto rstats = make_lw_shared<smf::reader_stats>();
  auto reader = make_lw_shared<smf::wal_reader>(".", rstats.get());
  return reader->open()
    .then([reader, rstats, expected_heder_count]() mutable {
      auto r = gen_read_request();
      DLOG_INFO("Read request: {}", r);
      return reader->get(r).then([reader, expected_heder_count](auto maybe) {
        DLOG_THROW_IF(!maybe, "eeeee... supposed to have some");
        DLOG_INFO("fragments: {}", maybe->fragments.size());
        DLOG_THROW_IF(expected_heder_count != maybe->fragments.size(),
                      "fragment size does not match expected fragment count");
        DLOG_INFO("Size of read request: {}", maybe->size());
        return make_ready_future<>();
      });
    })
    .then([reader, rstats] {
      return reader->close().finally([reader, rstats] {});
    });
}

static const uint32_t kNumberOfAppends = 1000;
int main(int args, char **argv, char **env) {
  app_template app;
  try {
    return app.run(args, argv, [&]() mutable -> future<int> {
      return write(kNumberOfAppends)
        .then([] { return read(kNumberOfAppends); })
        .then([] { return make_ready_future<int>(0); });
    });
  } catch (const std::exception &e) {
    std::cerr << "Fatal exception: " << e.what() << std::endl;
  }
}
