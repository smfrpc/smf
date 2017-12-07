// Copyright (c) 2016 Alexander Gallego. All rights reserved.
//
#include "filesystem/wal_reader.h"

#include <algorithm>
#include <memory>
#include <utility>

#include <core/reactor.hh>

#include "filesystem/wal_head_file_functor.h"
#include "filesystem/wal_name_extractor_utils.h"
#include "filesystem/wal_reader_node.h"
#include "hashing/hashing_utils.h"
#include "platform/log.h"
#include "utils/human_bytes.h"

namespace smf {

struct wal_reader_node_comparator {
  bool
  operator()(auto &xptr, auto &yptr) {
    return xptr->starting_epoch < yptr->starting_epoch;
  }
};
struct wal_reader_node_offset_comparator {
  bool
  operator()(const uint64_t &                             offset,
             const std::unique_ptr<smf::wal_reader_node> &yptr) {
    DLOG_INFO("with request offset: {} Comparing node offset: {} ",
              yptr->starting_epoch, offset);

    return offset < yptr->starting_epoch;
  }
  bool
  operator()(const std::unique_ptr<smf::wal_reader_node> &yptr,
             const uint64_t &                             offset) {
    DLOG_INFO("Comparing node offset: {} with request offset: {}",
              yptr->starting_epoch, offset);
    return yptr->starting_epoch < offset;
  }
};


wal_reader_visitor::wal_reader_visitor(wal_reader *r, seastar::file dir)
  : wal_file_walker(std::move(dir)), reader(r) {}
seastar::future<>
wal_reader_visitor::visit(seastar::directory_entry de) {
  return reader->monitor_files(std::move(de));
}

}  // namespace smf

namespace smf {
wal_reader::wal_reader(seastar::sstring workdir) : work_directory(workdir) {}

wal_reader::wal_reader(wal_reader &&o) noexcept
  : work_directory(std::move(o.work_directory))
  , nodes_(std::move(o.nodes_))
  , fs_observer_(std::move(o.fs_observer_)) {}


wal_reader::~wal_reader() {}

seastar::future<>
wal_reader::monitor_files(seastar::directory_entry entry) {
  auto e = wal_name_extractor_utils::extract_epoch(entry.name);
  if (!std::binary_search(nodes_.begin(), nodes_.end(), e,
                          wal_reader_node_offset_comparator{})) {
    auto n =
      std::make_unique<wal_reader_node>(e, work_directory + "/" + entry.name);
    auto copy = n.get();
    return copy->open().then([ this, n = std::move(n) ]() mutable {
      if (n->file_size() <= 0) {
        LOG_TRACE("Skipping empty (0-bytes) file: {}", n->filename);
      } else {
        LOG_INFO("WAL File: {} (offsets: {} - {}) {}", n->filename,
                 n->starting_epoch, n->ending_epoch(),
                 human_bytes(n->ending_epoch() - n->starting_epoch));
        nodes_.push_back(std::move(n));
        std::sort(nodes_.begin(), nodes_.end(), wal_reader_node_comparator{});
      }
      return seastar::make_ready_future<>();
    });
  }
  return seastar::make_ready_future<>();
}

seastar::future<>
wal_reader::close() {
  return seastar::do_for_each(nodes_.begin(), nodes_.end(),
                              [this](auto &b) { return b->close(); });
}

seastar::future<>
wal_reader::open() {
  return open_directory(work_directory).then([this](seastar::file f) {
    fs_observer_ = std::make_unique<wal_reader_visitor>(this, std::move(f));
    // the visiting actually happens on a subscription thread from the filesys
    // api and it calls seastar::future<>monitor_files()...
    return fs_observer_->done();
  });
}

seastar::future<seastar::lw_shared_ptr<wal_read_reply>>
wal_reader::get(wal_read_request r) {
  auto retval = seastar::make_lw_shared<wal_read_reply>(r.req->offset());
  if (r.req->max_bytes() == 0) {
    DLOG_WARN("Received request with zero max_bytes() to read");
    return seastar::make_ready_future<decltype(retval)>(retval);
  }
  DLOG_INFO("READER getting the page. offset: {}, partition:{}, topic:{}",
            r.req->offset(), r.req->partition(), r.req->topic());
  // just a wrapper around a lw_shared_ptr<>
  // ok to call copy ctor

  return seastar::repeat([this, r, retval]() mutable {
           DLOG_INFO("READER: XXX: next read epoch: {}",
                     retval->get_consume_offset());
           // for (auto &n : nodes_) {
           //   DLOG_INFO("Node: RANGE: {} - {}", n->starting_epoch,
           //             n->ending_epoch());
           // }
           auto it = std::lower_bound(nodes_.begin(), nodes_.end(),
                                      retval->next_epoch(),
                                      wal_reader_node_offset_comparator{});
           // safety
           if (it == nodes_.end()) {
             if (retval->empty() && !nodes_.empty()) {
               retval->reply()->next_offset =
                 nodes_.rbegin()->get()->starting_epoch;
             }
             return seastar::make_ready_future<seastar::stop_iteration>(
               seastar::stop_iteration::yes);
           }

           wal_reader_node *ptr = it->get();
           DLOG_INFO("READER found bucket offset: {} to {} - NEXTEPOCH: {}",
                     ptr->starting_epoch, ptr->ending_epoch(),
                     retval->next_epoch());

           // DLOG_INFO_IF(retval->on_wire_size() >= r.req->max_bytes(),
           //              "retval->on_wire:{}, r.req->max_bytes()",
           //              retval->on_wire_size(), r.req->max_bytes());
           // DLOG_INFO_IF(r.req->offset() < ptr->starting_epoch,
           //              "r.req->offset():{}, ptr->starting_epoch:{} ",
           //              r.req->offset(), ptr->starting_epoch);
           // DLOG_INFO_IF(r.req->offset() >= ptr->ending_epoch(),
           //              "r.req->offset():{}, ptr->ending_epoch():{}",
           //              r.req->offset(), ptr->ending_epoch());
           // accounting
           // if (retval->on_wire_size() >= r.req->max_bytes() ||
           //     r.req->offset() < ptr->starting_epoch ||
           //     r.req->offset() >= ptr->ending_epoch()) {
           //   // out of bounds
           //   if (r.req->offset() < ptr->starting_epoch) {
           //     retval->reply()->next_offset = ptr->starting_epoch;
           //   } else {
           //     retval->reply()->next_offset =
           //       std::max(retval->reply()->next_offset, ptr->starting_epoch);
           //   }
           //   return seastar::make_ready_future<seastar::stop_iteration>(
           //     seastar::stop_iteration::yes);
           // }
           // first scan
           // if (retval->get_consume_offset() == 0) {
           //   retval->update_consume_offset(
           //     ptr->starting_epoch > r.req->offset() ?
           //       0 :
           //       r.req->offset() - ptr->starting_epoch);
           // }
           return ptr->get(retval, r)
             .then([this, retval, r]() mutable {
               DLOG_INFO("READER found retval of on_disk_size: {}",
                         retval->on_disk_size());
               if (retval->on_wire_size() >= r.req->max_bytes()) {
                 return seastar::stop_iteration::yes;
               }
               if (retval->reply()->error !=
                   smf::wal::wal_read_errno::wal_read_errno_none) {
                 DLOG_ERROR(
                   "Error from wal_node_reader: {}. Stopping iteration",
                   smf::wal::EnumNamewal_read_errno(retval->reply()->error));
                 return seastar::stop_iteration::yes;
               }
               retval->reset_consume_offset();
               return seastar::stop_iteration::no;
             })
             .handle_exception([r](auto eptr) {
               LOG_ERROR("READER Caught exception: {}. Offset:{}, req size:{}",
                         eptr, r.req->offset(), r.req->max_bytes());
               return seastar::stop_iteration::yes;
             });
         })
    .then([retval] {
      DLOG_INFO("READER:: size of retval->reply()->gets.size(): {}",
                retval->reply()->gets.size());

      return seastar::make_ready_future<decltype(retval)>(retval);
    });
}


}  // namespace smf
