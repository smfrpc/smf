#pragma once
// std
#include <cstdio>
// seastar
#include <core/fstream.hh>
// smf
#include "histogram.h"
#include "log.h"
#include "priority_manager.h"


// TODO(agallego) - add a .cc file for this now that it works ! yay!
//
namespace smf {
struct histogram_seastar_utils {
  static future<temporary_buffer<char>> print_histogram(histogram h) {
    // HdrHistogram/GoogleChartsExample/example1.txt is 5K
    temporary_buffer<char> buf(4096 + 2048);
    FILE *fp = fmemopen(static_cast<void *>(buf.get_write()), buf.size(), "w+");
    if(fp == nullptr) {
      LOG_THROW("Failed to allocate filestream");
    }
    h.print(fp);
    fflush(fp);
    auto len = ftell(fp);
    fclose(fp);
    buf.trim(len);
    return make_ready_future<temporary_buffer<char>>(std::move(buf));
  }
  static future<> write_histogram(sstring filename, histogram h) {
    return open_file_dma(filename, open_flags::rw | open_flags::create
                                     | open_flags::truncate)
      .then([h = std::move(h)](file seastar_file) mutable {
        auto f = make_lw_shared<output_stream<char>>(
          make_file_output_stream(std::move(seastar_file)));
        return histogram_seastar_utils::print_histogram(std::move(h))
          .then([f](temporary_buffer<char> buf) {
            return f->write(buf.get(), buf.size())
              .then([f]() mutable {
                return f->flush().then(
                  [f]() mutable { return f->close().finally([f] {}); });
              });
          });
      });
  }

  static void sync_write_histogram(const sstring &filename, histogram *h) {
    try {
      FILE *fp = fopen(filename.c_str(), "w");
      if(fp) {
        h->print(fp);
        fclose(fp);
      }
    } catch(...) {
    }
  }
};


} // namespace smf
