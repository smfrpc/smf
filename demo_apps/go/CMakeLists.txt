include(${PROJECT_SOURCE_DIR}/CMake/golang.cmake)
smfc_gen(
  GO
  TARGET_NAME go_demo_fbs
  OUTPUT_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
  SOURCES ${CMAKE_CURRENT_SOURCE_DIR}/../demo_service.fbs
  )

set(golang_demo_dir "${GOPATH}/src/github.com/senior7515/smf/demo_apps/go")
add_custom_target(goserver)
add_custom_command(
  TARGET goserver
  COMMAND export GOPATH=${GOPATH} && cd ${golang_demo_dir}/server && ${GO} build ...
  DEPENDS gosmf ${go_demo_fbs})

add_custom_target(goserver_all ALL DEPENDS goserver)
