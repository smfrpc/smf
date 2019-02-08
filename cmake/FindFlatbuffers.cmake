find_package (PkgConfig REQUIRED)
pkg_search_module (Flatbuffers_PC QUIET flatbuffers)
find_library (Flatbuffers_LIBRARY
  NAMES flatbuffers
  HINTS
    ${Flatbuffers_PC_LIBDIR}
    ${Flatbuffers_PC_LIBRARY_DIRS})
find_path (Flatbuffers_INCLUDE_DIR
  NAMES "flatbuffers/flatbuffers.h"
  HINTS
    ${Flatbuffers_PC_INCLUDEDIR}
    ${Flatbuffers_PC_INCLUDEDIRS})
mark_as_advanced (
  Flatbuffers_LIBRARY
  Flatbuffers_INCLUDE_DIR)

include (FindPackageHandleStandardArgs)
find_package_handle_standard_args (Flatbuffers
  REQUIRED_VARS
    Flatbuffers_LIBRARY
    Flatbuffers_INCLUDE_DIR
  VERSION_VAR Flatbuffers_PC_VERSION)

set (Flatbuffers_LIBRARIES ${Flatbuffers_LIBRARY})
set (Flatbuffers_INCLUDE_DIRS ${Flatbuffers_INCLUDE_DIR})
if (Flatbuffers_FOUND AND NOT (TARGET Flatbuffers::flatbuffers))
  add_library (Flatbuffers::flatbuffers UNKNOWN IMPORTED)
  set_target_properties (Flatbuffers::flatbuffers
    PROPERTIES
      IMPORTED_LOCATION ${Flatbuffers_LIBRARY}
      INTERFACE_INCLUDE_DIRECTORIES ${Flatbuffers_INCLUDE_DIRS})
endif ()
