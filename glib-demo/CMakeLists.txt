cmake_minimum_required(VERSION 3.13)
project(glib_demo VERSION 0.0.1)

find_package(PkgConfig REQUIRED)
pkg_check_modules(GLIB2 REQUIRED glib-2.0)

add_executable(glib_demo main.c)
target_link_directories(glib_demo PUBLIC ${GLIB2_INCLUDE_DIRS})
target_link_libraries(glib_demo ${GLIB2_LIBRARIES})