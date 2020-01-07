cmake_minimum_required(VERSION 3.13)
project(libnice_demo VERSION 0.0.1)

find_package(PkgConfig REQUIRED)
pkg_check_modules(LIBNICE REQUIRED nice)

message(STATUS ">>> library info libnice >>>")
message(STATUS "libnice version : [${LIBNICE_VERSION}]")
message(STATUS "libnice header file dir : [${LIBNICE_INCLUDE_DIRS}]")
message(STATUS "libnice binary lib : [${LIBNICE_LIBRARIES}]")
message(STATUS "<<< library info libnice <<<")

add_executable(nice_demo nice.c)
target_include_directories(nice_demo PUBLIC ${LIBNICE_INCLUDE_DIRS})
target_link_libraries(nice_demo ${LIBNICE_LIBRARIES})
set_target_properties(nice_demo PROPERTIES RUNTIME_OUTPUT_DIRECTORY "~/Desktop")
