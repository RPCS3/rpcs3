cmake_minimum_required(VERSION 2.8.12)

set(CMAKE_MODULE_PATH "${CMAKE_CURRENT_LIST_DIR}/cmake_modules")
set(RES_FILES "")
include(cotire)

project(rpcs3)

# Generate git-version.h at build time.
add_custom_target(GitVersion ALL
	DEPENDS something_that_never_exists)
add_custom_command(OUTPUT something_that_never_exists
	COMMAND ${CMAKE_COMMAND} -DSOURCE_DIR=${CMAKE_CURRENT_SOURCE_DIR}
		-P ${CMAKE_CURRENT_SOURCE_DIR}/git-version.cmake)

# Check for a sufficient compiler and set build options
include(ConfigureCompiler)


if(WIN32)
	add_definitions(-DUNICODE)
	add_definitions(-D_WIN32_WINNT=0x0601)
	set(RES_FILES "rpcs3_.rc")
	if(NOT MSVC)
		set(CMAKE_RC_COMPILER_INIT windres)
		enable_language(RC)
		set(CMAKE_RC_COMPILE_OBJECT "<CMAKE_RC_COMPILER> <FLAGS> -O coff <DEFINES> -i <SOURCE> -o <OBJECT>")
	endif()
endif()

if(NOT MSVC)
	add_definitions(-DwxGUI)
	if($ENV{CI})
		set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -O1") # fix for travis gcc OoM crash. Might be fixed with the move to containers.
	endif()
	set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++14 -fexceptions")
	if(WIN32)
		set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wl,--allow-multiple-definition") # Workaround for mingw64 (MSYS2)
	endif()
	add_compile_options(-msse -msse2 -mcx16 -mssse3)
else()
	set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /Zc:throwingNew /D _CRT_SECURE_NO_DEPRECATE=1 /D _CRT_NON_CONFORMING_SWPRINTFS=1 /D _SCL_SECURE_NO_WARNINGS=1")
endif()

if(APPLE)
	set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -I/opt/X11/include")
	set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -I/opt/X11/include")
endif()

if(${CMAKE_SYSTEM_NAME} MATCHES "Linux")
	#on some Linux distros shm_unlink and similar functions are in librt only
	set(ADDITIONAL_LIBS "rt" "X11")
elseif(UNIX OR NOT MSVC)
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
if(NOT MSVC)
	if(APPLE)
		find_path(GLEW_INCLUDE_DIR GL/glew.h
			/usr/include/GL
			/usr/openwin/share/include
			/usr/openwin/include
			/usr/X11R6/include
			/usr/include/X11
			/opt/graphics/OpenGL/include
			/opt/graphics/OpenGL/contrib/libglew
			/usr/local/include
		)
	endif()
	find_package(GLEW REQUIRED)
	find_package(ZLIB REQUIRED)
endif()
find_package(OpenGL REQUIRED)
find_package(OpenAL REQUIRED)
find_package(LLVM 4.0 CONFIG)

include("${wxWidgets_USE_FILE}")

if(APPLE)
	set(PLATFORM_ARCH "macosx/x86_64")
elseif(WIN32)
	set(PLATFORM_ARCH "Windows/x86_64")
else()
	set(PLATFORM_ARCH "linux/x86_64")
endif()

# Select the version of libpng to use, default is builtin
if(USE_SYSTEM_LIBPNG)
	message("-- RPCS3: using shared libpng")
	find_package(PNG REQUIRED)
	include_directories("${PNG_INCLUDE_DIRS}")
else()
	message("-- RPCS3: using builtin libpng")
	include_directories("${RPCS3_SRC_DIR}/../3rdparty/libpng")
endif()

# Select the version of ffmpeg to use, default is builtin
if(USE_SYSTEM_FFMPEG)
	message("-- RPCS3: using shared ffmpeg")
	find_package(FFMPEG REQUIRED)
	include_directories("${FFMPEG_INCLUDE_DIR}")
else()
	message("-- RPCS3: using builtin ffmpeg")
	include_directories("${RPCS3_SRC_DIR}/../3rdparty/ffmpeg/${PLATFORM_ARCH}/include")
endif()

include_directories(
${GLEW_INCLUDE_DIR}
${wxWidgets_INCLUDE_DIRS}
${ZLIB_INCLUDE_DIR}
${OPENAL_INCLUDE_DIR}
${LLVM_INCLUDE_DIRS}
"${RPCS3_SRC_DIR}/../3rdparty/pugixml/src"
"${RPCS3_SRC_DIR}"
"${RPCS3_SRC_DIR}/Loader"
"${RPCS3_SRC_DIR}/Crypto"
"${RPCS3_SRC_DIR}/.."
"${RPCS3_SRC_DIR}/../Utilities/yaml-cpp/include"
"${RPCS3_SRC_DIR}/../asmjit/src/asmjit"
"${RPCS3_SRC_DIR}/../3rdparty/GSL/include"
"${RPCS3_SRC_DIR}/../rsx_program_decompiler/rsx_decompiler"
"${RPCS3_SRC_DIR}/../rsx_program_decompiler/shader_code"
"${RPCS3_SRC_DIR}/../Vulkan/Vulkan-LoaderAndValidationLayers/include"
"${RPCS3_SRC_DIR}/../Vulkan/glslang/glslang/Public"
# Includes 3rdparty stuff that isn't included yet
"${RPCS3_SRC_DIR}/../3rdparty/GL"
"${RPCS3_SRC_DIR}/../3rdparty/stblib"
"${RPCS3_SRC_DIR}/../rsx_program_decompiler/rsx_decompiler"
"${RPCS3_SRC_DIR}/../rsx_program_decompiler/shader_code"
"${RPCS3_SRC_DIR}/../3rdparty/cereal/include"
)
if(WIN32)
	include_directories(BEFORE "${RPCS3_SRC_DIR}/../3rdparty/XAudio2_7") # Slimmed down version of minidx9 for XAudio2_7 only
	include_directories(BEFORE "${RPCS3_SRC_DIR}/../3rdparty/minidx12/Include")
endif()

if(NOT LLVM_FOUND)
	Message("LLVM not found! LLVM 4.0 is required. RPCS3 will be compiled without LLVM support.")
else()
	add_definitions(${LLVM_DEFINITIONS})
	add_definitions(-DLLVM_AVAILABLE)
	if (CMAKE_BUILD_TYPE STREQUAL "Release")
		llvm_map_components_to_libnames(LLVM_LIBS mcjit vectorize ipo x86codegen x86disassembler)
	else()
		llvm_map_components_to_libnames(LLVM_LIBS mcjit vectorize ipo x86codegen x86disassembler mcdisassembler)
	endif()
	if (NOT MSVC)
		set_source_files_properties(${RPCS3_SRC_DIR}/../Utilities/JIT.cpp PROPERTIES COMPILE_FLAGS -fno-rtti)
		set_source_files_properties(${RPCS3_SRC_DIR}/Emu/Cell/PPUTranslator.cpp PROPERTIES COMPILE_FLAGS -fno-rtti)
	endif()
endif()

link_directories(
"${RPCS3_SRC_DIR}/../3rdparty/minidx12/"
"${RPCS3_SRC_DIR}/../Vulkan"
)

if(NOT USE_SYSTEM_FFMPEG)
	if(MSVC OR NOT WIN32)
		link_directories("${RPCS3_SRC_DIR}/../3rdparty/ffmpeg/${PLATFORM_ARCH}/lib")
	endif()
endif()

get_property(dirs DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR} PROPERTY INCLUDE_DIRECTORIES)
foreach(dir ${dirs})
  message(STATUS "dir='${dir}'")
endforeach()

file(
GLOB_RECURSE
RPCS3_SRC
"${RPCS3_SRC_DIR}/*.cpp"
"${RPCS3_SRC_DIR}/../Utilities/*.cpp"
"${RPCS3_SRC_DIR}/../rsx_program_decompiler/rsx_decompiler/*.cpp"
"${RPCS3_SRC_DIR}/../rsx_program_decompiler/shader_code/*.cpp"
"${RPCS3_SRC_DIR}/../asmjit/src/asmjit/*.cpp"
)

if(NOT WIN32)
	set (EXCLUDE_DIR "/RSX/VK/")
	foreach (TMP_PATH ${RPCS3_SRC})
		string (FIND ${TMP_PATH} ${EXCLUDE_DIR} EXCLUDE_DIR_FOUND)
		if (NOT ${EXCLUDE_DIR_FOUND} EQUAL -1)
			list (REMOVE_ITEM RPCS3_SRC ${TMP_PATH})
		endif ()
	endforeach(TMP_PATH)
endif()

add_executable(rpcs3 ${RPCS3_SRC} ${RES_FILES})


if(MSVC)
	set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} /NODEFAULTLIB:libc.lib /NODEFAULTLIB:libcmt.lib /NODEFAULTLIB:libcd.lib /NODEFAULTLIB:libcmtd.lib /NODEFAULTLIB:msvcrtd.lib")
	set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} /SUBSYSTEM:WINDOWS /DYNAMICBASE:NO /BASE:0x10000 /FIXED")
endif()
if(WIN32)
	target_link_libraries(rpcs3  ws2_32.lib Winmm.lib Psapi.lib VKstatic.1 glslang OSDependent OGLCompiler SPIRV HLSL)
	if(NOT MSVC)
		target_link_libraries(rpcs3  ${OPENGL_LIBRARIES} ${GLEW_LIBRARY} opengl32.lib glu32.lib libpthread)
	else()
		target_link_libraries(rpcs3  dxgi.lib d2d1.lib dwrite.lib)
	endif()
	target_link_libraries(rpcs3  avformat.lib avcodec.lib avutil.lib swresample.lib swscale.lib png16_static ${wxWidgets_LIBRARIES} ${OPENAL_LIBRARY} ${ADDITIONAL_LIBS})
else()
	target_link_libraries(rpcs3  ${wxWidgets_LIBRARIES} ${OPENAL_LIBRARY} ${GLEW_LIBRARY} ${OPENGL_LIBRARIES})
	target_link_libraries(rpcs3  -ldl ${ZLIB_LIBRARIES} ${ADDITIONAL_LIBS})
	if (USE_SYSTEM_FFMPEG)
		link_libraries(${FFMPEG_LIBRARY_DIR})
		target_link_libraries(rpcs3  libavformat.so libavcodec.so libavutil.so libswresample.so libswscale.so)
	else()
		target_link_libraries(rpcs3  libavformat.a libavcodec.a libavutil.a libswresample.a libswscale.a -ldl)
	endif()
	if (USE_SYSTEM_LIBPNG)
		target_link_libraries(rpcs3  ${PNG_LIBRARIES})
	else()
		target_link_libraries(rpcs3  png16_static)
	endif()
endif()
if(LLVM_FOUND)
	target_link_libraries(rpcs3  ${LLVM_LIBS})
endif()

target_link_libraries(rpcs3 rsx_decompiler shader_code)

set_target_properties(rpcs3 PROPERTIES COTIRE_CXX_PREFIX_HEADER_INIT "${RPCS3_SRC_DIR}/stdafx.h")
cotire(rpcs3)
