// Copyright 2017 Alexander Gallego
//
#pragma once
#include <iostream>
#include "filesystem/wal_requests.h"
#include "flatbuffers/wal_generated.h"

namespace std {
ostream &operator<<(ostream &o, const ::smf::wal::tx_get_request &r);
ostream &operator<<(ostream &o, const ::smf::wal::tx_get_reply &r);
ostream &operator<<(ostream &o, const ::smf::wal::tx_put_request &r);
ostream &operator<<(ostream &o, const ::smf::wal::tx_put_reply &r);

ostream &operator<<(ostream &o, const ::smf::wal::wal_header &h);
ostream &operator<<(ostream &o, const ::smf::wal_read_request &r);
ostream &operator<<(ostream &o, const ::smf::wal_read_reply &r);
ostream &operator<<(ostream &o, const ::smf::wal_write_request &r);
ostream &operator<<(ostream &o, const ::smf::wal_write_reply &r);
}  // namespace std
