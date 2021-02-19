// Copyright (c) 2017 Alexander Gallego. All rights reserved.
//
#include "smf/histogram_seastar_utils.h"

#include <utility>

#include <seastar/core/file.hh>
#include <seastar/core/reactor.hh>
#include <seastar/core/fstream.hh>
// smf
#include "smf/histogram.h"
#include "smf/log.h"

namespace smf {

seastar::future<seastar::temporary_buffer<char>>
histogram_seastar_utils::print_histogram(histogram *h) {
  // HdrHistogram/GoogleChartsExample/example1.txt is 5K
  char *buf;
  std::size_t len;
  FILE *fp = open_memstream(&buf, &len);
  LOG_THROW_IF(fp == nullptr, "Failed to allocate filestream");
  h->print(fp);
  // MUST fflush in order to have len update
  fflush(fp);
  fclose(fp);

  auto ret =
    seastar::temporary_buffer<char>(buf, len, seastar::make_free_deleter(buf));
  return seastar::make_ready_future<decltype(ret)>(std::move(ret));
}
seastar::future<>
histogram_seastar_utils::write_histogram(seastar::sstring filename,
                                         histogram *h) {
  auto flags = seastar::open_flags::rw | seastar::open_flags::create
                 | seastar::open_flags::truncate;

  return seastar::with_file_close_on_failure(
    seastar::open_file_dma(filename, flags),
    [h = std::move(h)](seastar::file file) mutable {
      return  seastar::make_file_output_stream(std::move(file))
        .then([h = std::move(h)](seastar::output_stream<char> out) mutable {
          return histogram_seastar_utils::print_histogram(h)
            .then([out = std::move(out)](seastar::temporary_buffer<char> buf) mutable {
              return out.write(buf.get(), buf.size())
                .then([out = std::move(out)]() mutable {
                  return out.flush()
                    .then([out = std::move(out)]() mutable { 
                      return out.close().finally([out = std::move(out)] {}); 
                    });
                });
            });
        });
    });
}

}  // namespace smf
