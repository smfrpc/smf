#pragma once
#include <iostream>
#include "filesystem/wal_requests.h"
#include "flatbuffers/wal_generated.h"

namespace std {
ostream &operator<<(ostream &o, const ::smf::fbs::wal::wal_header &h);
ostream &operator<<(ostream &o, const ::smf::wal_read_request &r);
}  // namespace std
