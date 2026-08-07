cmake_minimum_required(VERSION 3.28)
project("rpcs3-android")

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_POSITION_INDEPENDENT_CODE on)

get_filename_component(RPCS3_SOURCE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/../../../../.." ABSOLUTE)
get_filename_component(RPCS3_ANDROID_ROOT "${CMAKE_CURRENT_SOURCE_DIR}/../../../.." ABSOLUTE)

if(NOT DEFINED RPCS3_PREBUILT_DIR)
    set(RPCS3_PREBUILT_DIR "${RPCS3_ANDROID_ROOT}/prebuilt")
endif()

set(FFMPEG_PATH "${RPCS3_PREBUILT_DIR}/ffmpeg-${ANDROID_ABI}")
set(LLVM_DIR "${RPCS3_PREBUILT_DIR}/llvm-${ANDROID_ABI}/lib/cmake/llvm")

if(NOT EXISTS "${FFMPEG_PATH}/lib/libavcodec.a")
    message(FATAL_ERROR "FFmpeg for ${ANDROID_ABI} not found at ${FFMPEG_PATH}. Run build-ffmpeg-android.sh first.")
endif()

if(NOT EXISTS "${LLVM_DIR}")
    message(FATAL_ERROR "LLVM for ${ANDROID_ABI} not found at ${LLVM_DIR}. Run build-llvm-android.sh first.")
endif()

function(import_ffmpeg_library name)
    add_library(ffmpeg::${name} STATIC IMPORTED GLOBAL)
    set_property(TARGET ffmpeg::${name} PROPERTY IMPORTED_LOCATION "${FFMPEG_PATH}/lib/lib${name}.a")
    set_property(TARGET ffmpeg::${name} PROPERTY INTERFACE_INCLUDE_DIRECTORIES "${FFMPEG_PATH}/include")
endfunction()

import_ffmpeg_library(avcodec)
import_ffmpeg_library(avformat)
import_ffmpeg_library(avutil)
import_ffmpeg_library(swscale)
import_ffmpeg_library(swresample)

add_library(3rdparty_ffmpeg INTERFACE)
target_link_libraries(3rdparty_ffmpeg INTERFACE
    ffmpeg::avformat
    ffmpeg::avcodec
    ffmpeg::avutil
    ffmpeg::swscale
    ffmpeg::swresample
)

set(USE_SYSTEM_LIBUSB off)
set(USE_SYSTEM_CURL off)
set(USE_DISCORD_RPC off)
set(USE_SYSTEM_OPENCV off)
set(USE_SYSTEM_FFMPEG off)
set(USE_SYSTEM_PUGIXML off)
set(USE_SYSTEM_ZSTD off)
set(USE_SYSTEM_RTMIDI off)
set(USE_SYSTEM_PROTOBUF off)
set(USE_FAUDIO off)
set(USE_SDL off)
set(USE_SYSTEM_SDL off)
set(BUILD_LLVM off)
set(STATIC_LINK_LLVM on)
set(DISABLE_LTO on)
set(USE_LTO off)
set(USE_OPENSL off)
set(ASMJIT_NO_SHM_OPEN on)
set(USE_SYSTEM_ZLIB off)
set(USE_LIBEVDEV off)
set(USE_PRECOMPILED_HEADERS off)

add_subdirectory(${RPCS3_SOURCE_DIR} rpcs3 EXCLUDE_FROM_ALL)

add_library(${CMAKE_PROJECT_NAME} SHARED
    native-lib.cpp
    iso.cpp
    ${RPCS3_SOURCE_DIR}/rpcs3/Input/ds3_pad_handler.cpp
    ${RPCS3_SOURCE_DIR}/rpcs3/Input/ds4_pad_handler.cpp
    ${RPCS3_SOURCE_DIR}/rpcs3/Input/dualsense_pad_handler.cpp
    ${RPCS3_SOURCE_DIR}/rpcs3/Input/evdev_joystick_handler.cpp
    ${RPCS3_SOURCE_DIR}/rpcs3/Input/hid_pad_handler.cpp
    ${RPCS3_SOURCE_DIR}/rpcs3/Input/virtual_pad_handler.cpp
    ${RPCS3_SOURCE_DIR}/rpcs3/Input/mm_joystick_handler.cpp
    ${RPCS3_SOURCE_DIR}/rpcs3/Input/pad_thread.cpp
    ${RPCS3_SOURCE_DIR}/rpcs3/Input/product_info.cpp
    ${RPCS3_SOURCE_DIR}/rpcs3/Input/ps_move_calibration.cpp
    ${RPCS3_SOURCE_DIR}/rpcs3/Input/ps_move_config.cpp
    ${RPCS3_SOURCE_DIR}/rpcs3/Input/ps_move_handler.cpp
    ${RPCS3_SOURCE_DIR}/rpcs3/Input/ps_move_tracker.cpp
    ${RPCS3_SOURCE_DIR}/rpcs3/Input/raw_mouse_config.cpp
    ${RPCS3_SOURCE_DIR}/rpcs3/Input/raw_mouse_handler.cpp
    ${RPCS3_SOURCE_DIR}/rpcs3/Input/sdl_pad_handler.cpp
    ${RPCS3_SOURCE_DIR}/rpcs3/Input/skateboard_pad_handler.cpp
    ${RPCS3_SOURCE_DIR}/rpcs3/rpcs3_version.cpp
)

target_include_directories(${CMAKE_PROJECT_NAME} PUBLIC ${RPCS3_SOURCE_DIR}/rpcs3)

target_link_libraries(${CMAKE_PROJECT_NAME}
    android
    log
    nativehelper
    rpcs3_emu
    3rdparty::libusb
    3rdparty::hidapi
    3rdparty::wolfssl
    3rdparty::libcurl
    3rdparty::zlib
    3rdparty::fusion
)
