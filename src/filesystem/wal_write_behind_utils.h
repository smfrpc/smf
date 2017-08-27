#pragma once

#include <core/fstream.hh>

namespace smf {

/// \brief shared as the default creator for multiple cache options.
///
inline seastar::file_output_stream_options default_file_ostream_options() {
  return seastar::file_output_stream_options{
    .buffer_size        = 8192,
    .preallocation_size = 1024 * 1024,  // 1MB for write behind
    .write_behind       = 1  //  Number of buffers to write in parallel
  };
}

}  // namespace smf
