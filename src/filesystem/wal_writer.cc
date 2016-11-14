// source header
#include "filesystem/wal_writer.h"
// third party
#include <core/reactor.hh>
// smf
#include "log.h"
#include "filesystem/wal_writer_utils.h"
#include "filesystem/wal_writer_node.h"
#include "filesystem/wal_head_file_functor.h"


namespace smf {


future<> wal_writer::open() {
  return open_directory(directory)
    .then([this](file f) {
      auto l = make_lw_shared<wal_head_file_functor>(std::move(f));
      return l->close().then([l, this]() mutable {
        return open_file_dma(l->last_file, open_flags::ro)
          .then([this, prefix = l->name_parser.prefix](file last) {
            auto lastf = make_lw_shared<file>(std::move(last));
            return lastf->size().then([this, prefix, lastf](uint64_t size) {
              current_ = std::make_unique<wal_writer_node>(prefix, size);
              return lastf->close().then([this] { return current_->open(); });
            });
          });
      });
    });
}

future<> wal_writer::append(temporary_buffer<char> &&buf) {
  return current_->append(std::move(buf));
}


} // namespace smf
