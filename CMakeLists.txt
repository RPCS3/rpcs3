cmake_minimum_required(VERSION 3.16.9)

project(rpcs3)

if(${CMAKE_CXX_COMPILER_ID} MATCHES "GNU")
    if(CMAKE_CXX_COMPILER_VERSION VERSION_LESS 11)
        message(FATAL_ERROR "RPCS3 requires at least gcc-11.")
    endif()
elseif(${CMAKE_CXX_COMPILER_ID} MATCHES "Clang")
    if(CMAKE_CXX_COMPILER_VERSION VERSION_LESS 12.0)
        message(FATAL_ERROR "RPCS3 requires at least clang-12.0.")
    endif()
endif()

option(USE_NATIVE_INSTRUCTIONS "USE_NATIVE_INSTRUCTIONS makes rpcs3 compile with -march=native, which is useful for local builds, but not good for packages." ON)
option(WITH_LLVM "Enable usage of LLVM library" ON)
option(BUILD_LLVM_SUBMODULE "Build LLVM from git submodule" ON)
option(USE_ALSA "ALSA audio backend" ON)
option(USE_PULSE "PulseAudio audio backend" ON)
option(USE_FAUDIO "FAudio audio backend" ON)
option(USE_LIBEVDEV "libevdev-based joystick support" ON)
option(USE_DISCORD_RPC "Discord rich presence integration" ON)
option(USE_SYSTEM_ZLIB "Prefer system ZLIB instead of the builtin one" ON)
option(USE_VULKAN "Vulkan render backend" ON)
option(USE_PRECOMPILED_HEADERS "Use precompiled headers" OFF)

set(CMAKE_MODULE_PATH "${CMAKE_CURRENT_LIST_DIR}/buildfiles/cmake")

include(CheckCXXCompilerFlag)

# TODO(cjj19970505@live.cn)
# Currently cmake config scripts are single-config generator only.
get_property(IS_MULTI_CONFIG GLOBAL PROPERTY GENERATOR_IS_MULTI_CONFIG)
if(IS_MULTI_CONFIG)
    message(FATAL_ERROR "Multi-config generators are not supported.")
endif()

if(NOT CMAKE_BUILD_TYPE)
    message(STATUS "No build type selected, default to Release")
    set(CMAKE_BUILD_TYPE "Release")
endif()

if(CMAKE_BUILD_TYPE MATCHES "Debug" AND NOT MSVC)
    add_definitions(-D_DEBUG)
endif()

if(MSVC)
    option(USE_MSVC_STATIC_CRT "Use static MSVC C runtime" OFF)

    # TODO(cjj19970505@live.cno)
    # DiscordRPC binary in 3rdparty is compiled /MT
    # So theoretically we should enable DiscordRPC in Release and static CRT build
    # since we might encounter some rumtime issues when more than one CRT version are presented.
    # https://docs.microsoft.com/en-us/cpp/c-runtime-library/crt-library-features?view=msvc-160#what-problems-exist-if-an-application-uses-more-than-one-crt-version
    # Add other DiscordRPC binaries(compiled with /MTd, /MD, /MDd) or compile it from source may address this issue.
    if(NOT(CMAKE_BUILD_TYPE MATCHES "Release" AND USE_MSVC_STATIC_CRT))
        set(USE_DISCORD_RPC OFF CACHE BOOL "Discord RPC is only available in Release and static CRT build." FORCE)
    endif()
    
    if(USE_MSVC_STATIC_CRT)
        set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")
    else()
        # though doc ( https://cmake.org/cmake/help/latest/variable/CMAKE_MSVC_RUNTIME_LIBRARY.html )
        # says if that property is not set then CMake uses the default value MultiThreaded$<$<CONFIG:Debug>:Debug>DLL 
        # to select a MSVC runtime library.
        # But yaml-cpp set /MT(d) if CMAKE_MSVC_RUNTIME_LIBRARY is undefined
        # So we have to define it explicitly
        set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>DLL")
    endif()

    # TODO(cjj19970505@live.cn)
    # offical QT uses dynamic CRT.
    # When building our lib with static CRT and debug build type
    # and linking with Qt with dynmaic CRT and debug build, 
    # error is encountered in runtime (which is expected). 
    # But building our lib with static CRT and release build type, 
    # and linking with Qt with dynamic CRT and release build seems to be working, 
    # which is the same config with VS solution.
    # (though technically it might still have some hidden errors).
    # So we allow static CRT in both relase and debug build, but prompt warning in debug build.
    # For more info:
    # https://docs.microsoft.com/en-us/cpp/c-runtime-library/crt-library-features?view=msvc-160#what-problems-exist-if-an-application-uses-more-than-one-crt-version
    # https://wiki.qt.io/Technical_FAQ#Why_does_a_statically_built_Qt_use_the_dynamic_Visual_Studio_runtime_libraries_.3F_Do_I_need_to_deploy_those_with_my_application_.3F
    if(USE_MSVC_STATIC_CRT AND CMAKE_BUILD_TYPE MATCHES "Debug")
        message(AUTHOR_WARNING "Debug build currently can not work with static CRT.")
    endif()
endif()

if(NOT CMAKE_SIZEOF_VOID_P EQUAL 8)
    message( FATAL_ERROR "RPCS3 can only be compiled on 64-bit platforms." )
endif()

find_program(CCACHE_FOUND ccache)
if(CCACHE_FOUND)
    set_property(GLOBAL PROPERTY RULE_LAUNCH_COMPILE ccache)
    set_property(GLOBAL PROPERTY RULE_LAUNCH_LINK ccache)
endif()

if(WIN32)
    add_definitions(-DUNICODE)
    add_definitions(-D_WIN32_WINNT=0x0602)
endif()

if(APPLE)
    include_directories(/opt/local/include)
    link_directories(/opt/local/lib)
endif()

# Warnings are silenced for 3rdparty code
set(CMAKE_CXX_FLAGS -w)
set(CMAKE_C_FLAGS -w)
set(LLVM_ENABLE_WARNINGS OFF CACHE BOOL "")

if(MSVC)
    add_compile_options(/wd4530 /utf-8) # C++ exception handler used, but unwind semantics are not enabled
endif()

add_subdirectory(3rdparty)

unset(CMAKE_CXX_FLAGS)
unset(CMAKE_C_FLAGS)

if(NOT WIN32)
    add_compile_options(-pthread)
endif()

# TODO: do real installation, including copying directory structure
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_RELEASE "${PROJECT_BINARY_DIR}/bin")
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_DEBUG "${PROJECT_BINARY_DIR}/bin")
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_RELWITHDEBINFO "${PROJECT_BINARY_DIR}/bin")

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

add_subdirectory(rpcs3)
