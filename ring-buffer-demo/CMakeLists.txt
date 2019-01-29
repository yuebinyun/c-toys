cmake_minimum_required(VERSION 3.12)
project(ring C)

set(CMAKE_C_STANDARD 11)

add_executable(ring main.c ring_buffer.c ring_buffer.h)