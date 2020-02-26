cmake_minimum_required(VERSION 3.14.1)

project(rpcs3)

if(CMAKE_COMPILER_IS_GNUCXX)
	if(CMAKE_CXX_COMPILER_VERSION VERSION_LESS 9)
		message(FATAL_ERROR "RPCS3 requires at least gcc-9.")
	endif()
elseif(${CMAKE_CXX_COMPILER_ID} MATCHES "Clang")
	if(CMAKE_CXX_COMPILER_VERSION VERSION_LESS 9.0)
		message(FATAL_ERROR "RPCS3 requires at least clang-9.0.")
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
option(USE_COTIRE "Use precompiled headers" ON)

set(CMAKE_MODULE_PATH "${CMAKE_CURRENT_LIST_DIR}/rpcs3/cmake_modules")

set(CMAKE_CXX_STANDARD 20)
include(CheckCXXCompilerFlag)

if (NOT CMAKE_BUILD_TYPE)
	message(STATUS "No build type selected, default to Release")
	set(CMAKE_BUILD_TYPE "Release")
endif()

if(NOT CMAKE_BUILD_TYPE STREQUAL "Debug")
	add_definitions(-DNDEBUG)
endif()

if(NOT CMAKE_SIZEOF_VOID_P EQUAL 8)
	message( FATAL_ERROR "RPCS3 can only be compiled on 64-bit platforms." )
endif()

find_program(CCACHE_FOUND ccache)
if (CCACHE_FOUND)
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
	add_compile_options(/wd4530) # C++ exception handler used, but unwind semantics are not enabled
endif()

add_subdirectory(Vulkan EXCLUDE_FROM_ALL)
add_subdirectory(asmjitsrc EXCLUDE_FROM_ALL)
add_subdirectory(3rdparty)

unset(CMAKE_CXX_FLAGS)
unset(CMAKE_C_FLAGS)

if (NOT WIN32)
	add_compile_options(-pthread)
endif()

# TODO: do real installation, including copying directory structure
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_RELEASE "${PROJECT_BINARY_DIR}/bin")
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_DEBUG "${PROJECT_BINARY_DIR}/bin")
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_RELWITHDEBINFO "${PROJECT_BINARY_DIR}/bin")

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

add_subdirectory(rpcs3)
