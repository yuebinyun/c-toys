cmake_minimum_required(VERSION 3.13)
project(glib_demo VERSION 0.0.1)

find_package(PkgConfig REQUIRED)
pkg_check_modules(GLIB2 REQUIRED glib-2.0)

add_executable(glib_demo main.c)
target_include_directories(glib_demo PUBLIC ${GLIB2_INCLUDE_DIRS})
target_link_directories(glib_demo PUBLIC ${GLIB2_INCLUDE_DIRS})
target_link_libraries(glib_demo ${GLIB2_LIBRARIES})

add_executable(gsource gsource.c)
target_include_directories(gsource PUBLIC ${GLIB2_INCLUDE_DIRS})
target_link_directories(gsource PUBLIC ${GLIB2_INCLUDE_DIRS})
target_link_libraries(gsource ${GLIB2_LIBRARIES} gobject-2.0 gio-2.0)

add_executable(source source.h test_source.c)
target_include_directories(source PUBLIC ${GLIB2_INCLUDE_DIRS})
target_link_directories(source PUBLIC ${GLIB2_INCLUDE_DIRS})
target_link_libraries(source ${GLIB2_LIBRARIES} gobject-2.0 gio-2.0)