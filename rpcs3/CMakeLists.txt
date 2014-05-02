cmake_minimum_required(VERSION 2.8)

set (CMAKE_MODULE_PATH "${CMAKE_SOURCE_DIR}/rpcs3/cmake_modules")
include(cotire)

project(rpcs3)

if (CMAKE_COMPILER_IS_GNUCXX)
	if(CMAKE_CXX_COMPILER_VERSION VERSION_LESS 4.7.0)
		set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++0x")
	else()
		set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11")
	endif()
	add_definitions(-DwxGUI)
	#add_definitions(-D__WXGTK__)
	#add_definitions(-Wfatal-errors)
	add_definitions(-w) # TODO: remove me
	set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fexceptions")
	add_definitions(-fpermissive) # TODO: remove me
	add_definitions(-g)	# Debugging!!
	add_definitions(-msse2)
elseif("${CMAKE_CXX_COMPILER_ID}" STREQUAL "Clang")
	add_definitions(-DwxGUI)
	set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++0x")
	set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fexceptions")
endif()

If( NOT RPCS3_SRC_DIR)
  SET(RPCS3_SRC_DIR ${CMAKE_CURRENT_LIST_DIR})
  Message("-- Initializing RPCS3_SRC_DIR=${RPCS3_SRC_DIR}")
Else()
  Message("-- Using Custom RPCS3_SRC_DIR=${RPCS3_SRC_DIR}")
EndIf()

set(CMAKE_MODULE_PATH "${RPCS3_SRC_DIR}/cmake_modules")
set(EXECUTABLE_OUTPUT_PATH "${RPCS3_SRC_DIR}/../bin") # TODO: do real installation, including copying directory structure
#include(cotire)

add_definitions(-DGL_GLEXT_PROTOTYPES)
add_definitions(-DGLX_GLXEXT_PROTOTYPES)

find_package(wxWidgets COMPONENTS core base net aui gl xml REQUIRED)
find_package(GLEW REQUIRED)
find_package(OpenGL REQUIRED)
find_package(ZLIB REQUIRED)
find_package(OpenAL REQUIRED)

include("${wxWidgets_USE_FILE}")

if(NOT CMAKE_SIZEOF_VOID_P EQUAL 8)
	message( FATAL_ERROR "RPCS3 can only be compiled on 64-bit platforms." )
endif()


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
"${RPCS3_SRC_DIR}/../ffmpeg/${PLATFORM_ARCH}/include"
"${RPCS3_SRC_DIR}"
"${RPCS3_SRC_DIR}/Emu"
"${RPCS3_SRC_DIR}/Gui"
"${RPCS3_SRC_DIR}/Loader"
"${RPCS3_SRC_DIR}/Crypto"
"${RPCS3_SRC_DIR}/.."
"${RPCS3_SRC_DIR}/../asmjit/src/asmjit"
)

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

add_executable(rpcs3 ${RPCS3_SRC})

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -L${CMAKE_CURRENT_BINARY_DIR}/../asmjit/") #hack because the asmjit cmake file force fno exceptions
target_link_libraries(rpcs3  asmjit.a  ${wxWidgets_LIBRARIES} ${OPENAL_LIBRARY} ${GLEW_LIBRARY} ${OPENGL_LIBRARIES} libavformat.a libavcodec.a libavutil.a libswresample.a libswscale.a ${ZLIB_LIBRARIES})

set_target_properties(rpcs3 PROPERTIES COTIRE_CXX_PREFIX_HEADER_INIT "${RPCS3_SRC_DIR}/stdafx.h")
cotire(rpcs3)

