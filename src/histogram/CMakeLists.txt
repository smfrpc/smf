add_library(smf_tracing STATIC
  histogram.cc
  histogram_seastar_utils.cc)
target_link_libraries(smf_tracing hdr_histogram_static Seastar::seastar)
install(TARGETS smf_tracing DESTINATION lib)
install(FILES
  histogram.h
  unique_histogram_adder.h
  histogram_seastar_utils.h
  DESTINATION include/smf/histogram
  )
