cmake_minimum_required(VERSION 2.8.12)

set(ASMJIT_STATIC TRUE)

if (NOT CMAKE_BUILD_TYPE)
	message(STATUS "No build type selected, default to Release")
	set(CMAKE_BUILD_TYPE "Release")
endif()

if(NOT CMAKE_SIZEOF_VOID_P EQUAL 8)
	message( FATAL_ERROR "RPCS3 can only be compiled on 64-bit platforms." )
endif()

# We use libpng's static library and don't need to build the shared library and run the tests
set(PNG_SHARED OFF CACHE BOOL "Build shared lib." FORCE)
set(PNG_TESTS  OFF CACHE BOOL "Build tests." FORCE)

add_definitions(-DCMAKE_BUILD)

add_subdirectory( 3rdparty/libpng )
# TODO: do real installation, including copying directory structure
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_RELEASE "${PROJECT_BINARY_DIR}/bin")
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_DEBUG "${PROJECT_BINARY_DIR}/bin")
add_subdirectory( Vulkan )
add_subdirectory( rpcs3 )

