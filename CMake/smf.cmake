#  1. over time this should become a proper cmake module

function(smfc_gen)
  set(options CPP GOLANG)
  set(oneValueArgs TARGET_NAME OUTPUT_DIRECTORY)
  set(multiValueArgs SOURCES)
  cmake_parse_arguments(SMFC_GEN "${options}" "${oneValueArgs}" "${multiValueArgs}" ${ARGN})

  set(SMF_GEN_OUTPUTS)
  foreach(FILE ${SMFC_GEN_SOURCES})
    get_filename_component(BASE_NAME ${FILE} NAME_WE)
    set(FLATC_OUTPUT
      "${SMFC_GEN_OUTPUT_DIRECTORY}/${BASE_NAME}_generated.h")
    set(SMF_GEN_OUTPUT
      "${SMFC_GEN_OUTPUT_DIRECTORY}/${BASE_NAME}.smf.fb.h")
    list(APPEND SMF_GEN_OUTPUTS ${FLATC_OUTPUT} ${SMF_GEN_OUTPUT})
    add_custom_command(OUTPUT ${FLATC_OUTPUT}
      COMMAND flatc
      ARGS --gen-name-strings --gen-object-api --cpp
           --json --reflect-names --defaults-json
           --gen-mutable --cpp-str-type 'seastar::sstring'
           -o "${SMFC_GEN_OUTPUT_DIRECTORY}/" ${FILE}
      COMMENT "Building C++ header for ${FILE}"
      DEPENDS flatc
      DEPENDS ${FILE}
      WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR})

    add_custom_command(OUTPUT ${SMF_GEN_OUTPUT}
      COMMAND smfc
      ARGS --logtostderr --filename ${FILE}
           --output_path=${SMFC_GEN_OUTPUT_DIRECTORY}
      COMMENT "Generating smf rpc stubs for ${FILE}"
      DEPENDS smfc
      DEPENDS ${FILE}
      WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR})
  endforeach()
  set(${SMFC_GEN_TARGET_NAME} ${SMF_GEN_OUTPUTS} PARENT_SCOPE)
endfunction()
