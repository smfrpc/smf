#include <iostream>

#include <core/sstring.hh>
#include "flatbuffers/native_type_utils.h"
#include "flatbuffers/wal_generated.h"


int
main(void) {
  char data[50];
  std::memset(data, 'x', 50);
  smf::wal::tx_put_fragmentT f;
  f.op       = smf::wal::tx_put_operation::tx_put_operation_full;
  f.epoch_ms = 100;
  f.type     = smf::wal::tx_put_fragment_type::tx_put_fragment_type_kv;
  f.key.resize(50);
  f.value.resize(50);
  std::memcpy(&f.key[0], data, 50);
  std::memcpy(&f.value[0], data, 50);
  auto buf = smf::native_table_as_buffer<smf::wal::tx_put_fragment>(f);
  std::cout << "Size of struct: " << sizeof(f) + 100 << std::endl; // 180
  std::cout << "Size of buffer: " << buf.size() << std::endl;      // 160
  int64_t overhead = buf.size() - sizeof(f) - 100;                 // -20
  std::cout << "Buffer overrhead is: " << overhead << std::endl;
  return 0;
}
