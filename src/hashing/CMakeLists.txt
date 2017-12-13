add_library(smf_hashing INTERFACE)
target_compile_definitions(smf_hashing
  INTERFACE -DXXH_PRIVATE_API)
target_include_directories(smf_hashing
  INTERFACE ${CMAKE_SOURCE_DIR}/src/third_party/xxHash)
