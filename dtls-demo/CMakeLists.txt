cmake_minimum_required(VERSION 3.12)

project(dtls_demo C)

set(CMAKE_C_STANDARD 99)
set(EXECUTABLE_OUTPUT_PATH ${PROJECT_SOURCE_DIR})

link_libraries(ssl crypto dl pthread)

add_executable(dtls_demo server2.c)

set_target_properties(dtls_demo PROPERTIES  SUFFIX ".app")