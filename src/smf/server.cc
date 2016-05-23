#include <iostream>
#include "rpc/rpc_generated.h"
int main(int args, char** argv, char** env){

  // read file char* data
  auto rpc = GetRPC(data);
  std::cout << "hello world" << std::endl;
}
