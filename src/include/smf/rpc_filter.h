// Copyright (c) 2016 Alexander Gallego. All rights reserved.
//
#pragma once
#include <utility>

#include <seastar/core/future.hh>

#include "smf/stdx.h"

namespace smf {

/// brief - generic filter interface (c++ conept'ish) that gets
/// applied from the server and client(todo) before and after a request
///
template <typename T>
struct rpc_filter {
  using type = T;
  seastar::future<T> operator()(T &&r);
};
using rpc_ougoing_filter = rpc_filter<stdx::optional<rpc_envelope>>;
using rpc_incoming_filter = rpc_filter<stdx::optional<rpc_recv_context>>;

/// brief - applies a functor `future<T> operator()(T t)` to all the filters
/// useful for incoming and outgoing filters. Taking a pair of iterators
///
template <typename Iterator, typename R>
seastar::future<R>
rpc_filter_apply(const Iterator &b, const Iterator &end, R &&arg) {
  if (!arg) { return seastar::make_ready_future<R>(std::forward<R>(arg)); }
  auto begin = b;
  if (begin == end) {
    return seastar::make_ready_future<R>(std::forward<R>(arg));
  }
  return (*begin)(std::forward<R>(arg))
    .then([begin = std::next(begin), end](R &&a) {
      return rpc_filter_apply(begin, end, std::forward<R>(a));
    });
}

template <class Container, typename T>
seastar::future<T>
rpc_filter_apply(Container *c, T &&arg) {
  return rpc_filter_apply<typename Container::iterator, T>(
    std::begin(*c), std::end(*c), std::forward<T>(arg));
}

}  // namespace smf
