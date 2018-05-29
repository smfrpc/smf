
set(GOPATH "${CMAKE_CURRENT_BINARY_DIR}/go")
file(MAKE_DIRECTORY ${GOPATH})

find_program(GO go)

function(go_build)
  set(options VERBOSE)
  set(oneValueArgs TARGET_NAME FOLDER)
  set(multiValueArgs INCLUDE_DIRS GO_GET_TARGETS)
  cmake_parse_arguments(GO_BUILD "${options}" "${oneValueArgs}" "${multiValueArgs}" ${ARGN})

  if(NOT SMF_ENABLE_GO)
    return()
  endif()
  if(NOT DEFINED GO)
    message("Could not find go, skipping go_build")
    return()
  endif()

  set(GO_GET_DEPS)

  foreach(GET_TARGET ${GO_BUILD_GO_GET_TARGETS})
    string(SHA1 DEP_HASH "${GET_TARGET}")
    set(DEP_NAME "${GO_BUILD_TARGET_NAME}_${DEP_HASH}")
    add_custom_target(${DEP_NAME} env GOPATH=${GOPATH} ${GO} get ${GET_TARGET})
    list(APPEND GO_GET_DEPS ${DEP_NAME})
  endforeach()

  add_custom_target(${GO_BUILD_TARGET_NAME})
  add_custom_command(TARGET ${GO_BUILD_TARGET_NAME}
    COMMAND env GOPATH=${GOPATH} ${GO} build
    -o "${CMAKE_CURRENT_BINARY_DIR}/${GO_BUILD_TARGET_NAME}"
    ${GO_BUILD_FOLDER}
    WORKING_DIRECTORY ${CMAKE_CURRENT_LIST_DIR}
    DEPENDS ${GO_GET_DEPS})

  install(PROGRAMS ${CMAKE_CURRENT_BINARY_DIR}/${GO_BUILD_TARGET_NAME}
    DESTINATION bin)
endfunction()
