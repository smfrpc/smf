// Copyright 2017 Alexander Gallego
//
#pragma once
#include <iostream>
#include "filesystem/wal_requests.h"
#include "flatbuffers/wal_generated.h"

namespace std {
// TODO(agallego) - add smart printing for all types

ostream &operator<<(ostream &o, const ::smf::wal::wal_header &h);
ostream &operator<<(ostream &o, const ::smf::wal_read_request &r);
}  // namespace std
