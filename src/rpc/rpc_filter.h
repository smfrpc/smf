#pragma once
#include <core/future.hh>
namespace smf {
template <typename T> struct rpc_filter { future<T> operator()(T t); };
}
