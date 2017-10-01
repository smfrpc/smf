add_library(smf_tracing STATIC
  ${PROJECT_SOURCE_DIR}/src/histogram/histogram.cc
  ${PROJECT_SOURCE_DIR}/src/histogram/histogram_seastar_utils.cc
  )
target_link_libraries(smf_tracing
  hdr_histogram_static
  )
smf_seastar_target(smf_tracing)
install(TARGETS smf_tracing DESTINATION lib)
