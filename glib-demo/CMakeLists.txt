cmake_minimum_required(VERSION 3.12)
project(glib_demo C)

set(CMAKE_C_STANDARD 11)

include_directories(/usr/include/glib-2.0/)
include_directories(/usr/lib/x86_64-linux-gnu/glib-2.0/include/)

add_executable(glib_demo main.c)
target_link_libraries(glib_demo glib-2.0)