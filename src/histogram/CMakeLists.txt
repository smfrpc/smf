add_library(smftracing STATIC
  ${PROJECT_SOURCE_DIR}/src/histogram/histogram.cc
  ${PROJECT_SOURCE_DIR}/src/histogram/histogram_seastar_utils.cc
  )
target_link_libraries(smftracing
  hdr_histogram_static
  ${SEASTAR_LDFLAGS}
  )
install(TARGETS smftracing DESTINATION lib)
