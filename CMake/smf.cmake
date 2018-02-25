# TODO: over time this should become a proper cmake module

function(smf_generate_cpp name)
  set(SMF_GEN_OUTPUTS)
  foreach(FILE ${ARGN})
    get_filename_component(BASE_NAME ${FILE} NAME_WE)
    set(FLATC_OUTPUT "${CMAKE_CURRENT_BINARY_DIR}/${BASE_NAME}_generated.h")
    set(SMF_GEN_OUTPUT "${CMAKE_CURRENT_BINARY_DIR}/${BASE_NAME}.smf.fb.h")
    list(APPEND SMF_GEN_OUTPUTS ${FLATC_OUTPUT} ${SMF_GEN_OUTPUT})

    add_custom_command(OUTPUT ${FLATC_OUTPUT}
      COMMAND flatc
      ARGS --gen-name-strings --gen-object-api --cpp
           --json --reflect-names --defaults-json
           --gen-mutable --cpp-str-type 'seastar::sstring'
           -o "${CMAKE_CURRENT_BINARY_DIR}/" ${FILE}
      COMMENT "Building C++ header for ${FILE}"
      DEPENDS flatc
      DEPENDS ${FILE}
      WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR})

    add_custom_command(OUTPUT ${SMF_GEN_OUTPUT}
      COMMAND smf_gen
      ARGS --logtostderr --filename ${CMAKE_CURRENT_SOURCE_DIR}/${FILE}
           --output_path=${CMAKE_CURRENT_BINARY_DIR}
      COMMENT "Generating smf rpc stubs for ${FILE}"
      DEPENDS smf_gen
      DEPENDS ${FILE}
      WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR})
  endforeach()
  set(${name}_OUTPUTS ${SMF_GEN_OUTPUTS} PARENT_SCOPE)
endfunction()
