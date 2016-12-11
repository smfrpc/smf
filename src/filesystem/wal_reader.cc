// source header
#include "filesystem/wal_reader.h"
// third party
#include <core/reactor.hh>
// smf
#include "log.h"
#include "filesystem/wal_reader_node.h"
#include "filesystem/wal_name_extractor_utils.h"
#include "filesystem/wal_head_file_functor.h"


namespace smf {

future<> wal_reader::close() {
  assert(reader_ != nullptr);
  return reader_->close();
}

future<> wal_reader::open() {
  return open_directory(directory).then([this](file f) {
    auto l =
      make_lw_shared<wal_head_file_functor<[this](auto new_file, auto current) {
        auto e = extract_epoch(new_file);
        if(readers_.find(e) == readers_.end()) {
          readers_.insert({e, std::make_unique<wal_reader_node>(new_file)});
        }
        return true;
      }>>(std::move(f));
    return l->close().then([l = l] { return make_ready_future<>(); });
  });
}
future<std::experimental::optional<temporary_buffer<char>>>
wal_reader::get(uint64_t epoch) {
  if(readers_.find(epoch) == readers_.end()) {
    return std::experimental::nullopt;
  }
  return readers_[epoch]->get(epoch);
}


} // namespace smf
