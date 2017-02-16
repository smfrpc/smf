// Copyright (c) 2017 Alexander Gallego. All rights reserved.
//
#include "histogram/histogram_seastar_utils.h"
#include <utility>
// smf
#include "histogram/histogram.h"
#include "platform/log.h"
#include "seastar_io/priority_manager.h"

namespace smf {

future<temporary_buffer<char>> histogram_seastar_utils::print_histogram(
  histogram h) {
  // HdrHistogram/GoogleChartsExample/example1.txt is 5K
  temporary_buffer<char> buf(4096 + 2048);
  FILE *fp = fmemopen(static_cast<void *>(buf.get_write()), buf.size(), "w+");
  LOG_THROW_IF(fp == nullptr, "Failed to allocate filestream");
  h.print(fp);
  fflush(fp);
  auto len = ftell(fp);
  fclose(fp);
  buf.trim(len);
  return make_ready_future<temporary_buffer<char>>(std::move(buf));
}
future<> histogram_seastar_utils::write_histogram(sstring   filename,
                                                  histogram h) {
  return open_file_dma(
           filename, open_flags::rw | open_flags::create | open_flags::truncate)
    .then([h = std::move(h)](file seastar_file) mutable {
      auto f = make_lw_shared<output_stream<char>>(
        make_file_output_stream(std::move(seastar_file)));
      return histogram_seastar_utils::print_histogram(std::move(h))
        .then([f](temporary_buffer<char> buf) {
          return f->write(buf.get(), buf.size()).then([f]() mutable {
            return f->flush().then(
              [f]() mutable { return f->close().finally([f] {}); });
          });
        });
    });
}

}  // namespace smf
