cmake_minimum_required(VERSION 3.20)
project(pyjni VERSION 1.0.1 DESCRIPTION "pyjni library")
include(GNUInstallDirs)
add_library(pyjni SHARED src/pyjni.c)
target_link_libraries(pyjni -llog)
set_target_properties(pyjni PROPERTIES
    VERSION ${PROJECT_VERSION}
    SOVERSION ${PROJECT_VERSION_MAJOR}
    PUBLIC_HEADER include/pyjni.h)
install(TARGETS pyjni
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    PUBLIC_HEADER DESTINATION ${CMAKE_INSTALL_INCLUDEDIR})