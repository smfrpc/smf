set(GOPATH "${CMAKE_BINARY_DIR}")
set(SMF_GO_SOURCE "${GOPATH}/src/github.com/senior7515")
file(MAKE_DIRECTORY
  "${GOPATH}/src"
  "${GOPATH}/pkg/linux_amd64/gosmf"
  "${SMF_GO_SOURCE}"
  )
set(symlink_name "godep_symlink_${GOPATH}")
string(REGEX REPLACE "/" "_" symlink_name ${symlink_name})
add_custom_target("${symlink_name}" ALL
  COMMAND ${CMAKE_COMMAND}
  -E create_symlink "${PROJECT_SOURCE_DIR}" "${SMF_GO_SOURCE}/smf")


find_program(GO go)

if(DEFINED GO)
  set(godep "godep_${GOPATH}")
  string(REGEX REPLACE "/" "_" godep ${godep})
  add_custom_target("${godep}" ALL
    env GOPATH=${GOPATH} ${GO} get "github.com/golang/dep/cmd/dep")
endif()


function(go_build_lib)
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
    add_custom_target(${DEP_NAME}
      env GOPATH=${GOPATH} "${GOPATH}/bin/dep ensure ${GET_TARGET}"
      WORKING_DIRECTORY ${GOPATH}
      )
    list(APPEND GO_GET_DEPS ${DEP_NAME})
  endforeach()

  add_custom_target(${GO_BUILD_TARGET_NAME})

  set(clean_subdir ${GO_BUILD_FOLDER})
  file(RELATIVE_PATH
    clean_subdir # Output variable
    "${PROJECT_SOURCE_DIR}" # Base directory
    ${clean_subdir} # Absolute path to the file
    )
  set(actual_source_directory
    "${GOPATH}/src/github.com/senior7515/smf/${clean_subdir}")
  add_custom_command(TARGET ${GO_BUILD_TARGET_NAME}
    COMMAND env GOPATH=${GOPATH} ${GO} build
    -o "${GOPATH}/pkg/linux_amd64/${GO_BUILD_TARGET_NAME}/${GO_BUILD_TARGET_NAME}.a"
    "./..."
    WORKING_DIRECTORY "${actual_source_directory}"
    DEPENDS ${GO_GET_DEPS})

  add_custom_target(${GO_BUILD_TARGET_NAME}_all ALL DEPENDS ${GO_BUILD_TARGET_NAME})
endfunction()
