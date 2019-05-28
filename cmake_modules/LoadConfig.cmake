# Usefull to share cmake global var in codes :
# CMAKE_BUILD_TYPE
# GIT_REVISION

set(ASGARD_SOURCE_DIR ${CMAKE_BINARY_DIR}/asgard)

configure_file("${CMAKE_SOURCE_DIR}/conf.h.cmake" "${ASGARD_SOURCE_DIR}/conf.h")
configure_file("${CMAKE_SOURCE_DIR}/config.cpp.cmake" "${ASGARD_SOURCE_DIR}/config.cpp")
add_library(config "${ASGARD_SOURCE_DIR}/config.cpp")
