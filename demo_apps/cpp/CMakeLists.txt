smfc_gen(
  CPP
  TARGET_NAME demo_fbs
  OUTPUT_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
  SOURCES ${CMAKE_CURRENT_SOURCE_DIR}/../demo_service.fbs)

add_executable(demo_server server/main.cc "${demo_fbs}")
target_link_libraries(demo_server smf)
target_include_directories(demo_server
  PUBLIC ${CMAKE_CURRENT_BINARY_DIR}
  PUBLIC ${PROJECT_SOURCE_DIR}/src/include
  PUBLIC ${PROJECT_SOURCE_DIR}/src/third_party/flatbuffers/include
)

add_executable(demo_client client/main.cc "${demo_fbs}")
target_link_libraries(demo_client smf)
target_include_directories(demo_client
  PUBLIC ${CMAKE_CURRENT_BINARY_DIR}
  PUBLIC ${PROJECT_SOURCE_DIR}/src/include
  PUBLIC ${PROJECT_SOURCE_DIR}/src/third_party/flatbuffers/include
)



install(TARGETS
  demo_server
  demo_client
  DESTINATION bin)
