// Copyright (c) 2016 Alexander Gallego. All rights reserved.
//
#pragma once
#include <list>

#include <core/sstring.hh>

#include "filesystem/wal_requests.h"

namespace smf {
/// \brief map the request to the lcore that is going to handle the reads
uint32_t get_to_lcore(const smf::wal::tx_get_request *r);

/// \brief get a list of core assignments. Note that because the mapping is
/// always consistent and based off the actual underlying hardware, a core might
/// get multiple assignments.
///
std::list<smf::wal_write_request> core_assignment(
  const smf::wal::tx_put_request *p);

}  // namespace smf
