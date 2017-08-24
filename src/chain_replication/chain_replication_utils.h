// Copyright (c) 2016 Alexander Gallego. All rights reserved.
//
#pragma once
#include "flatbuffers/chain_replication.smf.fb.h"
namespace smf {
namespace chains {
/// \brief map the request to the lcore that is going to handle the put
uint32_t put_to_lcore(const char *topic, smf::wal::tx_put_partition_pair *p);

/// \brief map the request to the lcore that is going to handle the reads
uint32_t get_to_lcore(const smf::wal::tx_get_request *r);
}
}
