// Copyright (c) 2016 Alexander Gallego. All rights reserved.
//
#pragma once
#include <core/sstring.hh>
#include "flatbuffers/wal_generated.h"
namespace smf {
/// \brief map the request to the lcore that is going to handle the put
uint32_t put_to_lcore(const char *                           topic,
                      const smf::wal::tx_put_partition_pair *p);

/// \brief map the request to the lcore that is going to handle the reads
uint32_t get_to_lcore(const smf::wal::tx_get_request *r);
}
