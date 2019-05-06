######################################
## 使用 pkg-config 寻找 libav*。

##```bash
## sudo add-apt-repository ppa:jonathonf/ffmpeg-4
## sudo apt-get install libavcodec-dev
## sudo apt-get install libavformat-dev
## sudo apt-get install libavfilter-dev
##```
######################################

cmake_minimum_required(VERSION 3.13)
project(ffmpeg_demo LANGUAGES C VERSION 1.2.7)

message(" C:    ${CMAKE_C_COMPILER}")
message(" C++:  ${CMAKE_CXX_COMPILER}")
message(" Source Dir:   ${PROJECT_SOURCE_DIR}")
message(" Binary Dir:   ${PROJECT_BINARY_DIR}")
message(" Version:      ${PROJECT_VERSION}")

set(CMAKE_C_STANDARD 11)

find_package(PkgConfig REQUIRED)
pkg_check_modules(LIBAV REQUIRED libavutil libavcodec libavformat libavfilter)

message(" LIBAV DIR : ${LIBAV_LIBRARY_DIRS}, ${LIBAV_INCLUDE_DIRS}")

link_directories(${LIBAV_LIBRARY_DIRS})

add_executable(ffmpeg_decode decode.c)
target_include_directories(ffmpeg_decode PUBLIC ${LIBAV_INCLUDE_DIRS})
#target_compile_options(ffmpeg_abc PUBLIC ${LIBAV_LIBRARY_DIRS})
target_link_libraries(ffmpeg_decode ${LIBAV_LIBRARIES})


add_executable(ffmpeg_remuxing remuxing.c)
target_include_directories(ffmpeg_remuxing PUBLIC ${LIBAV_INCLUDE_DIRS})
target_link_libraries(ffmpeg_remuxing ${LIBAV_LIBRARIES})


add_executable(ffmpeg_filter filter.c)
target_include_directories(ffmpeg_filter PUBLIC ${LIBAV_INCLUDE_DIRS})
target_link_libraries(ffmpeg_filter ${LIBAV_LIBRARIES})




