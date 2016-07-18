cmake_minimum_required(VERSION 3.0)
project(libclassreader)

set(CMAKE_MACOSX_RPATH 1)

find_package(PkgConfig)
pkg_check_modules(GLIB2 glib-2.0)

set(CMAKE_C_FLAGS "-std=c99 -pedantic -Wall -D_POSIX_SOURCE")
include_directories(
    ${CMAKE_CURRENT_SOURCE_DIR}/include
    ${GLIB2_INCLUDE_DIRS}
)

link_directories(
    ${GLIB2_LIBRARY_DIRS}
)

add_library(classreader SHARED
    src/javaclass.c
    src/javafield.c
    src/javamethod.c
)

add_library(classreaderstatic STATIC
    src/javaclass.c
    src/javafield.c
    src/javamethod.c
)

set_target_properties(classreaderstatic PROPERTIES OUTPUT_NAME classreader)

target_link_libraries(classreader glib-2.0)

install(TARGETS
    classreader
    classreaderstatic
    DESTINATION
    lib
)

install(FILES
    include/javaclass.h
    include/javafield.h
    include/javamethod.h
    DESTINATION
    include/classreader
)
