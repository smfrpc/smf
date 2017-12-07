// Copyright 2017 Alexander Gallego
//
#pragma once

#include <iostream>

#include <core/sstring.hh>

#include "filesystem/wal_requests.h"
#include "flatbuffers/wal_generated.h"

namespace std {
ostream &operator<<(ostream &o, const ::smf::wal::tx_put_fragment &);
ostream &operator<<(ostream &o, const ::smf::wal::tx_get_request &);
ostream &operator<<(ostream &o, const ::smf::wal::tx_get_reply &);
ostream &operator<<(ostream &o, const ::smf::wal::tx_put_request &);
ostream &operator<<(ostream &o, const ::smf::wal::tx_put_reply &);
ostream &operator<<(ostream &o, const ::smf::wal::wal_header &);
ostream &operator<<(ostream &o,
                    const ::smf::wal::tx_put_reply_partition_tupleT &);
ostream &operator<<(ostream &o,
                    const ::smf::wal::tx_put_reply_partition_tuple &);

ostream &operator<<(ostream &o, const ::smf::wal_read_request &);
ostream &operator<<(ostream &o, const ::smf::wal_read_reply &);
ostream &operator<<(ostream &o, const ::smf::wal_write_request &);
ostream &operator<<(ostream &o, const ::smf::wal_write_reply &);
}  // namespace std
