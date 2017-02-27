// Copyright (c) 2016 Alexander Gallego. All rights reserved.
//
#pragma once
#include "flatbuffers/chain_replication.smf.fb.h"
namespace smf {
namespace chains {
/** \brief map the request to the lcore that is going to handle the put */
uint32_t put_to_lcore(const tx_put_request *r);
}
}
