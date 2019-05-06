cmake_minimum_required(VERSION 3.12)
project(ws-client)

set(CMAKE_C_STANDARD 11)

#message("${PROJECT_SOURCE_DIR}/include/spdlog/")

file(GLOB spdlog_src "${PROJECT_SOURCE_DIR}/include/spdlog/*")

include_directories(include)

add_executable(ws-client ${spdlog_src} queue.h WsClient.h WsClient.cpp Main.cpp)
set_property(TARGET ws-client PROPERTY CXX_STANDARD 11)
target_link_libraries(ws-client ${PROJECT_SOURCE_DIR}/lib/libwebsockets.a pthread)