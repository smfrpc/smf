// Copyright (c) 2016 Alexander Gallego. All rights reserved.
//
#pragma once
// third party
#include <core/sstring.hh>

namespace smf {

/// \brief extract a uint64_t epoch out of a write ahead log name
/// "smf0_<epoch>...wal"
uint64_t extract_epoch(const sstring &filename);

}  // namespace smf
