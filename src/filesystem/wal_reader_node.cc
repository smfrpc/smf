// Copyright(c) 2016 Alexander Gallego.All rights reserved.
//
#include "filesystem/wal_reader_node.h"

#include <cstdint>
#include <memory>
#include <system_error>
#include <utility>

#include <core/byteorder.hh>
#include <core/metrics.hh>
#include <core/reactor.hh>
#include <core/seastar.hh>

#include "filesystem/file_size_utils.h"
#include "filesystem/wal_page_cache.h"
#include "filesystem/wal_pretty_print_utils.h"
#include "flatbuffers/fbs_typed_buf.h"
#include "hashing/hashing_utils.h"
#include "platform/log.h"
#include "platform/macros.h"

namespace smf {
struct read_exactly_req {
  read_exactly_req(uint64_t                          _offset,
                   uint64_t                          _size,
                   const seastar::io_priority_class &p)
    : offset(_offset), size(_size), pc(p) {}

  const uint64_t                    offset;
  const uint64_t                    size;
  const seastar::io_priority_class &pc;

  inline uint64_t
  consume_offset() const {
    return offset + consumed;
  }
  inline uint64_t
  size_left_to_consume() const {
    return size - consumed;
  }
  uint64_t consumed{0};
};


// -- static helper funcs
//
static uint64_t
round_up(const uint64_t &file_size, const uint64_t &alignment) {
  uint64_t ret = file_size / alignment;
  if (file_size % alignment != 0) { ret += 1; }
  return ret;
}

inline smf::wal::wal_read_errno
validate_header(const wal::wal_header &hdr, const uint64_t &file_size) {
  if (hdr.checksum() == 0 || hdr.size() == 0) {
    return smf::wal::wal_read_errno::wal_read_errno_invalid_header;
  }
  DLOG_THROW_IF(hdr.size() > file_size,
                "Header asked for more data than file_size. Header:{}", hdr);
  return smf::wal::wal_read_errno::wal_read_errno_none;
}

inline smf::wal::wal_read_errno
validate_payload(const wal::wal_header &                hdr,
                 const seastar::temporary_buffer<char> &buf) {
  if (buf.size() != hdr.size()) {
    return smf::wal::wal_read_errno::
      wal_read_errno_missmatching_header_payload_size;
  }
  if (xxhash_32(buf.get(), buf.size()) != hdr.checksum()) {
    return smf::wal::wal_read_errno::
      wal_read_errno_missmatching_payload_checksum;
  }
  return smf::wal::wal_read_errno::wal_read_errno_none;
}


// There are 2 offsets. The consuming-from and the producing-to
// one comes form the files, one is being advanced by this function
//
static uint64_t
copy_page_data(seastar::lw_shared_ptr<seastar::temporary_buffer<char>> buf,
               const seastar::lw_shared_ptr<read_exactly_req>          req,
               const user_page    disk_page,
               const std::size_t &page_no,
               const std::size_t &alignment) {
  LOG_THROW_IF(disk_page.size == 0, "Bad page");

  DLOG_INFO(
    "offset: {}, consume_offset: {}, page begin: {}, page: {}, alignment: {}",
    req->offset, req->consume_offset(), page_no * alignment, page_no,
    alignment);

  const uint64_t page_offset = req->consume_offset() - (page_no * alignment);
  const char *   src         = disk_page.data() + page_offset;
  char *         dst         = buf->get_write() + req->consumed;

  // XXX agallego - working on this
  const uint64_t step_size = std::min<int64_t>(disk_page.size - page_offset,
                                               req->size_left_to_consume());

  DLOG_INFO("page_offset: {}, step_size: {}, space_left_to_consume(): {}",
            page_offset, step_size, req->size_left_to_consume());

  if (step_size == 0) {
    // nothing left to copy
    LOG_WARN("step_size to copy is 0. invalid memcpy");
    return step_size;
  }
  std::memcpy(dst, src, step_size);
  return step_size;
}


// -- wal_reader_node impl
//

wal_reader_node::wal_reader_node(uint64_t epoch, seastar::sstring _filename)
  // needed signed for comparisons
  : starting_epoch(static_cast<int64_t>(epoch)), filename(_filename) {
  namespace sm = seastar::metrics;
  metrics_.add_group("smf::wal_reader_node::" + filename,
                     {sm::make_derive("bytes_read", stats_.total_bytes,
                                      sm::description("bytes read from disk")),
                      sm::make_derive("total_reads", stats_.total_reads,
                                      sm::description("Number of read ops"))});
}
wal_reader_node::~wal_reader_node() {}

seastar::future<>
wal_reader_node::close() {
  auto f = file_;  // must hold file until handle closes
  return f->close().finally([f] {});
}

seastar::future<>
wal_reader_node::open_node() {
  return seastar::open_file_dma(filename, seastar::open_flags::ro)
    .then([this](seastar::file ff) {
      file_ = seastar::make_lw_shared<seastar::file>(std::move(ff));
      return seastar::make_ready_future<>();
    });
}
seastar::future<>
wal_reader_node::open() {
  return seastar::file_size(filename)
    .then([this](auto size) { file_size_ = size; })
    .then([this] { return open_node(); })
    .then([this] {
      // must happen after actual read
      number_of_pages_ = round_up(file_size_, file_->disk_read_dma_alignment());
      return file_->stat().then([this](auto stat) {
        // https://www.gnu.org/software/libc/manual/html_node/Attribute-Meanings.html
        // inode and device id create a globally unique file id
        //
        incremental_xxhash64 inc;
        inc.update(stat.st_ino);
        inc.update(stat.st_dev);
        global_file_id_ = inc.digest();
        DLOG_TRACE("Opened {} with global file_id: xxhash_64({}:{}) == {}",
                   filename, stat.st_ino, stat.st_dev, global_file_id_);
      });
    });
}

seastar::future<>
wal_reader_node::get(seastar::lw_shared_ptr<wal_read_reply> retval,
                     wal_read_request                       r) {
  auto offset = retval->next_epoch();
  if (offset >= static_cast<uint64_t>(starting_epoch) &&
      offset < static_cast<uint64_t>(ending_epoch())) {
    return do_read(retval, r);
  }
  DLOG_INFO("... Skipping read");
  return seastar::make_ready_future<>();
}

// change this to do_read()
// relative_offset should be a function.
//

seastar::future<>
wal_reader_node::do_read(seastar::lw_shared_ptr<wal_read_reply> retval,
                         wal_read_request                       r) {
  return seastar::repeat([r, this, retval]() mutable {
    // FIXME(agallego) - all the debug_* stuff shoudl return error
    // enums, according to wal.fbs
    auto consume = retval->get_consume_offset();

    //    DLOG_INFO_IF(retval->on_wire_size() >= r.req->max_bytes(),
    DLOG_INFO("do_read from offset: (consume offset): {}, "
              "retval->on_wire_size(): {}, r.req->max_bytes():{}, "
              "next_epoch():{} (RANGE: {} - {})",
              consume, retval->on_wire_size(), r.req->max_bytes(),
              retval->next_epoch(), starting_epoch, ending_epoch());


    if (retval->on_wire_size() >= r.req->max_bytes() ||
        retval->next_epoch() < starting_epoch ||
        retval->next_epoch() >= (ending_epoch() - kOnDiskInternalHeaderSize)) {
      return seastar::make_ready_future<seastar::stop_iteration>(
        seastar::stop_iteration::yes);
    }


    return this->read_exactly(consume, kOnDiskInternalHeaderSize, r.pc)
      .then([this, r, retval, consume](auto buf) mutable {
        if (buf.size() != kOnDiskInternalHeaderSize) {
          DLOG_INFO("Could not read header. Only read: {} bytes", buf.size());
          return seastar::make_ready_future<seastar::stop_iteration>(
            seastar::stop_iteration::yes);
        }
        auto hdr = wal::wal_header{};
        hdr.mutate_size(seastar::read_le<uint32_t>(buf.get()));
        hdr.mutate_checksum(seastar::read_le<uint32_t>(buf.get() + 4));
        hdr.mutate_compression(
          static_cast<smf::wal::wal_entry_compression_type>(
            seastar::read_le<int8_t>(buf.get() + 8)));

        consume += kOnDiskInternalHeaderSize;

        retval->reply()->error = validate_header(hdr, file_size_);
        if (retval->reply()->error !=
            smf::wal::wal_read_errno::wal_read_errno_none) {
          DLOG_ERROR("Error parsing header: {}",
                     smf::wal::EnumNamewal_read_errno(retval->reply()->error));
          return seastar::make_ready_future<seastar::stop_iteration>(
            seastar::stop_iteration::yes);
        }

        return this->read_exactly(consume, hdr.size(), r.pc).then([
          retval, hdr = std::move(hdr), this, r
        ](auto buf) mutable {
          if (buf.size() != hdr.size()) {
            DLOG_INFO("Could not read payload. Only read: {} bytes",
                      buf.size());
            return seastar::stop_iteration::yes;
          }
          retval->reply()->error = validate_payload(hdr, buf);
          if (retval->reply()->error !=
              smf::wal::wal_read_errno::wal_read_errno_none) {
            DLOG_ERROR(
              "Error parsing payload: {}",
              smf::wal::EnumNamewal_read_errno(retval->reply()->error));
            return seastar::stop_iteration::yes;
          }

          retval->update_consume_offset(retval->get_consume_offset() +
                                        buf.size() + kOnDiskInternalHeaderSize);
          auto ptr = std::make_unique<wal::tx_get_fragmentT>();
          ptr->hdr = std::make_unique<wal::wal_header>(std::move(hdr));

          ptr->fragment.resize(buf.size());
          std::copy_n(buf.get(), buf.size(), ptr->fragment.begin());
          DLOG_THROW_IF(ptr->fragment.size() != buf.size(),
                        "fragment: {}, buf:{}", ptr->fragment.size(),
                        buf.size());
          retval->reply()->gets.push_back(std::move(ptr));
          DLOG_INFO("NODE:: size of retval->reply()->gets: {}",
                    retval->reply()->gets.size());
          return seastar::stop_iteration::no;
        });
      });
  });
}

seastar::future<seastar::temporary_buffer<char>>
wal_reader_node::read_exactly(const uint64_t &                  offset,
                              const uint64_t &                  size,
                              const seastar::io_priority_class &pc) {
  auto buf = seastar::make_lw_shared<seastar::temporary_buffer<char>>(size);
  auto req = seastar::make_lw_shared<read_exactly_req>(offset, size, pc);
  auto max_pages = 1 + (req->size / file_->disk_read_dma_alignment());
  LOG_THROW_IF(max_pages > number_of_pages_,
               "Request asked to read more pages, than available on disk");
  return seastar::repeat([buf, this, req]() mutable {
           if (req->size_left_to_consume() == 0) {
             return seastar::make_ready_future<seastar::stop_iteration>(
               seastar::stop_iteration::yes);
           }

           auto page = offset_to_page(req->consume_offset(),
                                      file_->disk_read_dma_alignment());

           DLOG_INFO("TEMPBUF:: size: {}, next_page to read: {}, size left to "
                     "read:{}, consume_offset: {}, consumed:{}",
                     buf->size(), page, req->size_left_to_consume(),
                     req->consume_offset(), req->consumed);
           // Note:
           // Make sure that all our reads go through the lcore system cache
           //
           return wal_page_cache::get()
             .cache_read(file_, global_file_id_, page, req->pc)
             .then([buf, page, req, this](auto page_data) mutable {
               if (!page_data) {
                 DLOG_INFO("Page:{} not in filesystem", page);
                 return seastar::stop_iteration::yes;
               }
               auto &&datax = page_data.value();
               req->consumed += copy_page_data(
                 buf, req, datax, page, file_->disk_read_dma_alignment());
               return seastar::stop_iteration::no;
             });
         })
    .then([req, buf] {
      if (req->size_left_to_consume() > 0) {
        DLOG_INFO("Did not read enough data. Wanted to read: {}, but read:{}",
                  req->size, req->size - req->size_left_to_consume());
        // cannot return incomplete data
        return seastar::make_ready_future<seastar::temporary_buffer<char>>();
      }
      return seastar::make_ready_future<seastar::temporary_buffer<char>>(
        buf->share());

    });
}

}  // namespace smf
