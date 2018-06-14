cmake_minimum_required(VERSION 3.1)

option(WITH_GDB "WITH_GDB" OFF)
option(WITHOUT_LLVM "WITHOUT_LLVM" OFF)
option(USE_NATIVE_INSTRUCTIONS "USE_NATIVE_INSTRUCTIONS makes rpcs3 compile with -march=native, which is useful for local builds, but not good for packages." ON)

if (WITH_GDB)
	add_definitions(-DWITH_GDB_DEBUGGER)
endif()

set(ASMJIT_EMBED TRUE)
set(ASMJIT_DIR "${CMAKE_CURRENT_LIST_DIR}/asmjit" CACHE PATH "Location of 'asmjit'")
include("${ASMJIT_DIR}/CMakeLists.txt")

if (NOT CMAKE_BUILD_TYPE)
	message(STATUS "No build type selected, default to Release")
	set(CMAKE_BUILD_TYPE "Release")
endif()

if(NOT CMAKE_SIZEOF_VOID_P EQUAL 8)
	message( FATAL_ERROR "RPCS3 can only be compiled on 64-bit platforms." )
endif()

find_program(CCACHE_FOUND ccache)
if (CCACHE_FOUND)
	set_property(GLOBAL PROPERTY RULE_LAUNCH_COMPILE ccache)
	set_property(GLOBAL PROPERTY RULE_LAUNCH_LINK ccache)
endif()

find_package(ZLIB QUIET)
if (NOT ZLIB_FOUND)
    add_subdirectory(3rdparty/zlib EXCLUDE_FROM_ALL)
    set(ZLIB_INCLUDE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/3rdparty/zlib" "${CMAKE_CURRENT_BINARY_DIR}/3rdparty/zlib" CACHE INTERNAL "")
    set(ZLIB_LIBRARY zlibstatic)
endif()

# Select the version of libpng to use, default is builtin
if (NOT USE_SYSTEM_LIBPNG)
	# We use libpng's static library and don't need to build the shared library and run the tests
	set(PNG_SHARED OFF CACHE BOOL "Build shared lib" FORCE)
	set(PNG_TESTS OFF CACHE BOOL "Build libpng tests" FORCE)
	set(SKIP_INSTALL_ALL ON)
	add_subdirectory(3rdparty/libpng EXCLUDE_FROM_ALL)
endif()

add_subdirectory(3rdparty/pugixml EXCLUDE_FROM_ALL)

# TODO: do real installation, including copying directory structure
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_RELEASE "${PROJECT_BINARY_DIR}/bin")
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_DEBUG "${PROJECT_BINARY_DIR}/bin")
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_RELWITHDEBINFO "${PROJECT_BINARY_DIR}/bin")

add_subdirectory(Vulkan EXCLUDE_FROM_ALL)
add_subdirectory(rpcs3)

include_directories(3rdparty/hidapi/hidapi)
if(APPLE)
	add_subdirectory(3rdparty/hidapi/mac EXCLUDE_FROM_ALL)
	#list(APPEND LIBS hidapi)
elseif(CMAKE_SYSTEM MATCHES "Linux")
	add_subdirectory(3rdparty/hidapi/linux EXCLUDE_FROM_ALL)
elseif(MSVC)
	add_subdirectory(3rdparty/hidapi/windows EXCLUDE_FROM_ALL)
else()
	add_subdirectory(3rdparty/hidapi/libusb EXCLUDE_FROM_ALL)
	#list(APPEND LIBS hidapi-libusb)
endif()

set(YAML_CPP_BUILD_TESTS OFF CACHE BOOL "Enable testing" FORCE)
set(YAML_CPP_BUILD_TOOLS OFF CACHE BOOL "Enable parse tools" FORCE)
set(YAML_CPP_BUILD_CONTRIB OFF CACHE BOOL "Enable contrib stuff in library" FORCE)
add_subdirectory(3rdparty/yaml-cpp EXCLUDE_FROM_ALL)

set(XXHASH_BUNDLED_MODE ON)
set(BUILD_SHARED_LIBS OFF CACHE BOOL "Make xxHash build static libs")
add_subdirectory(3rdparty/xxHash/cmake_unofficial)
