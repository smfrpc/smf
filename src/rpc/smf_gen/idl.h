// Copyright (c) 2016 Alexander Gallego. All rights reserved.
//
#pragma once
#include <flatbuffers/flatbuffers.h>
#include <flatbuffers/idl.h>
namespace smf_gen {
bool generate(const flatbuffers::Parser &parser, std::string file_name);
}  // namespace smf_gen
