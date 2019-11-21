cmake_minimum_required(VERSION 3.12)
project(base)

set(CMAKE_C_STANDARD 11)

find_package(PkgConfig REQUIRED)
pkg_check_modules(GLIB2 REQUIRED glib-2.0)

add_executable(exit exit.c staticTest.c)
target_include_directories(exit PUBLIC ${GLIB2_INCLUDE_DIRS})
target_link_libraries(exit ${GLIB2_LIBRARIES})