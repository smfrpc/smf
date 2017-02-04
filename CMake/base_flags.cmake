# Seastar doesn't finish linking w/ gold linker
# "-fuse-ld=gold"
set(BASE_FLAGS
  "-fdiagnostics-color=auto"
  "-fPIC"
  "-fconcepts"
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
  )

if(CMAKE_BUILD_TYPE MATCHES Debug)
  set(BASE_FLAGS
    ${BASE_FLAGS}
    "-lasan"
    "-lubsan"
    "-O0"
    "-ggdb"
    )
else()
  set(BASE_FLAGS
    ${BASE_FLAGS}
    "-O2"
    )
endif()
