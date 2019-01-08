// Copyright (c) 2016 Alexander Gallego. All rights reserved.
//
#pragma once
#include <seastar/core/future.hh>
namespace smf {

/// brief - generic filter interface (c++ conept'ish) that gets
/// applied from the server and client(todo) before and after a request
///
template <typename T>
struct rpc_filter {
  seastar::future<T> operator()(T t);
};

/// brief - applies a functor `future<T> operator()(T t)` to all the filters
/// useful for incoming and outgoing filters. Taking a pair of iterators
///
template <typename Iterator, typename Arg, typename... Ret>
seastar::future<Ret...>
rpc_filter_apply(const Iterator &b, const Iterator &end, Arg &&arg) {
  auto begin = b;
  if (begin == end) {
    return seastar::make_ready_future<Ret...>(std::forward<Arg>(arg));
  }
  return (*begin)(std::forward<Arg>(arg))
    .then([begin = std::next(begin), end](Arg &&a) {
      return rpc_filter_apply<Iterator, Arg, Ret...>(begin, end,
                                                     std::forward<Arg>(a));
    });
}

template <class Container, typename Arg>
seastar::future<Arg>
rpc_filter_apply(Container *c, Arg &&arg) {
  return rpc_filter_apply<typename Container::iterator, Arg, Arg>(
    c->begin(), c->end(), std::forward<Arg>(arg));
}

}  // namespace smf
