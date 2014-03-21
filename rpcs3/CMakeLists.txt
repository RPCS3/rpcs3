cmake_minimum_required(VERSION 2.8)
project(rpcs3)

if (CMAKE_COMPILER_IS_GNUCXX)
	add_definitions(-std=gnu++11)
	#add_definitions(-D__WXGTK__)
	#add_definitions(-Wfatal-errors)
	add_definitions(-w) # TODO: remove me
	add_definitions(-fpermissive) # TODO: remove me
endif()

set(CMAKE_MODULE_PATH ${CMAKE_CURRENT_SOURCE_DIR}/cmake_modules)
set(EXECUTABLE_OUTPUT_PATH "${CMAKE_SOURCE_DIR}/../bin")

add_definitions(-DGL_GLEXT_PROTOTYPES)
add_definitions(-DGLX_GLXEXT_PROTOTYPES)

find_package(wxWidgets COMPONENTS core base net aui gl xml REQUIRED)
find_package(GLEW REQUIRED)
find_package(OpenGL REQUIRED)
find_package(ZLIB REQUIRED)
find_package(OpenAL REQUIRED)

include("${wxWidgets_USE_FILE}")

if(CMAKE_SIZEOF_VOID_P EQUAL 8)
	set(PLATFORM_ARCH "linux/x86_64")
else()
	set(PLATFORM_ARCH "linux/x86")
endif()

include_directories(
${wxWidgets_INCLUDE_DIRS}
${OPENAL_INCLUDE_DIR}
${CMAKE_SOURCE_DIR}/../ffmpeg/${PLATFORM_ARCH}/include
${CMAKE_SOURCE_DIR}
${CMAKE_SOURCE_DIR}/Emu
${CMAKE_SOURCE_DIR}/Gui
${CMAKE_SOURCE_DIR}/Loader
${CMAKE_SOURCE_DIR}/Crypto
${CMAKE_SOURCE_DIR}/..
)

link_directories(${CMAKE_SOURCE_DIR}/../ffmpeg/${PLATFORM_ARCH}/lib)

file(
GLOB_RECURSE
RPCS3_SRC
${CMAKE_SOURCE_DIR}/rpcs3.cpp
${CMAKE_SOURCE_DIR}/AppConnector.cpp
${CMAKE_SOURCE_DIR}/Ini.cpp
${CMAKE_SOURCE_DIR}/Emu/*
${CMAKE_SOURCE_DIR}/Gui/*
${CMAKE_SOURCE_DIR}/Loader/*
${CMAKE_SOURCE_DIR}/Crypto/*
${CMAKE_SOURCE_DIR}/../Utilities/*
)

add_executable(rpcs3 ${RPCS3_SRC})

target_link_libraries(rpcs3 ${wxWidgets_LIBRARIES} ${OPENAL_LIBRARY} ${GLEW_LIBRARIES} ${OPENGL_LIBRARIES} libavformat.a libavcodec.a libavutil.a libswresample.a libswscale.a ${ZLIB_LIBRARIES})

