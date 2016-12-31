#pragma once
#include "rpc/smf_gen/smf_file.h"
#include <string>
namespace smf_gen {
// Return the prologue of the generated header file.
std::string get_header_prologue(smf_file *file);
// Return the includes needed for generated header file.
std::string get_header_includes(smf_file *file);
// Return the includes needed for generated source file.
std::string get_header_services(smf_file *file);
// Return the epilogue of the generated header file.
std::string get_header_epilogue(smf_file *file);
} // namespace smf_gen
