link_directories(
  ${SEASTAR_LIBRARY_DIRS}
  ${PROJECT_SOURCE_DIR}/src/third_party/lib
  ${PROJECT_SOURCE_DIR}/src/third_party/lib64
  # system includes should be last
  /usr/local/lib
  /usr/local/lib64
  /usr/lib
  /usr/lib64
  /lib
  /lib64
  )
include_directories(
  SYSTEM
  ${SEASTAR_INCLUDE_DIRS}
  ${PROJECT_SOURCE_DIR}/src/third_party/include
  )
include_directories(
  ${PROJECT_SOURCE_DIR}/src
  )

# accept include dirs passed in command line
foreach(dir ${CMAKE_INCLUDE_PATH})
  include_directories(${dir})
endforeach()
