// Copyright (c) 2017 Alexander Gallego. All rights reserved.
//
#include "histogram/histogram_seastar_utils.h"

#include <utility>

#include <core/reactor.hh>
#include <core/file.hh>
// smf
#include "histogram/histogram.h"
#include "platform/log.h"

namespace smf {

seastar::future<seastar::temporary_buffer<char>>
histogram_seastar_utils::print_histogram(histogram *h) {
  // HdrHistogram/GoogleChartsExample/example1.txt is 5K
  seastar::temporary_buffer<char> buf(4096 + 2048);
  FILE *fp = fmemopen(static_cast<void *>(buf.get_write()), buf.size(), "w+");
  LOG_THROW_IF(fp == nullptr, "Failed to allocate filestream");
  h->print(fp);
  fflush(fp);
  auto len = ftell(fp);
  fclose(fp);
  buf.trim(len);
  return seastar::make_ready_future<seastar::temporary_buffer<char>>(
    std::move(buf));
}
seastar::future<>
histogram_seastar_utils::write_histogram(seastar::sstring filename,
                                         histogram *      h) {
  return open_file_dma(filename, seastar::open_flags::rw |
                                   seastar::open_flags::create |
                                   seastar::open_flags::truncate)
    .then([h = std::move(h)](seastar::file file) mutable {
      auto f = seastar::make_lw_shared<seastar::output_stream<char>>(
        seastar::make_file_output_stream(std::move(file)));
      return histogram_seastar_utils::print_histogram(h).then(
        [f](seastar::temporary_buffer<char> buf) {
          return f->write(buf.get(), buf.size()).then([f]() mutable {
            return f->flush().then(
              [f]() mutable { return f->close().finally([f] {}); });
          });
        });
    });
}

}  // namespace smf
