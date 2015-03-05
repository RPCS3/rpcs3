cmake_minimum_required(VERSION 2.8)

set (CMAKE_MODULE_PATH "${CMAKE_CURRENT_LIST_DIR}/cmake_modules")
include(cotire)

project(rpcs3)

# Generate git-version.h at build time.
add_custom_target(GitVersion ALL
	DEPENDS something_that_never_exists)
add_custom_command(OUTPUT something_that_never_exists
	COMMAND ${CMAKE_COMMAND} -DSOURCE_DIR=${CMAKE_CURRENT_SOURCE_DIR}
		-P ${CMAKE_CURRENT_SOURCE_DIR}/git-version.cmake)

if (CMAKE_COMPILER_IS_GNUCXX)
	if(CMAKE_CXX_COMPILER_VERSION VERSION_LESS 4.7.0)
		message(FATAL_ERROR "GCC ${CMAKE_CXX_COMPILER_VERSION} is too old.")
	endif()
	# Warnings
	add_definitions(-Wno-attributes -Wno-enum-compare)
	set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wno-invalid-offsetof")
elseif("${CMAKE_CXX_COMPILER_ID}" STREQUAL "Clang")
	# TODO: stdlib?
endif()

if (NOT MSVC)
	add_definitions(-DwxGUI)
	set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11 -fexceptions")
	set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -g -D_DEBUG")
	set(CMAKE_CXX_FLAGS_MINSIZEREL "${CMAKE_CXX_FLAGS_MINSIZEREL} -Os -D_NDEBUG")
	set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -O1 -D_NDEBUG")
	set(CMAKE_CXX_FLAGS_RELWITHDEBINFO "${CMAKE_CXX_FLAGS_RELWITHDEBINFO} -O1 -g -D_NDEBUG")
	set(CMAKE_C_FLAGS_DEBUG "${CMAKE_C_FLAGS_DEBUG} -g -D_DEBUG")
	set(CMAKE_C_FLAGS_MINSIZEREL "${CMAKE_C_FLAGS_MINSIZEREL} -Os -D_NDEBUG")
	set(CMAKE_C_FLAGS_RELEASE "${CMAKE_C_FLAGS_RELEASE} -O1 -D_NDEBUG")
	set(CMAKE_C_FLAGS_RELWITHDEBINFO "${CMAKE_C_FLAGS_RELWITHDEBINFO} -O1 -g -D_NDEBUG")
	add_definitions(-msse2 -mcx16)
endif()

if (APPLE)
	set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -I/opt/X11/include")
	set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -I/opt/X11/include")
endif()

if(${CMAKE_SYSTEM_NAME} MATCHES "Linux")
	#on some Linux distros shm_unlink and similar functions are in librt only
	set(ADDITIONAL_LIBS "rt")
elseif(UNIX)
	#it seems like glibc includes the iconv functions we use but other libc
	#implementations like the one on OSX don't seem implement them
	set(ADDITIONAL_LIBS "iconv")
else()
	set(ADDITIONAL_LIBS "")
endif()

If( NOT RPCS3_SRC_DIR)
  SET(RPCS3_SRC_DIR ${CMAKE_CURRENT_LIST_DIR})
  Message("-- Initializing RPCS3_SRC_DIR=${RPCS3_SRC_DIR}")
Else()
  Message("-- Using Custom RPCS3_SRC_DIR=${RPCS3_SRC_DIR}")
EndIf()

set(CMAKE_MODULE_PATH "${RPCS3_SRC_DIR}/cmake_modules")
# TODO: do real installation, including copying directory structure
set(EXECUTABLE_OUTPUT_PATH "${PROJECT_BINARY_DIR}/../bin")
#include(cotire)

add_definitions(-DGL_GLEXT_PROTOTYPES)
add_definitions(-DGLX_GLXEXT_PROTOTYPES)

find_package(wxWidgets COMPONENTS core base net aui gl xml REQUIRED)
find_package(GLEW REQUIRED)
find_package(OpenGL REQUIRED)
find_package(ZLIB REQUIRED)
find_package(OpenAL REQUIRED)
find_package(LLVM REQUIRED CONFIG)

include("${wxWidgets_USE_FILE}")

if(APPLE)
	set(PLATFORM_ARCH "macosx/x86_64")
elseif(WIN32)
	set(PLATFORM_ARCH "Windows/x86_64")
else()
	set(PLATFORM_ARCH "linux/x86_64")
endif()

include_directories(
${wxWidgets_INCLUDE_DIRS}
${OPENAL_INCLUDE_DIR}
${LLVM_INCLUDE_DIRS}
"${RPCS3_SRC_DIR}/../ffmpeg/${PLATFORM_ARCH}/include"
"${RPCS3_SRC_DIR}"
"${RPCS3_SRC_DIR}/Loader"
"${RPCS3_SRC_DIR}/Crypto"
"${RPCS3_SRC_DIR}/.."
"${RPCS3_SRC_DIR}/../asmjit/src/asmjit"
)

add_definitions(${LLVM_DEFINITIONS})
add_definitions(-DLLVM_AVAILABLE)
if (CMAKE_BUILD_TYPE STREQUAL "Release")
	llvm_map_components_to_libnames(LLVM_LIBS jit vectorize x86codegen x86disassembler)
else()
	llvm_map_components_to_libnames(LLVM_LIBS jit vectorize x86codegen x86disassembler mcdisassembler)
endif()

link_directories("${RPCS3_SRC_DIR}/../ffmpeg/${PLATFORM_ARCH}/lib")

get_property(dirs DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR} PROPERTY INCLUDE_DIRECTORIES)
foreach(dir ${dirs})
  message(STATUS "dir='${dir}'")
endforeach()

file(
GLOB_RECURSE
RPCS3_SRC
"${RPCS3_SRC_DIR}/rpcs3.cpp"
"${RPCS3_SRC_DIR}/Ini.cpp"
"${RPCS3_SRC_DIR}/../Utilities/GNU.cpp"
"${RPCS3_SRC_DIR}/Emu/*"
"${RPCS3_SRC_DIR}/Gui/*"
"${RPCS3_SRC_DIR}/Loader/*"
"${RPCS3_SRC_DIR}/Crypto/*"
"${RPCS3_SRC_DIR}/../Utilities/*"
)

list(REMOVE_ITEM RPCS3_SRC ${RPCS3_SRC_DIR}/../Utilities/simpleini/ConvertUTF.c)
set_source_files_properties(${RPCS3_SRC_DIR}/Emu/Cell/PPULLVMRecompiler.cpp PROPERTIES COMPILE_FLAGS -fno-rtti)

add_executable(rpcs3 ${RPCS3_SRC})

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -L${CMAKE_CURRENT_BINARY_DIR}/../asmjit/") #hack because the asmjit cmake file force fno exceptions
target_link_libraries(rpcs3  asmjit.a  ${wxWidgets_LIBRARIES} ${OPENAL_LIBRARY} ${GLEW_LIBRARY} ${OPENGL_LIBRARIES} libavformat.a libavcodec.a libavutil.a libswresample.a libswscale.a ${ZLIB_LIBRARIES} ${LLVM_LIBS} ${ADDITIONAL_LIBS} )

set_target_properties(rpcs3 PROPERTIES COTIRE_CXX_PREFIX_HEADER_INIT "${RPCS3_SRC_DIR}/stdafx.h")
cotire(rpcs3)

