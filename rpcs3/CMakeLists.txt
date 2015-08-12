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
	add_definitions(-Wno-attributes -Wno-enum-compare -Wno-invalid-offsetof)
elseif("${CMAKE_CXX_COMPILER_ID}" STREQUAL "Clang")
	# TODO: stdlib?
endif()

if (NOT MSVC)
	add_definitions(-DwxGUI)
	set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++14 -fexceptions")
	set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -g -D_DEBUG")
	set(CMAKE_CXX_FLAGS_MINSIZEREL "${CMAKE_CXX_FLAGS_MINSIZEREL} -Os -D_NDEBUG")
	set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -O1 -D_NDEBUG")
	set(CMAKE_CXX_FLAGS_RELWITHDEBINFO "${CMAKE_CXX_FLAGS_RELWITHDEBINFO} -O1 -g -D_NDEBUG")
	set(CMAKE_C_FLAGS_DEBUG "${CMAKE_C_FLAGS_DEBUG} -g -D_DEBUG")
	set(CMAKE_C_FLAGS_MINSIZEREL "${CMAKE_C_FLAGS_MINSIZEREL} -Os -D_NDEBUG")
	set(CMAKE_C_FLAGS_RELEASE "${CMAKE_C_FLAGS_RELEASE} -O1 -D_NDEBUG")
	set(CMAKE_C_FLAGS_RELWITHDEBINFO "${CMAKE_C_FLAGS_RELWITHDEBINFO} -O1 -g -D_NDEBUG")
	add_definitions(-msse -msse2 -mcx16 -mssse3)
else()
	set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /EHa /Zc:throwingNew /D _CRT_SECURE_NO_DEPRECATE=1 /D _CRT_NON_CONFORMING_SWPRINTFS=1 /D _SCL_SECURE_NO_WARNINGS=1")
endif()

if (APPLE)
	set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -I/opt/X11/include")
	set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -I/opt/X11/include")
endif()

if(${CMAKE_SYSTEM_NAME} MATCHES "Linux")
	#on some Linux distros shm_unlink and similar functions are in librt only
	set(ADDITIONAL_LIBS "rt" "X11")
elseif(UNIX)
	#it seems like glibc includes the iconv functions we use but other libc
	#implementations like the one on OSX don't seem implement them
	set(ADDITIONAL_LIBS "iconv")
else()
	set(ADDITIONAL_LIBS "")
endif()

If(NOT RPCS3_SRC_DIR)
  SET(RPCS3_SRC_DIR ${CMAKE_CURRENT_LIST_DIR})
  Message("-- Initializing RPCS3_SRC_DIR=${RPCS3_SRC_DIR}")
Else()
  Message("-- Using Custom RPCS3_SRC_DIR=${RPCS3_SRC_DIR}")
EndIf()

set(CMAKE_MODULE_PATH "${RPCS3_SRC_DIR}/cmake_modules")

if(NOT WIN32)
    add_definitions(-DGL_GLEXT_PROTOTYPES)
    add_definitions(-DGLX_GLXEXT_PROTOTYPES)
endif()

find_package(wxWidgets COMPONENTS core base net aui gl xml REQUIRED)
if(NOT WIN32)
    find_package(GLEW REQUIRED)
endif()
find_package(OpenGL REQUIRED)
find_package(OpenAL REQUIRED)
find_package(LLVM CONFIG)

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
if(WIN32)
	include_directories(BEFORE "${RPCS3_SRC_DIR}/../minidx9/Include")
endif()

if(LLVM_FOUND)
	add_definitions(${LLVM_DEFINITIONS})
	add_definitions(-DLLVM_AVAILABLE)
	if (CMAKE_BUILD_TYPE STREQUAL "Release")
		llvm_map_components_to_libnames(LLVM_LIBS mcjit vectorize x86codegen x86disassembler)
	else()
		llvm_map_components_to_libnames(LLVM_LIBS mcjit vectorize x86codegen x86disassembler mcdisassembler)
	endif()
	if(MSVC)
		set_source_files_properties(${RPCS3_SRC_DIR}/Emu/Cell/PPULLVMRecompiler.cpp PROPERTIES COMPILE_FLAGS /GR-)
	else()
		set_source_files_properties(${RPCS3_SRC_DIR}/Emu/Cell/PPULLVMRecompiler.cpp PROPERTIES COMPILE_FLAGS -fno-rtti)
	endif()
endif()

link_directories("${RPCS3_SRC_DIR}/../ffmpeg/${PLATFORM_ARCH}/lib" "${RPCS3_SRC_DIR}/../asmjit/")

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

add_executable(rpcs3 ${RPCS3_SRC})

if(NOT MSVC)
	set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -L${CMAKE_CURRENT_BINARY_DIR}/../asmjit/") #hack because the asmjit cmake file force fno exceptions upd: not sure if vs2015 build is affected
else()
	set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} /SUBSYSTEM:WINDOWS /NODEFAULTLIB:libc.lib /NODEFAULTLIB:libcmt.lib /NODEFAULTLIB:libcd.lib /NODEFAULTLIB:libcmtd.lib /NODEFAULTLIB:msvcrtd.lib")
endif()
if(WIN32) # I'm not sure we need all of these libs, but we link them in vs
	target_link_libraries(rpcs3  odbc32.lib odbccp32.lib comctl32.lib ws2_32.lib shlwapi.lib winmm.lib kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib)
	if(LLVM_FOUND)
		target_link_libraries(rpcs3  asmjit.lib ${wxWidgets_LIBRARIES} ${OPENAL_LIBRARY} avformat.lib avcodec.lib avutil.lib swresample.lib swscale.lib ${LLVM_LIBS} ${ADDITIONAL_LIBS})
	else()
		target_link_libraries(rpcs3  asmjit.lib ${wxWidgets_LIBRARIES} ${OPENAL_LIBRARY} avformat.lib avcodec.lib avutil.lib swresample.lib swscale.lib ${ADDITIONAL_LIBS})
	endif()
else()
	if(LLVM_FOUND)
		target_link_libraries(rpcs3  asmjit.a  ${wxWidgets_LIBRARIES} ${OPENAL_LIBRARY} ${GLEW_LIBRARY} ${OPENGL_LIBRARIES} libavformat.a libavcodec.a libavutil.a libswresample.a libswscale.a ${LLVM_LIBS} ${ADDITIONAL_LIBS})
	else()
		target_link_libraries(rpcs3  asmjit.a  ${wxWidgets_LIBRARIES} ${OPENAL_LIBRARY} ${GLEW_LIBRARY} ${OPENGL_LIBRARIES} libavformat.a libavcodec.a libavutil.a libswresample.a libswscale.a ${ADDITIONAL_LIBS})
	endif()
endif()

set_target_properties(rpcs3 PROPERTIES COTIRE_CXX_PREFIX_HEADER_INIT "${RPCS3_SRC_DIR}/stdafx.h")
cotire(rpcs3)

