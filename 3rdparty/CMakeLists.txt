find_package(PkgConfig)
include(ExternalProject)
include(CMakeDependentOption)

set(CMAKE_CXX_STANDARD 20)

# Dummy target to use when lib isn't available
add_library(3rdparty_dummy_lib INTERFACE)


# ZLib
add_subdirectory(zlib EXCLUDE_FROM_ALL)

# 7z sdk
add_subdirectory(7z EXCLUDE_FROM_ALL)

add_library(3rdparty_flatbuffers INTERFACE)
if (USE_SYSTEM_FLATBUFFERS)
	pkg_check_modules(FLATBUFFERS REQUIRED IMPORTED_TARGET flatbuffers>=2.0.0)
	target_link_libraries(3rdparty_flatbuffers INTERFACE PkgConfig::FLATBUFFERS)
else()
	target_include_directories(3rdparty_flatbuffers INTERFACE flatbuffers/include)
endif()

# libPNG
add_subdirectory(libpng EXCLUDE_FROM_ALL)


# pugixml
if (USE_SYSTEM_PUGIXML)
	pkg_check_modules(PUGIXML REQUIRED IMPORTED_TARGET pugixml>=1.11)
	add_library(pugixml INTERFACE)
	target_link_libraries(pugixml INTERFACE PkgConfig::PUGIXML)
else()
	add_subdirectory(pugixml EXCLUDE_FROM_ALL)
endif()


# libusb
if(CMAKE_SYSTEM MATCHES "DragonFly|FreeBSD")
	pkg_check_modules(LIBUSB REQUIRED IMPORTED_TARGET libusb-1.0>=1.0 )
	CMAKE_DEPENDENT_OPTION( USE_SYSTEM_LIBUSB "Use system libusb-1.0 as shared library" ON
			"LIBUSB_FOUND" OFF)
else()
	pkg_check_modules(LIBUSB IMPORTED_TARGET libusb-1.0>=1.0 )
	CMAKE_DEPENDENT_OPTION( USE_SYSTEM_LIBUSB "Use system libusb-1.0 as shared library" OFF
			"LIBUSB_FOUND" OFF)
endif()
if(CMAKE_SYSTEM MATCHES "DragonFly|FreeBSD")
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
	endif()
endif()


# hidapi
add_subdirectory(hidapi)


# Vulkan
add_subdirectory(glslang EXCLUDE_FROM_ALL)
add_subdirectory(SPIRV EXCLUDE_FROM_ALL)


# yaml-cpp
# We don't want to install yaml-cpp but its cmake file doesn't have option
# to disable it...
# So we just install it to a different directory
set(YAML_CPP_INSTALL OFF CACHE BOOL "Don't install YAML")
set(CMAKE_INSTALL_PREFIX_OLD ${CMAKE_INSTALL_PREFIX})
set(CMAKE_INSTALL_PREFIX ${CMAKE_BINARY_DIR}/yaml-cpp_install)

set(YAML_CPP_BUILD_TESTS OFF CACHE BOOL "Enable testing" FORCE)
set(YAML_CPP_BUILD_TOOLS OFF CACHE BOOL "Enable parse tools" FORCE)
set(YAML_CPP_BUILD_CONTRIB OFF CACHE BOOL "Enable contrib stuff in library" FORCE)
add_subdirectory(yaml-cpp EXCLUDE_FROM_ALL)

set(CMAKE_INSTALL_PREFIX ${CMAKE_INSTALL_PREFIX_OLD})


# xxHash
if (USE_SYSTEM_XXHASH)
	pkg_check_modules(XXHASH REQUIRED IMPORTED_TARGET libxxhash)
	add_library(xxhash INTERFACE)
	target_link_libraries(xxhash INTERFACE PkgConfig::XXHASH)
else()
	set(XXHASH_BUNDLED_MODE ON)
	set(XXHASH_BUILD_XXHSUM OFF)
	set(BUILD_SHARED_LIBS OFF CACHE BOOL "Make xxHash build static libs")
	add_subdirectory(xxHash/cmake_unofficial EXCLUDE_FROM_ALL)
	target_include_directories(xxhash INTERFACE xxHash)
endif()

# OpenGL

# Prefer GLVND for OpenGL rather than legacy, unless it's been defined elsewhere, in the case of AppImage builds
if(NOT DEFINED OpenGL_GL_PREFERENCE)
	set(OpenGL_GL_PREFERENCE GLVND)
endif()
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


# stblib
add_library(3rdparty_stblib INTERFACE)
target_include_directories(3rdparty_stblib INTERFACE stblib/include)


# DiscordRPC
add_subdirectory(discord-rpc)


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
		target_link_libraries(3rdparty_vulkan INTERFACE SPIRV SPIRV-Tools-opt Vulkan::Vulkan)

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

# AsmJit
add_subdirectory(asmjit EXCLUDE_FROM_ALL)

# OpenAL
add_subdirectory(OpenAL EXCLUDE_FROM_ALL)

# FAudio
set(FAUDIO_TARGET 3rdparty_dummy_lib)
if(USE_FAUDIO)
	# FAudio depends on SDL2
	find_package(SDL2)
	if (NOT SDL2_FOUND OR SDL2_VERSION VERSION_LESS 2.0.12)
		message(WARNING
			"-- RPCS3: FAudio requires SDL 2.0.9 or newer. Please note, this warning"
			"can also be displayed with SDL2 versions between 2.0.9-2.0.12, as the"
			"CMake config files are not correctly installed. Since a valid SDL2"
			">=2.0.9 version cannot be found, building with FAudio will be skipped.")
		set(USE_FAUDIO False)
	else()
		if (USE_SYSTEM_FAUDIO)
			message(STATUS "RPCS3: Using system FAudio")
			find_package(FAudio REQUIRED CONFIGS FAudioConfig.cmake FAudio-config.cmake)
			add_library(3rdparty_FAudio INTERFACE)
			target_link_libraries(3rdparty_FAudio INTERFACE FAudio)
			target_compile_definitions(3rdparty_FAudio INTERFACE -DHAVE_FAUDIO)
			set(FAUDIO_TARGET 3rdparty_FAudio)
		else()
			message(STATUS "RPCS3: Using builtin FAudio")
			set(BUILD_SHARED_LIBS OFF CACHE BOOL "Build shared library")
			add_subdirectory(FAudio EXCLUDE_FROM_ALL)
			target_compile_definitions(FAudio INTERFACE -DHAVE_FAUDIO)
			set(FAUDIO_TARGET FAudio)
		endif()
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

# WOLFSSL
if(USE_SYSTEM_WOLFSSL)
	message("-- RPCS3: using shared wolfssl")
	pkg_check_modules(WolfSSL REQUIRED IMPORTED_TARGET wolfssl>=4.7.0)
	add_library(wolfssl INTERFACE)
	target_link_libraries(wolfssl INTERFACE PkgConfig::WolfSSL)
else()
	SET(BUILD_TESTS NO CACHE BOOL "Build test applications")
	add_compile_definitions(HAVE_FFDHE_2048 TFM_TIMING_RESISTANT ECC_TIMING_RESISTANT
					WC_RSA_BLINDING HAVE_AESGCM WOLFSSL_SHA512 WOLFSSL_SHA384 NO_DSA HAVE_ECC TFM_ECC256 ECC_SHAMIR
					NO_RC4 NO_HC128 NO_RABBIT WOLFSSL_SHA224 WOLFSSL_SHA3 WOLFSSL_SHAKE256 HAVE_POLY1305
					HAVE_ONE_TIME_AUTH HAVE_CHACHA HAVE_HASHDRBG HAVE_TLS_EXTENSIONS HAVE_SNI HAVE_SUPPORTED_CURVES
					HAVE_EXTENDED_MASTER NO_RC4 HAVE_ENCRYPT_THEN_MAC NO_PSK NO_MD4
					WC_NO_ASYNC_THREADING OPENSSL_EXTRA WOLFSSL_DES_ECB WC_NO_HARDEN)
	set(ENABLED_HARDEN OFF CACHE BOOL "Disable Harden")
	add_subdirectory(wolfssl EXCLUDE_FROM_ALL)
	target_include_directories(wolfssl INTERFACE "wolfssl/")
	# This is needed for CURL
	configure_file(${CMAKE_CURRENT_SOURCE_DIR}/wolfssl/wolfssl/options.h.in ${CMAKE_CURRENT_BINARY_DIR}/wolfssl/wolfssl/options.h)
	set(WolfSSL_LIBRARY "${CMAKE_CURRENT_BINARY_DIR}/wolfssl/libwolfssl.a")
	set(WolfSSL_LIBRARIES "${CMAKE_CURRENT_BINARY_DIR}/wolfssl/libwolfssl.a")
	set(WolfSSL_INCLUDE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/wolfssl/" "${CMAKE_CURRENT_BINARY_DIR}/wolfssl/")
	set(WolfSSL_INCLUDE_DIRS "${CMAKE_CURRENT_SOURCE_DIR}/wolfssl/" "${CMAKE_CURRENT_BINARY_DIR}/wolfssl/")
endif()

# CURL
add_subdirectory(curl EXCLUDE_FROM_ALL)

# add nice ALIAS targets for ease of use
if(USE_SYSTEM_LIBUSB)
	add_library(3rdparty::libusb ALIAS usb-1.0-shared)
else()
	add_library(3rdparty::libusb ALIAS usb-1.0-static)
endif()
add_library(3rdparty::zlib ALIAS 3rdparty_zlib)
add_library(3rdparty::7z ALIAS 3rdparty_7z)
add_library(3rdparty::flatbuffers ALIAS 3rdparty_flatbuffers)
add_library(3rdparty::pugixml ALIAS pugixml)
add_library(3rdparty::yaml-cpp ALIAS yaml-cpp)
add_library(3rdparty::xxhash ALIAS xxhash)
add_library(3rdparty::hidapi ALIAS 3rdparty_hidapi)
add_library(3rdparty::libpng ALIAS ${LIBPNG_TARGET})
add_library(3rdparty::opengl ALIAS 3rdparty_opengl)
add_library(3rdparty::stblib ALIAS 3rdparty_stblib)
add_library(3rdparty::discordRPC ALIAS 3rdparty_discordRPC)
add_library(3rdparty::alsa ALIAS ${ALSA_TARGET})
add_library(3rdparty::pulse ALIAS ${PULSE_TARGET})
add_library(3rdparty::faudio ALIAS ${FAUDIO_TARGET})
add_library(3rdparty::libevdev ALIAS ${LIBEVDEV_TARGET})
add_library(3rdparty::vulkan ALIAS ${VULKAN_TARGET})
add_library(3rdparty::openal ALIAS 3rdparty_openal)
add_library(3rdparty::ffmpeg ALIAS 3rdparty_ffmpeg)
add_library(3rdparty::glew ALIAS 3rdparty_glew)
add_library(3rdparty::wolfssl ALIAS wolfssl)
add_library(3rdparty::libcurl ALIAS libcurl)
