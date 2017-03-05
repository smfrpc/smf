# packaging
include(CPack)
set(CPACK_SET_DESTDIR ON)
set(CPACK_PACKAGE_VENDOR  "Alexander Gallego")
set(CPACK_PACKAGE_CONTACT "gallego.alexx@gmail.com")
set(CPACK_PACKAGE_VERSION "${SMF_VERSION}")
set(CPACK_STRIP_FILES "ON")
set(CPACK_PACKAGE_NAME "smf")
set(CPACK_DEBIAN_PACKAGE_SECTION "utilities")
