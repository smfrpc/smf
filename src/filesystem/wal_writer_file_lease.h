// Copyright (c) 2016 Alexander Gallego. All rights reserved.
//
#pragma once
#include <core/future.hh>

namespace smf {
/// \brief exposes what the \ref wal_writer_node.h
/// needs which is effecitvely a seastar::output_stream<char>
/// however, we want to prevent the readers and writers to be accessing the
/// same files
///
class wal_writer_file_lease {
 public:
  virtual seastar::future<> close() = 0;
  virtual seastar::future<> flush() = 0;
  virtual seastar::future<> open()  = 0;
  virtual seastar::future<> append(const char *buf, size_t n) = 0;
  virtual ~wal_writer_file_lease() {}
};
}  // namespace smf
