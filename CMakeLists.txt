cmake_minimum_required(VERSION 3.28)

find_program(CCACHE_PATH ccache HINTS ENV PATH)
if(CCACHE_PATH)
  message(STATUS "Using ccache: ${CCACHE_PATH}")
  set(CMAKE_C_COMPILER_LAUNCHER "${CCACHE_PATH}")
  set(CMAKE_CXX_COMPILER_LAUNCHER "${CCACHE_PATH}")
endif()

project(rpcs3 LANGUAGES C CXX)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(CMAKE_POSITION_INDEPENDENT_CODE ON)

if(CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
    if(CMAKE_CXX_COMPILER_VERSION VERSION_LESS 13)
        message(FATAL_ERROR "RPCS3 requires at least gcc-13.")
    endif()
elseif(CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
    if(CMAKE_CXX_COMPILER_VERSION VERSION_LESS 19.0)
        message(FATAL_ERROR "RPCS3 requires at least clang-19.0.")
    endif()
endif()

if(APPLE OR WIN32)
    set(USE_SYSTEM_OPENAL_DEFAULT OFF)
else()
    set(USE_SYSTEM_OPENAL_DEFAULT ON)
endif()

option(USE_NATIVE_INSTRUCTIONS "USE_NATIVE_INSTRUCTIONS makes rpcs3 compile with -march=native, which is useful for local builds, but not good for packages." ON)
option(WITH_LLVM "Enable usage of LLVM library" ON)
option(BUILD_LLVM "Build LLVM from git submodule" OFF)
option(STATIC_LINK_LLVM "Link against LLVM statically. This will get set to ON if you build LLVM from the submodule." OFF)
option(USE_FAUDIO "FAudio audio backend" ON)
option(USE_LIBEVDEV "libevdev-based joystick support" ON)
option(USE_DISCORD_RPC "Discord rich presence integration" OFF)
option(USE_VULKAN "Vulkan render backend" ON)
option(USE_PRECOMPILED_HEADERS "Use precompiled headers" OFF)
option(USE_SDL "Enables SDL input handler" OFF)
option(USE_SYSTEM_CUBEB "Prefer system cubeb instead of the builtin one" OFF)
option(USE_SYSTEM_CURL "Prefer system Curl instead of the prebuild one" ON)
option(USE_SYSTEM_FAUDIO "Prefer system FAudio instead of the builtin one" OFF)
option(USE_SYSTEM_FFMPEG "Prefer system ffmpeg instead of the prebuild one" OFF)
option(USE_SYSTEM_PROTOBUF "Prefer system protobuf instead of the builtin one" OFF)
option(USE_SYSTEM_GLSLANG "Prefer system glslang instead of the builtin one" OFF)
option(USE_SYSTEM_HIDAPI "Prefer system hidapi instead of the builtin one" OFF)
option(USE_SYSTEM_LIBPNG "Prefer system libpng instead of the builtin one" OFF)
option(USE_SYSTEM_LIBUSB "Prefer system libusb instead of the builtin one" OFF)
option(USE_SYSTEM_MINIUPNPC "Prefer system MiniUPnPc instead of the builtin one" OFF)
option(USE_SYSTEM_MVK "Prefer system MoltenVK instead of the builtin one" OFF)
option(USE_SYSTEM_OPENAL "Prefer system OpenAL instead of the prebuild one" ${USE_SYSTEM_OPENAL_DEFAULT})
option(USE_SYSTEM_OPENCV "Prefer system OpenCV instead of the builtin one" ON)
option(USE_SYSTEM_PUGIXML "Prefer system pugixml instead of the builtin one" OFF)
option(USE_SYSTEM_RTMIDI "Prefer system RtMidi instead of the builtin one" OFF)
option(USE_SYSTEM_SDL "Prefer system SDL instead of the builtin one" ON)
option(USE_SYSTEM_VULKAN_MEMORY_ALLOCATOR "Prefer system Vulkan Memory Allocator instead of the builtin one" OFF)
option(USE_SYSTEM_WOLFSSL "Prefer system wolfSSL instead of the builtin one" OFF)
option(USE_SYSTEM_ZLIB "Prefer system ZLIB instead of the builtin one" ON)
option(USE_SYSTEM_ZSTD "Prefer system zstd instead of the builtin one" OFF)
option(HAS_MEMORY_BREAKPOINTS "Add support for memory breakpoints to the interpreter" OFF)
option(USE_LTO "Use LTO for building" ON)
option(BUILD_RPCS3_TESTS "Build RPCS3 unit tests." OFF)
option(RUN_RPCS3_TESTS "Run RPCS3 unit tests. Requires BUILD_RPCS3_TESTS" OFF)
option(USE_GAMEMODE "Choose whether to enable GameMode features or not." ON)

set(CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/buildfiles/cmake")

include(CheckCXXCompilerFlag)

get_property(IS_MULTI_CONFIG GLOBAL PROPERTY GENERATOR_IS_MULTI_CONFIG)
if(IS_MULTI_CONFIG)
    # TODO(cjj19970505@live.cn)
    # Currently DicordRPC is included as a binary compiled with /MT flag.
    # We need all 4 binaries(/MT /MTd /MD /MDd) or including the source instead of binary.
    set(USE_DISCORD_RPC OFF CACHE BOOL "Discord RPC is only avaliable with single-config generator" FORCE)
else()
    if(NOT CMAKE_BUILD_TYPE)
    message(STATUS "No build type selected, default to Release")
        set(CMAKE_BUILD_TYPE "Release")
    endif()
endif()

if(CMAKE_BUILD_TYPE MATCHES "Debug" AND NOT MSVC)
    add_compile_definitions(_DEBUG)
endif()

if(MSVC)
    add_compile_options("$<$<COMPILE_LANGUAGE:C,CXX>:/MP>")
endif()

if(NOT CMAKE_SIZEOF_VOID_P EQUAL 8)
    message(FATAL_ERROR "RPCS3 can only be compiled on 64-bit platforms.")
endif()

if(APPLE AND CMAKE_OSX_ARCHITECTURES STREQUAL "arm64")
    include_directories(/opt/homebrew/include)
    link_directories(/opt/homebrew/lib)
endif()

if(MSVC)
    add_compile_options("$<$<COMPILE_LANGUAGE:C,CXX>:/wd4530;/utf-8>") # C++ exception handler used, but unwind semantics are not enabled
endif()

add_subdirectory(3rdparty)

if (DISABLE_LTO)
    if (CMAKE_C_FLAGS)
        string(REGEX REPLACE "-flto[^ ]*" "" CMAKE_C_FLAGS ${CMAKE_C_FLAGS})
    endif()
    if (CMAKE_CXX_FLAGS)
        string(REGEX REPLACE "-flto[^ ]*" "" CMAKE_CXX_FLAGS ${CMAKE_CXX_FLAGS})
    endif()
endif()

string(FIND "${CMAKE_C_FLAGS} ${CMAKE_CXX_FLAGS}" "-flto" FOUND_LTO)
if (NOT FOUND_LTO EQUAL -1)
    message(FATAL_ERROR "RPCS3 doesn't support building with LTO, use -DDISABLE_LTO=TRUE to force-disable it")
endif()

## Look for Gamemode if its installed on Linux
if(LINUX)
    ## User chooses whether to Enable GameMode features or not
    if(USE_GAMEMODE)
         add_compile_definitions(GAMEMODE_AVAILABLE)
    endif()
endif()

# TODO: do real installation, including copying directory structure
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_RELEASE "${PROJECT_BINARY_DIR}/bin")
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_DEBUG "${PROJECT_BINARY_DIR}/bin")
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_RELWITHDEBINFO "${PROJECT_BINARY_DIR}/bin")

if(BUILD_RPCS3_TESTS)
    enable_testing()
endif()
add_subdirectory(rpcs3)

set_directory_properties(PROPERTIES VS_STARTUP_PROJECT rpcs3)
