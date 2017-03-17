find_program(GIT git)
if(NOT DEFINED GIT_VERSION AND GIT)
  execute_process(
    COMMAND ${GIT} log -1 --pretty=format:%h
    WORKING_DIRECTORY "${PROJECT_SOURCE_DIR}"
    OUTPUT_VARIABLE GIT_VERSION
    OUTPUT_STRIP_TRAILING_WHITESPACE
    )
endif()

if(NOT DEFINED GIT_VERSION)
  # We don't have git
  set(GIT_VERSION "${SMF_VERSION}")
endif()
