#include <iostream>

#include "folly/SocketAddress.h"

int main(int argc, char **argv) {
  folly::SocketAddress a;
  a.setFromHostPort("localhost", 2344);
  std::cout << "Address is: " << a.describe() << std::endl;
  return 0;
}
