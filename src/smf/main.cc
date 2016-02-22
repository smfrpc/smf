#include <iostream>
#include "folly/SocketAddress.h"

int main(int argc, char **argv) {
  folly::SocketAddress a;
  a.setFromHostPort("localhost", 2344);
  if(true)
    std::cout << "Test folly: Address is: " << a.describe() << std::endl;
}
