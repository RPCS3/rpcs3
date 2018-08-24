cmake_minimum_required(VERSION 3.8.2)

project(rpcs3)

option(WITH_GDB "WITH_GDB" OFF)
option(WITHOUT_LLVM "WITHOUT_LLVM" OFF)
option(USE_NATIVE_INSTRUCTIONS "USE_NATIVE_INSTRUCTIONS makes rpcs3 compile with -march=native, which is useful for local builds, but not good for packages." ON)

option(USE_ALSA "ALSA audio backend" ON)
option(USE_PULSE "PulseAudio audio backend" ON)
option(USE_LIBEVDEV "libevdev-based joystick support" ON)

option(USE_VULKAN "Vulkan render backend" ON)

set(CMAKE_MODULE_PATH "${CMAKE_CURRENT_LIST_DIR}/rpcs3/cmake_modules")

set(CMAKE_CXX_STANDARD 17)
include(CheckCXXCompilerFlag)

if (WITH_GDB)
	add_definitions(-DWITH_GDB_DEBUGGER)
endif()

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

if(WIN32)
	add_definitions(-DUNICODE)
	add_definitions(-D_WIN32_WINNT=0x0602)
endif()

add_subdirectory(Vulkan EXCLUDE_FROM_ALL)
add_subdirectory(asmjitsrc EXCLUDE_FROM_ALL)
add_subdirectory(3rdparty)

if(NOT WITHOUT_LLVM)
	if (EXISTS "${CMAKE_SOURCE_DIR}/llvmlibs")
		find_package(LLVM 7.0 CONFIG)
	endif()
	if(NOT LLVM_FOUND)
		message("LLVM will be built from the submodule.")

		set(LLVM_TARGETS_TO_BUILD "X86" CACHE INTERNAL "")
		option(LLVM_BUILD_RUNTIME OFF)
		option(LLVM_BUILD_TOOLS OFF)
		option(LLVM_INCLUDE_DOCS OFF)
		option(LLVM_INCLUDE_EXAMPLES OFF)
		option(LLVM_INCLUDE_TESTS OFF)
		option(LLVM_INCLUDE_TOOLS OFF)
		option(LLVM_INCLUDE_UTILS OFF)
		option(WITH_POLLY OFF)
		option(LLVM_ENABLE_CXX1Z TRUE)

		set(CXX_FLAGS_OLD ${CMAKE_CXX_FLAGS})

		if (MSVC)
			set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /D _SILENCE_ALL_CXX17_DEPRECATION_WARNINGS")
		endif()

		# LLVM needs to be built out-of-tree
		add_subdirectory(llvm llvm_build EXCLUDE_FROM_ALL)
		set(LLVM_DIR "${CMAKE_CURRENT_BINARY_DIR}/llvm_build/lib/cmake/llvm/")

		set(CMAKE_CXX_FLAGS ${CXX_FLAGS_OLD})

		# now tries to find LLVM again
		find_package(LLVM 7.0 CONFIG)
		if(NOT LLVM_FOUND)
			message(WARNING "Couldn't build LLVM from the submodule. You might need to run `git submodule update --init`")
		endif()
	endif()


	add_definitions(-DLLVM_AVAILABLE)
	set(LLVM_LIBS LLVMMCJIT LLVMX86CodeGen)

	add_library(3rdparty_llvm INTERFACE)
	target_link_libraries(3rdparty_llvm INTERFACE ${LLVM_LIBS})
	target_include_directories(3rdparty_llvm INTERFACE ${LLVM_INCLUDE_DIRS})
	target_compile_definitions(3rdparty_llvm INTERFACE ${LLVM_DEFINITIONS})

	add_library(3rdparty::llvm ALIAS 3rdparty_llvm)
else()
	add_library(3rdparty::llvm ALIAS 3rdparty_dummy_lib)
endif()

# TODO: do real installation, including copying directory structure
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_RELEASE "${PROJECT_BINARY_DIR}/bin")
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_DEBUG "${PROJECT_BINARY_DIR}/bin")
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_RELWITHDEBINFO "${PROJECT_BINARY_DIR}/bin")

add_subdirectory(Utilities)
add_subdirectory(rpcs3)
