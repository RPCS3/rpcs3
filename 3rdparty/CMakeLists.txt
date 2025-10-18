find_package(PkgConfig)
include(ExternalProject)
include(CMakeDependentOption)

set(CMAKE_CXX_STANDARD 20)

# Defines the ARCHITECTURE variable
include("DetectArchitecture.cmake")

# Warnings are silenced for 3rdparty code
if(NOT MSVC)
	add_compile_options("$<$<COMPILE_LANGUAGE:CXX,C>:-w>")
endif()

# Dummy target to use when lib isn't available
add_library(3rdparty_dummy_lib INTERFACE)


# ZLib
add_subdirectory(zlib EXCLUDE_FROM_ALL)

# ZSTD
add_subdirectory(zstd EXCLUDE_FROM_ALL)

# 7zip sdk
add_subdirectory(7zip EXCLUDE_FROM_ALL)

add_library(3rdparty_flatbuffers INTERFACE)
if (USE_SYSTEM_FLATBUFFERS)
	pkg_check_modules(FLATBUFFERS REQUIRED IMPORTED_TARGET flatbuffers>=2.0.0)
	target_link_libraries(3rdparty_flatbuffers INTERFACE PkgConfig::FLATBUFFERS)
	set(FBS_DIR "${CMAKE_CURRENT_SOURCE_DIR}/../rpcs3/Emu/NP/generated/")
	execute_process(COMMAND flatc --cpp -o "${FBS_DIR}" "${FBS_DIR}/np2_structs.fbs" RESULT_VARIABLE FBS_CMD_ERROR)
	if(FBS_CMD_ERROR AND NOT FBS_CMD_ERROR EQUAL 0)
		message(FATAL_ERROR "flatc failed to regenerate flatbuffers headers.")
	endif()
else()
	target_include_directories(3rdparty_flatbuffers INTERFACE flatbuffers/include)
endif()

# libPNG
add_subdirectory(libpng EXCLUDE_FROM_ALL)


# pugixml
if (USE_SYSTEM_PUGIXML)
	pkg_check_modules(PUGIXML REQUIRED IMPORTED_TARGET pugixml>=1.15)
	add_library(pugixml INTERFACE)
	target_link_libraries(pugixml INTERFACE PkgConfig::PUGIXML)
else()
	add_subdirectory(pugixml EXCLUDE_FROM_ALL)
endif()

if (USE_SYSTEM_VULKAN_MEMORY_ALLOCATOR)
	find_package(VulkanMemoryAllocator REQUIRED GLOBAL)
	add_library(3rdparty::vulkanmemoryallocator ALIAS GPUOpen::VulkanMemoryAllocator)
else()
	add_library(3rdparty_vulkanmemoryallocator INTERFACE)
	target_include_directories(3rdparty_vulkanmemoryallocator INTERFACE GPUOpen/VulkanMemoryAllocator/include)
	add_library(3rdparty::vulkanmemoryallocator ALIAS 3rdparty_vulkanmemoryallocator)
endif()

# libusb
if(CMAKE_SYSTEM_NAME MATCHES "DragonFly|FreeBSD")
	pkg_check_modules(LIBUSB REQUIRED IMPORTED_TARGET libusb-1.0>=1.0 )
	cmake_dependent_option(USE_SYSTEM_LIBUSB "Use system libusb-1.0 as shared library" ON
			"LIBUSB_FOUND" OFF)
else()
	pkg_check_modules(LIBUSB IMPORTED_TARGET libusb-1.0>=1.0 )
	cmake_dependent_option(USE_SYSTEM_LIBUSB "Use system libusb-1.0 as shared library" OFF
			"LIBUSB_FOUND" OFF)
endif()
if(CMAKE_SYSTEM_NAME MATCHES "DragonFly|FreeBSD")
	# Always use system libusb as reference implementation isn't supported
	add_library(usb-1.0-shared INTERFACE)
	target_link_libraries(usb-1.0-shared INTERFACE PkgConfig::LIBUSB)
elseif(MSVC)
	# Windows time.h defines timespec but doesn't add any flag for it, which makes libusb attempt to define it again
	add_definitions(-DHAVE_STRUCT_TIMESPEC=1)
	add_subdirectory(libusb EXCLUDE_FROM_ALL)
else()
	if(USE_SYSTEM_LIBUSB)
		# we have the system libusb and have selected to use it
		add_library(usb-1.0-shared INTERFACE)
		target_link_libraries(usb-1.0-shared INTERFACE PkgConfig::LIBUSB)
	else()
		# we don't have the system libusb, so we compile from submodule
		unset(LIBUSB_LIBRARIES CACHE)
		add_subdirectory(libusb EXCLUDE_FROM_ALL)

		if (NOT TARGET usb-1.0 AND TARGET usb-1.0-static)
			add_library(usb-1.0 ALIAS usb-1.0-static)
		endif()
	endif()
endif()


# hidapi
add_subdirectory(hidapi)

# glslang
add_subdirectory(glslang EXCLUDE_FROM_ALL)

# yaml-cpp
add_subdirectory(yaml-cpp)


# OpenGL

if (NOT ANDROID)
	find_package(OpenGL REQUIRED OPTIONAL_COMPONENTS EGL)

	add_library(3rdparty_opengl INTERFACE)
	target_include_directories(3rdparty_opengl INTERFACE GL)

	if (WIN32)
		if(NOT MSVC)
			target_link_libraries(3rdparty_opengl INTERFACE OpenGL::GL OpenGL::GLU)
		else()
			target_link_libraries(3rdparty_opengl INTERFACE dxgi.lib d2d1.lib dwrite.lib)
		endif()
	elseif(APPLE)
		target_link_libraries(3rdparty_opengl INTERFACE OpenGL::GL OpenGL::GLU)
	else()
		target_link_libraries(3rdparty_opengl INTERFACE OpenGL::GL OpenGL::GLU OpenGL::GLX)
	endif()
else()
	add_library(3rdparty_opengl INTERFACE)
	target_compile_definitions(3rdparty_opengl INTERFACE WITHOUT_OPENGL=1)
endif()

# stblib
add_subdirectory(stblib)

# DiscordRPC
add_subdirectory(discord-rpc)

# Cubeb
if(USE_SYSTEM_CUBEB)
	find_package(cubeb REQUIRED GLOBAL)
	message(STATUS "Using system cubeb version '${cubeb_VERSION}'")
	add_library(3rdparty::cubeb ALIAS cubeb::cubeb)
else()
	message(STATUS "Using static cubeb from 3rdparty")
	add_subdirectory(cubeb EXCLUDE_FROM_ALL)
endif()

# SoundTouch
add_subdirectory(SoundTouch EXCLUDE_FROM_ALL)

# libevdev
set(LIBEVDEV_TARGET 3rdparty_dummy_lib)
if(USE_LIBEVDEV)
	pkg_check_modules(LIBEVDEV libevdev libudev)
	if(LIBEVDEV_FOUND)
		add_library(3rdparty_libevdev INTERFACE)
		target_compile_definitions(3rdparty_libevdev INTERFACE -DHAVE_LIBEVDEV)
		target_include_directories(3rdparty_libevdev SYSTEM
			INTERFACE ${LIBEVDEV_INCLUDE_DIRS})
		target_link_libraries(3rdparty_libevdev INTERFACE ${LIBEVDEV_LDFLAGS})

		set(LIBEVDEV_TARGET 3rdparty_libevdev)
	endif()
endif()


# Vulkan
set(VULKAN_TARGET 3rdparty_dummy_lib)
if(USE_VULKAN)
	if(APPLE)
		if(USE_SYSTEM_MVK)
			message(STATUS "RPCS3: Using system MoltenVK")
		else()
			message(STATUS "RPCS3: MoltenVK submodule")

			execute_process(COMMAND "${CMAKE_COMMAND}" -G "${CMAKE_GENERATOR}" .
				WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/MoltenVK"
			)
			execute_process(COMMAND "${CMAKE_COMMAND}" --build .
				WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/MoltenVK"
			)

			add_library(moltenvk_lib SHARED IMPORTED)
			add_dependencies(moltenvk_lib moltenvk)
			set_target_properties(moltenvk_lib
				PROPERTIES IMPORTED_LOCATION "{Vulkan_LIBRARY}"
			)

			set(VULKAN_SDK "${CMAKE_CURRENT_SOURCE_DIR}/MoltenVK/MoltenVK/MoltenVK")
			set(VK_ICD_FILENAMES "${CMAKE_CURRENT_SOURCE_DIR}/MoltenVK/MoltenVK/MoltenVK/icd/MoltenVK_icd.json")
			set(Vulkan_INCLUDE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/MoltenVK/MoltenVK/MoltenVK/include")
			set(Vulkan_LIBRARY "${CMAKE_CURRENT_SOURCE_DIR}/MoltenVK/MoltenVK/Build/Products/Release/dynamic/libMoltenVK.dylib")
			set(Vulkan_TOOLS "${CMAKE_CURRENT_SOURCE_DIR}/MoltenVK/MoltenVK/Build/Products/Release")
		endif()
	endif()

	find_package(Vulkan)
	if(VULKAN_FOUND)
		add_library(3rdparty_vulkan INTERFACE)
		target_compile_definitions(3rdparty_vulkan INTERFACE -DHAVE_VULKAN)
		target_link_libraries(3rdparty_vulkan INTERFACE Vulkan::Vulkan)

		if(UNIX AND NOT APPLE AND NOT ANDROID)
			find_package(Wayland)
			if (WAYLAND_FOUND)
				target_include_directories(3rdparty_vulkan
					INTERFACE ${WAYLAND_INCLUDE_DIR})
			endif()
		endif()

		set(VULKAN_TARGET 3rdparty_vulkan)
	else()
		message(WARNING "USE_VULKAN was enabled, but libvulkan was not found. RPCS3 will be compiled without Vulkan support.")
		if(APPLE)
			message(FATAL_ERROR "To build without Vulkan support on macOS, please disable USE_VULKAN.")
		endif()
	endif()
endif()

# AsmJit
add_subdirectory(asmjit EXCLUDE_FROM_ALL)

# SDL3
set(SDL3_TARGET 3rdparty_dummy_lib)
if(USE_SDL)
	if(USE_SYSTEM_SDL)
		find_package(SDL3)
		if(SDL3_FOUND AND SDL3_VERSION VERSION_GREATER_EQUAL 3.2.0)
			message(STATUS "Using system SDL3 version '${SDL3_VERSION}'")
			add_library(3rdparty_sdl3 INTERFACE)
			target_compile_definitions(3rdparty_sdl3 INTERFACE -DHAVE_SDL3=1)
			target_link_libraries(3rdparty_sdl3 INTERFACE SDL3::SDL3)
			set(SDL3_TARGET 3rdparty_sdl3)
		else()
			message(FATAL_ERROR "SDL3 is not available on this system")
		endif()
	else()
		message(STATUS "Using static SDL3 from 3rdparty")
		add_subdirectory(libsdl-org EXCLUDE_FROM_ALL)
		target_compile_definitions(SDL3-static INTERFACE -DHAVE_SDL3=1)
		set(SDL3_TARGET SDL3-static)
		set(SDL3_DIR "${CMAKE_CURRENT_BINARY_DIR}/libsdl-org/SDL" CACHE STRING "")
	endif()
endif()

# OpenAL
if (NOT ANDROID)
	add_subdirectory(OpenAL EXCLUDE_FROM_ALL)
else()
	add_library(3rdparty_openal INTERFACE)
	target_compile_definitions(3rdparty_openal INTERFACE WITHOUT_OPENAL=1)
endif()

# FAudio
set(FAUDIO_TARGET 3rdparty_dummy_lib)
if(USE_FAUDIO)
	# FAudio depends on SDL3
	find_package(SDL3)
	if (USE_SYSTEM_FAUDIO)
		if (SDL3_FOUND AND SDL3_VERSION VERSION_GREATER_EQUAL 3.2.0)
			message(STATUS "RPCS3: Using system FAudio")
			find_package(FAudio REQUIRED CONFIGS FAudioConfig.cmake FAudio-config.cmake)
			add_library(3rdparty_FAudio INTERFACE)
			target_link_libraries(3rdparty_FAudio INTERFACE FAudio)
			target_compile_definitions(3rdparty_FAudio INTERFACE -DHAVE_FAUDIO)
			set(FAUDIO_TARGET 3rdparty_FAudio)
		else()
			message(WARNING
				"RPCS3: System FAudio requires SDL 3.2.0 or newer. Since a valid SDL3"
				">=3.2.0 version cannot be found, building with FAudio will be skipped.")
			set(USE_FAUDIO OFF CACHE BOOL "Disabled using system FAudio with SDL < 3.2.0" FORCE)
		endif()
	else()
		if (SDL3_FOUND AND SDL3_VERSION VERSION_GREATER_EQUAL 3.2.0)
			message(STATUS "RPCS3: Using builtin FAudio")
			set(BUILD_SHARED_LIBS OFF CACHE BOOL "Build shared library")
			add_subdirectory(FAudio EXCLUDE_FROM_ALL)
			target_compile_definitions(FAudio-static INTERFACE -DHAVE_FAUDIO)
			set(FAUDIO_TARGET FAudio-static)
		else()
			message(FATAL_ERROR
				"-- RPCS3: 3rdparty FAudio requires SDL 3.2.0 or newer. Since a valid SDL3"
				">=3.2.0 version cannot be found, building with FAudio will be skipped.")
			set(USE_FAUDIO OFF CACHE BOOL "Disabled FAudio with SDL < 3.2.0" FORCE)
		endif()
	endif()
endif()

set_property(TARGET ${FAUDIO_TARGET} PROPERTY FOLDER "3rdparty/")


# FFMPEG
if(NOT ANDROID)
	add_library(3rdparty_ffmpeg INTERFACE)

	# Select the version of ffmpeg to use, default is builtin
	if(USE_SYSTEM_FFMPEG)
		message(STATUS "RPCS3: using shared ffmpeg")
		find_package(FFMPEG REQUIRED)

		target_include_directories(3rdparty_ffmpeg INTERFACE ${FFMPEG_INCLUDE_DIR})
		target_link_libraries(3rdparty_ffmpeg INTERFACE ${FFMPEG_LIBRARIES})
	else()
		message(STATUS "RPCS3: using builtin ffmpeg")
		add_subdirectory(ffmpeg EXCLUDE_FROM_ALL)
		# ffmpeg-core libraries are extracted to CMAKE_BINARY_DIR
		set(FFMPEG_LIB_DIR "${CMAKE_BINARY_DIR}/3rdparty/ffmpeg/lib")

		if (WIN32)
			target_link_libraries(3rdparty_ffmpeg INTERFACE "Bcrypt.lib")
		endif()

		find_library(FFMPEG_LIB_AVFORMAT avformat PATHS ${FFMPEG_LIB_DIR} NO_DEFAULT_PATH)
		find_library(FFMPEG_LIB_AVCODEC avcodec PATHS ${FFMPEG_LIB_DIR} NO_DEFAULT_PATH)
		find_library(FFMPEG_LIB_AVUTIL avutil PATHS ${FFMPEG_LIB_DIR} NO_DEFAULT_PATH)
		find_library(FFMPEG_LIB_SWSCALE swscale PATHS ${FFMPEG_LIB_DIR} NO_DEFAULT_PATH)
		find_library(FFMPEG_LIB_SWRESAMPLE swresample PATHS ${FFMPEG_LIB_DIR} NO_DEFAULT_PATH)

		if (FFMPEG_LIB_AVFORMAT MATCHES "FFMPEG_LIB_AVFORMAT-NOTFOUND")
			message(FATAL_ERROR "@#$%! FFMPEG NOT FOUND! ${FFMPEG_LIB_DIR}")
		endif()

		target_link_libraries(3rdparty_ffmpeg
		INTERFACE
			${FFMPEG_LIB_AVFORMAT}
			${FFMPEG_LIB_AVCODEC}
			${FFMPEG_LIB_AVUTIL}
			${FFMPEG_LIB_SWSCALE}
			${FFMPEG_LIB_SWRESAMPLE}
		)
		target_include_directories(3rdparty_ffmpeg INTERFACE "ffmpeg/include")
	endif()
endif()


# GLEW
add_library(3rdparty_glew INTERFACE)
if(NOT MSVC AND NOT ANDROID)
	find_package(GLEW REQUIRED)
	target_link_libraries(3rdparty_glew INTERFACE GLEW::GLEW)
endif()


# LLVM
add_subdirectory(llvm EXCLUDE_FROM_ALL)

# WOLFSSL
add_subdirectory(wolfssl EXCLUDE_FROM_ALL)

# CURL
add_subdirectory(curl EXCLUDE_FROM_ALL)

# MINIUPNP
add_subdirectory(miniupnp EXCLUDE_FROM_ALL)

# RTMIDI
add_subdirectory(rtmidi EXCLUDE_FROM_ALL)

# OPENCV
add_subdirectory(opencv EXCLUDE_FROM_ALL)

# FUSION
add_subdirectory(fusion EXCLUDE_FROM_ALL)

# FERAL INTERACTIVE
add_subdirectory(feralinteractive EXCLUDE_FROM_ALL)

# add nice ALIAS targets for ease of use
if(USE_SYSTEM_LIBUSB)
	add_library(3rdparty::libusb ALIAS usb-1.0-shared)
else()
	add_library(3rdparty::libusb ALIAS usb-1.0-static)
endif()
add_library(3rdparty::zlib ALIAS 3rdparty_zlib)
add_library(3rdparty::zstd ALIAS 3rdparty_zstd)
add_library(3rdparty::7zip ALIAS 3rdparty_7zip)
add_library(3rdparty::flatbuffers ALIAS 3rdparty_flatbuffers)
add_library(3rdparty::pugixml ALIAS pugixml)
add_library(3rdparty::glslang ALIAS 3rdparty_glslang)
add_library(3rdparty::yaml-cpp ALIAS yaml-cpp)
add_library(3rdparty::hidapi ALIAS 3rdparty_hidapi)
add_library(3rdparty::libpng ALIAS ${LIBPNG_TARGET})
add_library(3rdparty::opengl ALIAS 3rdparty_opengl)
add_library(3rdparty::stblib ALIAS 3rdparty_stblib)
add_library(3rdparty::discordRPC ALIAS 3rdparty_discordRPC)
add_library(3rdparty::faudio ALIAS ${FAUDIO_TARGET})
add_library(3rdparty::libevdev ALIAS ${LIBEVDEV_TARGET})
add_library(3rdparty::vulkan ALIAS ${VULKAN_TARGET})
add_library(3rdparty::openal ALIAS 3rdparty_openal)
add_library(3rdparty::ffmpeg ALIAS 3rdparty_ffmpeg)
add_library(3rdparty::glew ALIAS 3rdparty_glew)
add_library(3rdparty::wolfssl ALIAS wolfssl)
add_library(3rdparty::libcurl ALIAS 3rdparty_libcurl)
add_library(3rdparty::soundtouch ALIAS soundtouch)
add_library(3rdparty::sdl3 ALIAS ${SDL3_TARGET})
add_library(3rdparty::miniupnpc ALIAS 3rdparty_miniupnpc)
add_library(3rdparty::rtmidi ALIAS rtmidi)
add_library(3rdparty::opencv ALIAS ${OPENCV_TARGET})
add_library(3rdparty::fusion ALIAS Fusion)
add_library(3rdparty::feralinteractive ALIAS 3rdparty_feralinteractive)
