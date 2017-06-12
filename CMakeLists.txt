cmake_minimum_required(VERSION 2.8.12)

option(WITH_GDB "WITH_GDB" OFF)
option(WITHOUT_LLVM "WITHOUT_LLVM" OFF)

if (WITH_GDB)
	add_definitions(-DWITH_GDB_DEBUGGER)
endif()

set(ASMJIT_STATIC TRUE)

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

add_definitions(-DCMAKE_BUILD)

# We use libpng's static library and don't need to build the shared library and run the tests
set(PNG_SHARED OFF CACHE BOOL "Build shared lib." FORCE)
set(PNG_TESTS  OFF CACHE BOOL "Build tests." FORCE)

# Select the version of libpng to use, default is builtin
if (NOT USE_SYSTEM_LIBPNG)
	add_subdirectory( 3rdparty/libpng )
endif()

option(VULKAN_PREBUILT "" OFF)

# TODO: do real installation, including copying directory structure
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_RELEASE "${PROJECT_BINARY_DIR}/bin")
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_DEBUG "${PROJECT_BINARY_DIR}/bin")

add_subdirectory( Vulkan )
add_subdirectory( rpcs3 )

include_directories(3rdparty/hidapi/hidapi)
if(APPLE)
	add_subdirectory(3rdparty/hidapi/mac)
	#list(APPEND LIBS hidapi)
elseif(${CMAKE_SYSTEM_NAME} STREQUAL "Linux")
	add_subdirectory(3rdparty/hidapi/linux)
elseif(MSVC)
	add_subdirectory(3rdparty/hidapi/windows)
else()
	add_subdirectory(3rdparty/hidapi/libusb)
	#list(APPEND LIBS hidapi-libusb)
endif()

# Linux installation

if(UNIX AND NOT APPLE)
# Install the application icon and menu item
install(FILES rpcs3/rpcs3.svg
        DESTINATION ${CMAKE_INSTALL_PREFIX}/share/icons/hicolor/scalable/apps)
install(FILES rpcs3/rpcs3.png
        DESTINATION ${CMAKE_INSTALL_PREFIX}/share/icons/hicolor/48x48/apps)
install(FILES rpcs3/rpcs3.desktop
        DESTINATION ${CMAKE_INSTALL_PREFIX}/share/applications)
# Install the binary
install(FILES "${PROJECT_BINARY_DIR}/bin/rpcs3"
        DESTINATION ${CMAKE_INSTALL_PREFIX}/bin
        PERMISSIONS OWNER_EXECUTE OWNER_READ OWNER_WRITE GROUP_READ GROUP_EXECUTE WORLD_READ WORLD_EXECUTE) # i.e. 755
endif()
