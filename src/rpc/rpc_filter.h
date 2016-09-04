#pragma once
#include <core/future.hh>
namespace smf {

/// brief - generic filter interface (c++ conept'ish) that gets
/// applied from the server and client(todo) before and after a request
///
template <typename T> struct rpc_filter { future<T> operator()(T t); };


/// brief - applies a function future<T> apply(T &&t); to all the filters
/// useful for incoming and outgoing filters. Taking a pair of iterators to
/// unique ptrs. see incoming_filter.h and outgoing_filter.h
/// for details
///
template <typename Iterator, typename Arg, typename... Ret>
future<Ret...>
rpc_filter_apply(const Iterator &b, const Iterator &end, Arg &&arg) {
  auto begin = b;
  if(begin == end) {
    return make_ready_future<Ret...>(std::forward<Arg>(arg));
  }
  return (*(begin++))(std::forward<Arg>(arg))
    .then([begin, end](Arg &&a) {
      return rpc_filter_apply<Iterator, Arg, Ret...>(begin, end,
                                                     std::forward<Arg>(a));
    });
}

} // namespace smf
