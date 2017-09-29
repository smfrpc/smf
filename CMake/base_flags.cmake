set(BASE_FLAGS
  "-fdiagnostics-color=auto"
  "-fPIC"
  "-Wall"
  "-Werror"
  "-Wextra"
  "-Wformat"
  "-Wmissing-braces"
  "-Wparentheses"
  "-Wpointer-arith"
  "-Wformat-security"
  "-Wunused"
  "-Wno-unused-parameter"
  "-Wcast-align"
  "-Wno-missing-field-initializers"
  "-Wno-ignored-qualifiers"
  "-Wdelete-non-virtual-dtor"
  )

# require at least gcc 6.1 for -lasan
# ubuntu errors

if("${CMAKE_CXX_COMPILER_ID}" STREQUAL "GNU")
  if (CMAKE_CXX_COMPILER_VERSION VERSION_GREATER 6.1)
    set(BASE_FLAGS
      ${BASE_FLAGS}
      "-fconcepts"
      )
    add_definitions(-DSMF_GCC_CONCEPTS=1)
    if(CMAKE_BUILD_TYPE MATCHES Debug)
      set(BUILD_FLAGS
        ${BUILD_FLAGS}
        "-lasan"
        "-lubsan"
        )
    endif()
  endif()
endif()

if(CMAKE_BUILD_TYPE MATCHES Debug)
  set(BASE_FLAGS
    ${BASE_FLAGS}
    "-O0"
    "-ggdb"
    )
else()
  set(BASE_FLAGS
    ${BASE_FLAGS}
    "-O2"
    )
endif()
