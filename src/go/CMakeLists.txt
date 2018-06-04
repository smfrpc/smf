include(${PROJECT_SOURCE_DIR}/CMake/golang.cmake)
smfc_gen(
  GOLANG
  TARGET_NAME go_api
  OUTPUT_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
  SOURCES ${PROJECT_SOURCE_DIR}/src/core/rpc.fbs)

add_subdirectory(smf)
