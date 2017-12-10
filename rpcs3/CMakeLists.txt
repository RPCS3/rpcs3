cmake_minimum_required(VERSION 3.1)

set(CMAKE_MODULE_PATH "${CMAKE_CURRENT_LIST_DIR}/cmake_modules")
set(RES_FILES "")
set(CMAKE_CXX_STANDARD 14)
include(CheckCXXCompilerFlag)

# Qt section
find_package(Qt5 5.7 COMPONENTS Widgets Network)
if(WIN32)
	find_package(Qt5 5.7 COMPONENTS WinExtras REQUIRED)
	set(RPCS3_QT_LIBS Qt5::Widgets Qt5::WinExtras Qt5::Network)
else()
	find_package(Qt5 5.7 COMPONENTS DBus)
	if(Qt5DBus_FOUND)
		set(RPCS3_QT_LIBS Qt5::Widgets Qt5::DBus Qt5::Network)
		add_definitions(-DHAVE_QTDBUS)
	else()
		set(RPCS3_QT_LIBS Qt5::Widgets Qt5::Network)
	endif()
endif()

# Let's make sure we have Qt before we continue
if(NOT Qt5Widgets_FOUND)
	if(Qt5Widgets_VERSION VERSION_LESS 5.7.0)
		message("Minimum supported Qt5 version is 5.7! You have version ${Qt5Widgets_VERSION} installed, please upgrade!")
		if("${CMAKE_SYSTEM}" MATCHES "Linux")
			message(FATAL_ERROR "Most distros do not provide an up-to-date version of Qt.
If you're on Ubuntu or Linux Mint, there are PPAs you can use to install an up-to-date qt5 version.
        https://launchpad.net/~beineri/+archive/ubuntu/opt-qt593-xenial
        https://launchpad.net/~beineri/+archive/ubuntu/opt-qt593-trusty
just make sure to run
    source /opt/qt59/bin/qt59-env.sh
before re-running cmake")
		elseif(WIN32)
			message(FATAL_ERROR "You can download the latest version of Qt5 here: https://www.qt.io/download-open-source/")
		else()
			message(FATAL_ERROR "Look online for instructions on installing an up-to-date Qt5 on ${CMAKE_SYSTEM}.")
		endif()
	endif()

	message("CMake was unable to find Qt5!")
	if(WIN32)
		message(FATAL_ERROR "Make sure the QTDIR env variable has been set properly. (for example C:\\Qt\\5.8\\msvc2017_64\\)")
	elseif("${CMAKE_SYSTEM}" MATCHES "Linux")
		message(FATAL_ERROR "Make sure to install your distro's qt5 package!")
	else()
		message(FATAL_ERROR "You need to have Qt5 installed, look online for instructions on installing Qt5 on ${CMAKE_SYSTEM}.")
	endif()
endif()

set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTOUIC ON)

# To make UI files on cmake 3.7 or less work
if(CMAKE_VERION VERSION_LESS 3.8)
	set(CMAKE_INCLUDE_CURRENT_DIR ON)
endif()

if(Qt5_POSITION_INDEPENDENT_CODE)
	set(CMAKE_POSITION_INDEPENDENT_CODE ON)
	if(UNIX)
		# Cotire needs this set for some reason
		set(CMAKE_CXX_COMPILE_OPTIONS_PIE -fPIC)
	endif()
endif()

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
	set(RES_FILES "rpcs3.rc")
else()
	add_definitions(-DGL_GLEXT_PROTOTYPES)
	add_definitions(-DGLX_GLXEXT_PROTOTYPES)
endif()

if(NOT MSVC)
	set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fexceptions")
	# This hides our LLVM from mesa's LLVM, otherwise we get some unresolvable conflicts.
	if(NOT APPLE)
		set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wl,--exclude-libs,ALL")
	endif()

	if(WIN32)
		set(CMAKE_RC_COMPILER_INIT windres)
		enable_language(RC)
		set(CMAKE_RC_COMPILE_OBJECT "<CMAKE_RC_COMPILER> <FLAGS> -O coff <DEFINES> -i <SOURCE> -o <OBJECT>")
		
		# Workaround for mingw64 (MSYS2)
		set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wl,--allow-multiple-definition")
	endif()
	
	add_compile_options(-msse -msse2 -mcx16 -mssse3 -mrtm)
	
	if("${CMAKE_CXX_COMPILER_ID}" STREQUAL "Clang")
		# This fixes 'some' of the st11range issues. See issue #2516
		if(APPLE)
			set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -Wl,-image_base,0x10000 -Wl,-pagezero_size,0x10000")
		else()
			set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -image-base=0x10000")
		endif()
	endif()
	
	# Some distros have the compilers set to use PIE by default, but RPCS3 doesn't work with PIE, so we need to disable it.
	if(APPLE)
		set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -Wl,-no_pie")
	else()
		CHECK_CXX_COMPILER_FLAG("-no-pie" HAS_NO_PIE)
		CHECK_CXX_COMPILER_FLAG("-nopie" HAS_NOPIE)

		if(HAS_NO_PIE)
			set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -no-pie")
		elseif(HAS_NOPIE)
			set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -nopie")
		endif()
	endif()

	find_package(ZLIB REQUIRED)
else()
	set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /Zc:throwingNew /D _CRT_SECURE_NO_DEPRECATE=1 /D _CRT_NON_CONFORMING_SWPRINTFS=1 /D _SCL_SECURE_NO_WARNINGS=1")
	
	set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} /NODEFAULTLIB:libc.lib /NODEFAULTLIB:libcmt.lib /NODEFAULTLIB:libcd.lib /NODEFAULTLIB:libcmtd.lib /NODEFAULTLIB:msvcrtd.lib")
	set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} /SUBSYSTEM:WINDOWS /DYNAMICBASE:NO /BASE:0x10000 /FIXED")
endif()

set(ADDITIONAL_LIBS "")
if(${CMAKE_SYSTEM_NAME} MATCHES "Linux")
	#on some Linux distros shm_unlink and similar functions are in librt only
	set(ADDITIONAL_LIBS ${ADDITIONAL_LIBS} "rt")
elseif(NOT MSVC AND NOT CMAKE_CXX_FLAGS MATCHES "LIBICONV_PLUG")
	#it seems like glibc includes the iconv functions we use but other libc
	#implementations like the one on OSX don't seem implement them
	set(ADDITIONAL_LIBS ${ADDITIONAL_LIBS} "iconv")
endif()
if(UNIX AND NOT APPLE)
	set(ADDITIONAL_LIBS ${ADDITIONAL_LIBS} "X11")
endif()

if(NOT RPCS3_SRC_DIR)
	set(RPCS3_SRC_DIR ${CMAKE_CURRENT_LIST_DIR})
	message("-- Initializing RPCS3_SRC_DIR=${RPCS3_SRC_DIR}")
else()
	message("-- Using Custom RPCS3_SRC_DIR=${RPCS3_SRC_DIR}")
endif()

set(CMAKE_MODULE_PATH "${RPCS3_SRC_DIR}/cmake_modules")

find_package(OpenGL REQUIRED)
find_package(OpenAL REQUIRED)
if(NOT WITHOUT_LLVM)
	find_package(LLVM 4.0 CONFIG)
	if(NOT LLVM_FOUND)
		message("System LLVM was not found, LLVM will be built from the submodule.")

		set(LLVM_TARGETS_TO_BUILD "X86" CACHE INTERNAL "")
		option(LLVM_BUILD_RUNTIME OFF)
		option(LLVM_BUILD_TOOLS OFF)
		option(LLVM_INCLUDE_DOCS OFF)
		option(LLVM_INCLUDE_EXAMPLES OFF)
		option(LLVM_INCLUDE_TESTS OFF)
		option(LLVM_INCLUDE_TOOLS OFF)
		option(LLVM_INCLUDE_UTILS OFF)
		option(WITH_POLLY OFF)

		# LLVM needs to be built out-of-tree
		add_subdirectory(../llvm ../llvm_build)
		set(LLVM_DIR "${CMAKE_CURRENT_BINARY_DIR}/../llvm_build/lib/cmake/llvm/")

		# now tries to find LLVM again
		find_package(LLVM 4.0 CONFIG)
		if(NOT LLVM_FOUND)
			message(WARNING "Couldn't build LLVM from the submodule. You might need to run `git submodule update --init`")
		endif()
	endif()
endif()


if(APPLE)
	set(PLATFORM_ARCH "macosx/x86_64")
elseif(WIN32)
	set(PLATFORM_ARCH "Windows/x86_64")
else()
	set(PLATFORM_ARCH "linux/x86_64")
	option(USE_ALSA "ALSA audio backend" ON)
	option(USE_PULSE "PulseAudio audio backend" ON)
	option(USE_LIBEVDEV "libevdev-based joystick support" ON)
	option(USE_VULKAN "Vulkan render backend" ON)
endif()

if(USE_ALSA)
	find_package(ALSA)
	if(ALSA_FOUND)
		add_definitions(-DHAVE_ALSA)
		include_directories(SYSTEM ${ALSA_INCLUDE_DIRS})
		list(APPEND ADDITIONAL_LIBS ${ALSA_LIBRARIES})
	endif()
endif()
if(USE_PULSE)
	find_package(PkgConfig)
	pkg_check_modules(PULSE libpulse-simple)
	if(PULSE_FOUND)
		add_definitions(-DHAVE_PULSE)
		include_directories(SYSTEM ${PULSE_INCLUDE_DIRS})
		list(APPEND ADDITIONAL_LIBS ${PULSE_LDFLAGS})
	endif()
endif()
if(USE_LIBEVDEV)
	find_package(PkgConfig)
	pkg_check_modules(LIBEVDEV libevdev)
	if(LIBEVDEV_FOUND)
		add_definitions(-DHAVE_LIBEVDEV)
		include_directories(SYSTEM ${LIBEVDEV_INCLUDE_DIRS})
		list(APPEND ADDITIONAL_LIBS ${LIBEVDEV_LDFLAGS})
	endif()
endif()
if(NOT WIN32 AND USE_VULKAN)
	find_package(Vulkan)
	if(VULKAN_FOUND)
		add_definitions(-DHAVE_VULKAN)
		list(APPEND ADDITIONAL_LIBS ${VULKAN_LIBRARY})
		if(VULKAN_PREBUILT)
			list(APPEND ADDITIONAL_LIBS glslang HLSL OGLCompiler SPIRV OSDependent)
		endif()
	else()
		message("WARNING! USE_VULKAN was enabled, but libvulkan was not found. RPCS3 will be compiled without Vulkan support.")
	endif()
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
"${RPCS3_SRC_DIR}/../Vulkan/Vulkan-LoaderAndValidationLayers/include"
"${RPCS3_SRC_DIR}/../Vulkan/glslang/glslang/Public"
"${RPCS3_SRC_DIR}/../Vulkan/glslang/"
"${RPCS3_SRC_DIR}/../3rdparty/hidapi/hidapi"
# Includes 3rdparty stuff that isn't included yet
"${RPCS3_SRC_DIR}/../3rdparty/GL"
"${RPCS3_SRC_DIR}/../3rdparty/stblib"
"${RPCS3_SRC_DIR}/../3rdparty/cereal/include"
"${RPCS3_SRC_DIR}/../3rdparty/Optional"
)

if(WIN32)
	# Slimmed down version of minidx9 for XAudio2_7 only
	include_directories(BEFORE "${RPCS3_SRC_DIR}/../3rdparty/XAudio2_7")
	include_directories(BEFORE "${RPCS3_SRC_DIR}/../3rdparty/minidx12/Include")
endif()

if(NOT LLVM_FOUND)
	message("LLVM 4.0 not found. RPCS3 will be compiled without LLVM support.")
else()
	add_definitions(${LLVM_DEFINITIONS})
	add_definitions(-DLLVM_AVAILABLE)
	if(CMAKE_BUILD_TYPE STREQUAL "Release")
		llvm_map_components_to_libnames(LLVM_LIBS core support executionengine object runtimedyld x86desc x86info scalaropts mcjit vectorize ipo x86codegen x86disassembler)
	else()
		llvm_map_components_to_libnames(LLVM_LIBS core support executionengine object runtimedyld x86desc x86info scalaropts mcjit vectorize ipo x86codegen x86disassembler mcdisassembler)
	endif()

	if(NOT MSVC)
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
foreach (dir ${dirs})
	message(STATUS "dir='${dir}'")
endforeach()

file(
GLOB_RECURSE
RPCS3_SRC
"${RPCS3_SRC_DIR}/*.cpp"
"${RPCS3_SRC_DIR}/../Utilities/*.cpp"
"${RPCS3_SRC_DIR}/../asmjit/src/asmjit/*.cpp"
)

if(NOT WIN32 AND VULKAN_FOUND)
# Compile glslang and SPIRV modules needed for glsl compilation
	file(
	GLOB_RECURSE
	UNIX_GLSLANG
	"${RPCS3_SRC_DIR}/../Vulkan/glslang/glslang/GenericCodeGen/*.cpp"
	"${RPCS3_SRC_DIR}/../Vulkan/glslang/glslang/MachineIndependent/*.cpp"
	"${RPCS3_SRC_DIR}/../Vulkan/glslang/glslang/OSDependent/Unix/*.cpp"
	"${RPCS3_SRC_DIR}/../Vulkan/glslang/OGLCompilersDLL/*.cpp"
	"${RPCS3_SRC_DIR}/../Vulkan/glslang/SPIRV/*.cpp"
	)
	
	set(RPCS3_SRC ${RPCS3_SRC} ${UNIX_GLSLANG})
endif()

#File exclusion section

if(NOT WIN32 AND NOT VULKAN_FOUND)
	set(EXCLUDE_FILES "/RSX/VK/")
endif()

# Ignore autogenerated moc_* files if present
set(EXCLUDE_FILES ${EXCLUDE_FILES} "moc_")
set(EXCLUDE_FILES ${EXCLUDE_FILES} "rpcs3_automoc")
set(EXCLUDE_FILES ${EXCLUDE_FILES} "qrc_resources.cpp")

foreach (TMP_PATH ${RPCS3_SRC})
	foreach (EXCLUDE_PATH ${EXCLUDE_FILES})
		string(FIND ${TMP_PATH} ${EXCLUDE_PATH} EXCLUDE_FILE_FOUND)
		if(NOT ${EXCLUDE_FILE_FOUND} EQUAL -1)
			list(REMOVE_ITEM RPCS3_SRC ${TMP_PATH})
		endif ()
	endforeach(EXCLUDE_PATH)
endforeach(TMP_PATH)

# Remove the Qt moc files as part of clean, they are compiled when generating automoc
file(GLOB_RECURSE TMP_MOC "${RPCS3_SRC_DIR}/moc_*.cpp" "${RPCS3_SRC_DIR}/rpcs3_automoc.cpp" "${RPCS3_SRC_DIR}/qrc_resources.cpp")
set_directory_properties(PROPERTIES ADDITIONAL_MAKE_CLEAN_FILES "${TMP_MOC}")

if(WIN32)
	add_executable(rpcs3 WIN32 ${RPCS3_SRC} ${RES_FILES} resources.qrc)
else()
	add_executable(rpcs3 ${RPCS3_SRC} ${RES_FILES} resources.qrc)
endif()

add_dependencies(rpcs3 GitVersion)

if(NOT MSVC)
	find_package(GLEW 1.13.0 REQUIRED)
	target_link_libraries(rpcs3 GLEW::GLEW)
endif()

if(WIN32)
	target_link_libraries(rpcs3 ws2_32.lib Winmm.lib Psapi.lib VKstatic.1 glslang OSDependent OGLCompiler SPIRV HLSL setupapi.lib hidapi-hid Shlwapi.lib)
	if(NOT MSVC)
		target_link_libraries(rpcs3 ${OPENGL_LIBRARIES} opengl32.lib glu32.lib libpthread)
	else()
		target_link_libraries(rpcs3 dxgi.lib d2d1.lib dwrite.lib)
	endif()
	target_link_libraries(rpcs3 avformat.lib avcodec.lib avutil.lib swscale.lib png16_static ${OPENAL_LIBRARY} ${ADDITIONAL_LIBS})
else()
	target_link_libraries(rpcs3 ${OPENAL_LIBRARY} ${OPENGL_LIBRARIES})
	if(APPLE)
		target_link_libraries(rpcs3 hidapi-mac "-framework CoreFoundation" "-framework IOKit")
	elseif(CMAKE_SYSTEM_NAME STREQUAL "Linux")
		target_link_libraries(rpcs3 hidapi-hidraw udev)
	else()
		target_link_libraries(rpcs3 hidapi-libusb usb)
	endif()
	if(VULKAN_FOUND)
		target_link_libraries(rpcs3 ${VULKAN_LIBRARIES})
	endif()
	target_link_libraries(rpcs3 ${CMAKE_DL_LIBS} -lpthread ZLIB::ZLIB ${ADDITIONAL_LIBS})
	if(USE_SYSTEM_FFMPEG)
		link_libraries(${FFMPEG_LIBRARY_DIR})
		target_link_libraries(rpcs3 libavformat.so libavcodec.so libavutil.so libswscale.so)
	else()
		target_link_libraries(rpcs3 libavformat.a libavcodec.a libavutil.a libswscale.a)
	endif()
	if(USE_SYSTEM_LIBPNG)
		target_link_libraries(rpcs3 ${PNG_LIBRARIES})
	else()
		target_link_libraries(rpcs3 png16_static)
	endif()
endif()

# For some reason GCC 7 requires manually linking with -latomic
if(CMAKE_COMPILER_IS_GNUCXX AND (CMAKE_CXX_COMPILER_VERSION VERSION_GREATER 7 OR CMAKE_CXX_COMPILER_VERSION VERSION_EQUAL 7))
	target_link_libraries(rpcs3 -latomic)
endif()

if(LLVM_FOUND)
	target_link_libraries(rpcs3 ${LLVM_LIBS})
endif()

target_link_libraries(rpcs3 ${RPCS3_QT_LIBS})

set_target_properties(rpcs3 PROPERTIES COTIRE_CXX_PREFIX_HEADER_INIT "${RPCS3_SRC_DIR}/stdafx.h")

if(MSVC)
	# Under Windows, some QT DLLs need to be in the same directory of the compiled
	# RPCS3 binary, so call the windeployqt tool that will take care of copying
	# them from the local QT installation at the end of the build.
	if(CMAKE_BUILD_TYPE STREQUAL "Debug")
		add_custom_command(TARGET rpcs3 POST_BUILD
			COMMAND $ENV{QTDIR}/bin/windeployqt --no-angle --no-opengl-sw --no-svg --no-translations --debug ${CMAKE_BINARY_DIR}/bin
		)
	else()
		add_custom_command(TARGET rpcs3 POST_BUILD
			COMMAND $ENV{QTDIR}/bin/windeployqt --no-angle --no-opengl-sw --no-svg --no-translations ${CMAKE_BINARY_DIR}/bin
		)
	endif()
endif()

cotire(rpcs3)

# Unix installation
if(UNIX AND NOT APPLE)
	# Install the binary
	install(TARGETS rpcs3 RUNTIME DESTINATION ${CMAKE_INSTALL_PREFIX}/bin)
	# Install the application icon and menu item
	install(FILES rpcs3.svg
		DESTINATION ${CMAKE_INSTALL_PREFIX}/share/icons/hicolor/scalable/apps)
	install(FILES rpcs3.png
		DESTINATION ${CMAKE_INSTALL_PREFIX}/share/icons/hicolor/48x48/apps)
	install(FILES rpcs3.desktop
		DESTINATION ${CMAKE_INSTALL_PREFIX}/share/applications)
endif()
