#pragma once
#include <memory>

namespace smf {


template <class T> class rpc_client_connection_pool {
  public:
  struct connection_pool_entry {
    uint32_t hits{0};
    std::unique_ptr<T> client_;

  };

  private:
  std::vector<std::unique_ptr<connection_pool_entry<T>>> clients_;
};
} // end namespace smf
