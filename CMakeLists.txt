cmake_minimum_required(VERSION 3.12)
project(MyC)

set(CMAKE_C_STANDARD 99)
set(SOURCE_FILES Mediainfo.c)

set(INC_DIR /usr/local/ffmpeg/include/)
set(LINK_DIR /usr/local/ffmpeg/lib/)

include_directories(${INC_DIR})
link_directories(${LINK_DIR})

add_executable(MyC ${SOURCE_FILES})
target_link_libraries(
        MyC
        avcodec
        avdevice
        avfilter
        avformat
        avutil
        postproc
        swresample
        swscale)

