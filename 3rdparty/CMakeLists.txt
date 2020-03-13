find_package(PkgConfig)
include(ExternalProject)

# Dummy target to use when lib isn't available
add_library(3rdparty_dummy_lib INTERFACE)


# ZLib
if (USE_SYSTEM_ZLIB)
	find_package(ZLIB QUIET)
endif()

if (NOT ZLIB_FOUND)
	message(STATUS "Using builtin ZLIB")
	set(SKIP_INSTALL_ALL ON)
	add_subdirectory(zlib EXCLUDE_FROM_ALL)
	set(ZLIB_INCLUDE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/zlib" "${CMAKE_CURRENT_BINARY_DIR}/zlib")
	set(ZLIB_LIBRARY zlibstatic)
endif()

add_library(3rdparty_zlib INTERFACE)
target_link_libraries(3rdparty_zlib INTERFACE ${ZLIB_LIBRARY})
target_include_directories(3rdparty_zlib INTERFACE ${ZLIB_INCLUDE_DIR})

# 7z sdk
if(WIN32)
	add_library(3rdparty_7z STATIC EXCLUDE_FROM_ALL
		7z/Sha256.c
		7z/Ppmd7Dec.c
		7z/XzDec.c
		7z/XzEnc.c
		7z/Lzma2Dec.c
		7z/XzCrc64.c
		7z/DllSecur.c
		7z/Lzma2DecMt.c
		7z/BraIA64.c
		7z/7zFile.c
		7z/7zStream.c
		7z/Lzma86Enc.c
		7z/Threads.c
		7z/7zAlloc.c
		7z/LzmaEnc.c
		7z/MtCoder.c
		7z/Lzma86Dec.c
		7z/Sort.c
		7z/LzFindMt.c
		7z/7zDec.c
		7z/Bcj2.c
		7z/Ppmd7.c
		7z/Bra86.c
		7z/Bcj2Enc.c
		7z/7zBuf.c
		7z/Aes.c
		7z/AesOpt.c
		7z/XzCrc64Opt.c
		7z/7zArcIn.c
		7z/Lzma2Enc.c
		7z/Bra.c
		7z/7zCrcOpt.c
		7z/7zBuf2.c
		7z/LzFind.c
		7z/Ppmd7Enc.c
		7z/CpuArch.c
		7z/Delta.c
		7z/XzIn.c
		7z/Alloc.c
		7z/Xz.c
		7z/LzmaDec.c
		7z/LzmaLib.c
		7z/7zCrc.c
		7z/MtDec.c)
	target_include_directories(3rdparty_7z INTERFACE 7z)
else()
	add_library(3rdparty_7z INTERFACE)
endif()

# libPNG
# Select the version of libpng to use, default is builtin
if (NOT USE_SYSTEM_LIBPNG)
	# We use libpng's static library and don't need to build the shared library and run the tests
	set(PNG_SHARED OFF CACHE BOOL "Build shared lib" FORCE)
	set(PNG_TESTS OFF CACHE BOOL "Build libpng tests" FORCE)
	set(PNG_BUILD_ZLIB ON CACHE BOOL "ZLIB is already build or package is found" FORCE)
	set(SKIP_INSTALL_ALL ON)
	add_subdirectory(libpng EXCLUDE_FROM_ALL)

	set(LIBPNG_TARGET png_static)

	target_include_directories(png_static INTERFACE "${libpng_BINARY_DIR}" "${libpng_SOURCE_DIR}")
else()
	find_package(PNG REQUIRED)

	add_library(3rdparty_system_libpng INTERFACE)
	target_include_directories(3rdparty_system_libpng INTERFACE ${PNG_INCLUDE_DIR})
	target_link_libraries(3rdparty_system_libpng INTERFACE ${PNG_LIBRARY})
	target_compile_definitions(3rdparty_system_libpng INTERFACE ${PNG_DEFINITIONS})

	set(LIBPNG_TARGET 3rdparty_system_libpng)
endif()


# pugixml
add_subdirectory(pugixml EXCLUDE_FROM_ALL)


# hidapi
add_library(3rdparty_hidapi INTERFACE)
target_include_directories(3rdparty_hidapi INTERFACE hidapi/hidapi)

if(APPLE)
	add_subdirectory(hidapi/mac EXCLUDE_FROM_ALL)
	target_include_directories(hidapi-mac PUBLIC hidapi/hidapi)

	target_link_libraries(3rdparty_hidapi INTERFACE hidapi-mac "-framework CoreFoundation" "-framework IOKit")
elseif(CMAKE_SYSTEM MATCHES "Linux")
	add_subdirectory(hidapi/linux EXCLUDE_FROM_ALL)
	target_include_directories(hidapi-hidraw PUBLIC hidapi/hidapi)

	target_link_libraries(3rdparty_hidapi INTERFACE hidapi-hidraw udev)
elseif(WIN32)
	add_subdirectory(hidapi/windows EXCLUDE_FROM_ALL)
	target_include_directories(hidapi-hid PUBLIC hidapi/hidapi)

	target_link_libraries(3rdparty_hidapi INTERFACE hidapi-hid Shlwapi.lib)
else()
	add_subdirectory(hidapi/libusb EXCLUDE_FROM_ALL)
	target_include_directories(hidapi-libusb PUBLIC hidapi/hidapi)

	target_link_libraries(3rdparty_hidapi INTERFACE hidapi-libusb usb)
endif()


# libusb
if(CMAKE_SYSTEM MATCHES "DragonFly|FreeBSD")
	# Always use system libusb as reference implementation isn't supported
	add_library(usb-1.0-static INTERFACE)
	target_link_libraries(usb-1.0-static INTERFACE usb)
elseif(MSVC)
	# Windows time.h defines timespec but doesn't add any flag for it, which makes libusb attempt to define it again
	add_definitions(-DHAVE_STRUCT_TIMESPEC=1)
	add_subdirectory(libusb_cmake EXCLUDE_FROM_ALL)
else()
	add_subdirectory(libusb_cmake EXCLUDE_FROM_ALL)
endif()


# yaml-cpp
# We don't want to install yaml-cpp but its cmake file doesn't have option
# to disable it...
# So we just install it to a different directory
set(CMAKE_INSTALL_PREFIX_OLD ${CMAKE_INSTALL_PREFIX})
set(CMAKE_INSTALL_PREFIX ${CMAKE_BINARY_DIR}/yaml-cpp_install)

set(YAML_CPP_BUILD_TESTS OFF CACHE BOOL "Enable testing" FORCE)
set(YAML_CPP_BUILD_TOOLS OFF CACHE BOOL "Enable parse tools" FORCE)
set(YAML_CPP_BUILD_CONTRIB OFF CACHE BOOL "Enable contrib stuff in library" FORCE)
add_subdirectory(yaml-cpp EXCLUDE_FROM_ALL)

set(CMAKE_INSTALL_PREFIX ${CMAKE_INSTALL_PREFIX_OLD})


# xxHash
set(XXHASH_BUNDLED_MODE ON)
set(BUILD_SHARED_LIBS OFF CACHE BOOL "Make xxHash build static libs")
add_subdirectory(xxHash/cmake_unofficial EXCLUDE_FROM_ALL)
target_include_directories(xxhash INTERFACE xxHash)


# cereal
add_library(3rdparty_cereal INTERFACE)
target_include_directories(3rdparty_cereal INTERFACE cereal/include)


# OpenGL

# Prefer GLVND for OpenGL rather than legacy
set(OpenGL_GL_PREFERENCE GLVND)
find_package(OpenGL REQUIRED)

add_library(3rdparty_opengl INTERFACE)
target_include_directories(3rdparty_opengl INTERFACE GL)

if (WIN32)
	if(NOT MSVC)
		target_link_libraries(3rdparty_opengl INTERFACE ${OPENGL_LIBRARIES} opengl32.lib glu32.lib)
	else()
		target_link_libraries(3rdparty_opengl INTERFACE dxgi.lib d2d1.lib dwrite.lib)
	endif()
else()
	target_link_libraries(3rdparty_opengl INTERFACE ${OPENGL_LIBRARIES})

	target_compile_definitions(3rdparty_opengl
		INTERFACE
			-DGL_GLEXT_PROTOTYPES
			-DGLX_GLXEXT_PROTOTYPES)
endif()


# span
add_library(3rdparty_span INTERFACE)
target_include_directories(3rdparty_span INTERFACE span/include)


# stblib
add_library(3rdparty_stblib INTERFACE)
target_include_directories(3rdparty_stblib INTERFACE stblib)


# discord_rpc
add_library(3rdparty_discord-rpc INTERFACE)

# We don't want Discord Rich Presence on the BSDs and other OSes
if (USE_DISCORD_RPC AND (WIN32 OR CMAKE_SYSTEM MATCHES "Linux" OR APPLE))
	if (WIN32 AND NOT MSVC)
		ExternalProject_Add(discord-rpc
				GIT_REPOSITORY https://github.com/discordapp/discord-rpc
				BINARY_DIR ${CMAKE_CURRENT_BINARY_DIR}/discord-rpc
				INSTALL_COMMAND ""
			)

	endif()

	target_include_directories(3rdparty_discord-rpc INTERFACE discord-rpc/include)
	target_compile_definitions(3rdparty_discord-rpc INTERFACE -DWITH_DISCORD_RPC)

	set(DISCORD_RPC_LIB NOTFOUND)
	if (WIN32)
		if (NOT MSVC)
			set(DISCORD_RPC_LIB ${CMAKE_CURRENT_BINARY_DIR}/discord-rpc/src/libdiscord-rpc.a)
		else()
			find_library(DISCORD_RPC_LIB discord-rpc PATHS discord-rpc/lib/ NO_DEFAULT_PATH)
		endif()
	elseif(CMAKE_SYSTEM MATCHES "Linux")
		find_library(DISCORD_RPC_LIB discord-rpc-linux PATHS discord-rpc/lib/ NO_DEFAULT_PATH)
	elseif(APPLE)
		find_library(DISCORD_RPC_LIB discord-rpc-mac PATHS discord-rpc/lib/ NO_DEFAULT_PATH)
	endif()
	target_link_libraries(3rdparty_discord-rpc INTERFACE ${DISCORD_RPC_LIB})
	if(APPLE)
		target_link_libraries(3rdparty_discord-rpc INTERFACE "objc" "-framework Foundation" "-framework CoreServices")
	endif()
endif()


# ALSA
set(ALSA_TARGET 3rdparty_dummy_lib)

if(USE_ALSA)
	find_package(ALSA)
	if(ALSA_FOUND)
		add_library(3rdparty_alsa INTERFACE)
		target_compile_definitions(3rdparty_alsa INTERFACE -DHAVE_ALSA)
		target_include_directories(3rdparty_alsa SYSTEM INTERFACE ${ALSA_INCLUDE_DIRS})
		target_link_libraries(3rdparty_alsa INTERFACE ${ALSA_LIBRARIES})

		set(ALSA_TARGET 3rdparty_alsa)
	endif()
endif()


# Pulse
set(PULSE_TARGET 3rdparty_dummy_lib)
if(USE_PULSE)
	pkg_check_modules(PULSE libpulse-simple)

	if(PULSE_FOUND)
		add_library(3rdparty_pulse INTERFACE)
		target_compile_definitions(3rdparty_pulse INTERFACE -DHAVE_PULSE)
		target_include_directories(3rdparty_pulse SYSTEM
			INTERFACE ${PULSE_INCLUDE_DIRS})
		target_link_libraries(3rdparty_pulse INTERFACE ${PULSE_LDFLAGS})

		set(PULSE_TARGET 3rdparty_pulse)
	endif()
endif()

# libevdev
set(LIBEVDEV_TARGET 3rdparty_dummy_lib)
if(USE_LIBEVDEV)
	pkg_check_modules(LIBEVDEV libevdev)
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
	find_package(Vulkan)
	if(VULKAN_FOUND)
		add_library(3rdparty_vulkan INTERFACE)
		target_compile_definitions(3rdparty_vulkan INTERFACE -DHAVE_VULKAN)
		target_link_libraries(3rdparty_vulkan INTERFACE SPIRV Vulkan::Vulkan)

		if(UNIX AND NOT APPLE)
			find_package(Wayland)
			if (WAYLAND_FOUND)
				target_include_directories(3rdparty_vulkan
					INTERFACE ${WAYLAND_INCLUDE_DIR})

				target_compile_definitions(3rdparty_vulkan
					INTERFACE -DVK_USE_PLATFORM_WAYLAND_KHR)
			endif()
		endif()

		set(VULKAN_TARGET 3rdparty_vulkan)
	else()
		message("WARNING! USE_VULKAN was enabled, but libvulkan was not found. RPCS3 will be compiled without Vulkan support.")
	endif()
endif()


# OpenAL
if (MSVC)
	find_path(OPENAL_INCLUDE_DIR al.h PATHS OpenAL/include)
	find_library(OPENAL_LIBRARY OpenAL32 PATHS OpenAL/libs/Win64/)
else()
	find_package(OpenAL REQUIRED)
endif()

add_library(3rdparty_openal INTERFACE)
target_include_directories(3rdparty_openal INTERFACE ${OPENAL_INCLUDE_DIR})
target_link_libraries(3rdparty_openal INTERFACE ${OPENAL_LIBRARY})

# FAudio
set(FAUDIO_TARGET 3rdparty_dummy_lib)
if(USE_FAUDIO)
	# FAudio depends on SDL2
	pkg_check_modules(SDL2 sdl2)
	if (NOT SDL2_FOUND OR SDL2_VERSION VERSION_LESS 2.0.9)
		message("-- RPCS3: FAudio requires SDL 2.0.9 or newer.")
	else()
		set(BUILD_SHARED_LIBS OFF CACHE BOOL "Build shared library")
		add_subdirectory(FAudio EXCLUDE_FROM_ALL)
		target_compile_definitions(FAudio INTERFACE -DHAVE_FAUDIO)
		set(FAUDIO_TARGET FAudio)
	endif()
endif()

# FFMPEG
add_library(3rdparty_ffmpeg INTERFACE)

# Select the version of ffmpeg to use, default is builtin
if(USE_SYSTEM_FFMPEG)
	message("-- RPCS3: using shared ffmpeg")
	find_package(FFMPEG REQUIRED)

	target_include_directories(3rdparty_ffmpeg INTERFACE ${FFMPEG_INCLUDE_DIR})
	target_link_libraries(3rdparty_ffmpeg INTERFACE ${FFMPEG_LIBRARIES})
else()
	if (NOT MSVC AND WIN32)
		message("-- RPCS3: building ffmpeg submodule")

		ExternalProject_Add(ffmpeg-mingw
			DOWNLOAD_COMMAND ""
			SOURCE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/ffmpeg
			BINARY_DIR ${CMAKE_CURRENT_BINARY_DIR}/ffmpeg
			CONFIGURE_COMMAND  ${CMAKE_CURRENT_SOURCE_DIR}/ffmpeg/configure --prefix=./windows/x86_64 --arch=x86_64 --disable-avdevice --disable-programs --disable-avfilter --disable-postproc --disable-doc --disable-pthreads --enable-w32threads --disable-network --disable-everything --disable-encoders --disable-muxers --disable-hwaccels --disable-parsers --disable-protocols --enable-dxva2 --enable-static --disable-shared --enable-decoder=aac --enable-decoder=aac_latm --enable-decoder=atrac3 --enable-decoder=atrac3p --enable-decoder=mp3 --enable-decoder=pcm_s16le --enable-decoder=pcm_s8 --enable-decoder=h264 --enable-decoder=mpeg4 --enable-decoder=mpeg2video --enable-decoder=mjpeg --enable-decoder=mjpegb --enable-encoder=pcm_s16le --enable-encoder=ffv1 --enable-encoder=mpeg4 --enable-parser=h264 --enable-parser=mpeg4video --enable-parser=mpegaudio --enable-parser=mpegvideo --enable-parser=mjpeg --enable-parser=aac --enable-parser=aac_latm --enable-muxer=avi --enable-demuxer=h264 --enable-demuxer=m4v --enable-demuxer=mp3 --enable-demuxer=mpegvideo --enable-demuxer=mpegps --enable-demuxer=mjpeg --enable-demuxer=avi --enable-demuxer=aac --enable-demuxer=pmp --enable-demuxer=oma --enable-demuxer=pcm_s16le --enable-demuxer=pcm_s8 --enable-demuxer=wav --enable-hwaccel=h264_dxva2 --enable-indev=dshow --enable-protocol=file
			BUILD_COMMAND make -j 4
			INSTALL_COMMAND make install
		)

		set(FFMPEG_LIB_AVFORMAT "${CMAKE_CURRENT_BINARY_DIR}/ffmpeg/windows/x86_64/lib/libavformat.a")
		set(FFMPEG_LIB_AVCODEC "${CMAKE_CURRENT_BINARY_DIR}/ffmpeg/windows/x86_64/lib/libavcodec.a")
		set(FFMPEG_LIB_AVUTIL "${CMAKE_CURRENT_BINARY_DIR}/ffmpeg/windows/x86_64/lib/libavutil.a")
		set(FFMPEG_LIB_SWSCALE "${CMAKE_CURRENT_BINARY_DIR}/ffmpeg/windows/x86_64/lib/libswscale.a")
	else()
		message("-- RPCS3: using builtin ffmpeg")

		if (WIN32)
			set(FFMPEG_LIB_DIR "ffmpeg/windows/x86_64")
			target_link_libraries(3rdparty_ffmpeg INTERFACE "Bcrypt.lib")
		elseif(CMAKE_SYSTEM MATCHES "Linux")
			set(FFMPEG_LIB_DIR "ffmpeg/linux/x86_64")
		elseif(APPLE)
			set(FFMPEG_LIB_DIR "ffmpeg/macos/x86_64")
		else()
			message(FATAL_ERROR "Prebuilt ffmpeg is not available on this platform! Try USE_SYSTEM_FFMPEG=ON.")
		endif()

		find_library(FFMPEG_LIB_AVFORMAT avformat PATHS ${FFMPEG_LIB_DIR} NO_DEFAULT_PATH)
		find_library(FFMPEG_LIB_AVCODEC avcodec PATHS ${FFMPEG_LIB_DIR} NO_DEFAULT_PATH)
		find_library(FFMPEG_LIB_AVUTIL avutil PATHS ${FFMPEG_LIB_DIR} NO_DEFAULT_PATH)
		find_library(FFMPEG_LIB_SWSCALE swscale PATHS ${FFMPEG_LIB_DIR} NO_DEFAULT_PATH)
	endif()

	target_include_directories(3rdparty_ffmpeg INTERFACE "ffmpeg/include")

	target_link_libraries(3rdparty_ffmpeg
		INTERFACE
			${FFMPEG_LIB_AVFORMAT}
			${FFMPEG_LIB_AVCODEC}
			${FFMPEG_LIB_AVUTIL}
			${FFMPEG_LIB_SWSCALE})
endif()


# GLEW
add_library(3rdparty_glew INTERFACE)
if(NOT MSVC)
	find_package(GLEW 1.13.0 REQUIRED)
	target_link_libraries(3rdparty_glew INTERFACE GLEW::GLEW)
endif()


# LLVM
include(llvm.cmake)

# Wolfssl
add_subdirectory(wolfssl EXCLUDE_FROM_ALL)

# Libcurl
add_subdirectory(curl EXCLUDE_FROM_ALL)

# add nice ALIAS targets for ease of use
add_library(3rdparty::libusb ALIAS usb-1.0-static)
add_library(3rdparty::zlib ALIAS 3rdparty_zlib)
add_library(3rdparty::7z ALIAS 3rdparty_7z)
add_library(3rdparty::pugixml ALIAS pugixml)
add_library(3rdparty::yaml-cpp ALIAS yaml-cpp)
add_library(3rdparty::xxhash ALIAS xxhash)
add_library(3rdparty::hidapi ALIAS 3rdparty_hidapi)
add_library(3rdparty::libpng ALIAS ${LIBPNG_TARGET})
add_library(3rdparty::cereal ALIAS 3rdparty_cereal)
add_library(3rdparty::opengl ALIAS 3rdparty_opengl)
add_library(3rdparty::span ALIAS 3rdparty_span)
add_library(3rdparty::stblib ALIAS 3rdparty_stblib)
add_library(3rdparty::discord-rpc ALIAS 3rdparty_discord-rpc)
add_library(3rdparty::alsa ALIAS ${ALSA_TARGET})
add_library(3rdparty::pulse ALIAS ${PULSE_TARGET})
add_library(3rdparty::faudio ALIAS ${FAUDIO_TARGET})
add_library(3rdparty::libevdev ALIAS ${LIBEVDEV_TARGET})
add_library(3rdparty::vulkan ALIAS ${VULKAN_TARGET})
add_library(3rdparty::openal ALIAS 3rdparty_openal)
add_library(3rdparty::ffmpeg ALIAS 3rdparty_ffmpeg)
add_library(3rdparty::glew ALIAS 3rdparty_glew)
add_library(3rdparty::wolfssl ALIAS wolfssl-3-static)
add_library(3rdparty::libcurl ALIAS libcurl)
