link_directories(
  ${SEASTAR_LIBRARY_DIRS}
  ${PROJECT_SOURCE_DIR}/src/third_party/lib
  ${PROJECT_SOURCE_DIR}/src/third_party/lib64
  /usr/local/lib # must be last if locals aren't used
  )
include_directories(
  SYSTEM
  ${SEASTAR_INCLUDE_DIRS}
  ${PROJECT_SOURCE_DIR}/src/third_party/include
  )
include_directories(
  ${PROJECT_SOURCE_DIR}/src
  )
