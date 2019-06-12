file(GLOB SRC_FILES "*.cpp")

add_library(rpcs3_emu ${SRC_FILES})

target_link_libraries(rpcs3_emu
	PRIVATE
		3rdparty::zlib 3rdparty::yaml-cpp
	PUBLIC
		3rdparty::libevdev)

# For stdafx.h
target_include_directories(rpcs3_emu
	PUBLIC
		${RPCS3_SRC_DIR})


# Utilities
file(GLOB UTILITIES_SRC_FILES "../../Utilities/*.cpp")
target_sources(rpcs3_emu PRIVATE ${UTILITIES_SRC_FILES})

target_include_directories(rpcs3_emu PUBLIC "${CMAKE_SOURCE_DIR}")

target_link_libraries(rpcs3_emu
	PUBLIC
		3rdparty::pugixml
		3rdparty::gsl)

set_source_files_properties("../../Utilities/JIT.cpp" PROPERTIES COMPILE_FLAGS -fno-rtti)


# Crypto
file(GLOB CRYPTO_SRC_FILES "../Crypto/*.cpp")
target_sources(rpcs3_emu PRIVATE ${CRYPTO_SRC_FILES})

# Loader
file(GLOB LOADER_SRC_FILES "../Loader/*.cpp")
target_sources(rpcs3_emu PRIVATE ${LOADER_SRC_FILES})


# Audio
file(GLOB_RECURSE AUDIO_SRC_FILES "Audio/*.cpp")
target_sources(rpcs3_emu PRIVATE ${AUDIO_SRC_FILES})

if(WIN32)
	# Slimmed down version of minidx9 for XAudio2_7 only
	include_directories(BEFORE "${RPCS3_SRC_DIR}/../3rdparty/XAudio2_7")
endif()

target_link_libraries(rpcs3_emu
	PUBLIC
		3rdparty::alsa 3rdparty::pulse 3rdparty::openal)


# Cell
file(GLOB_RECURSE CELL_SRC_FILES "Cell/*.cpp")
target_sources(rpcs3_emu PRIVATE ${CELL_SRC_FILES})

if(NOT MSVC)
	set_source_files_properties(Cell/PPUTranslator.cpp PROPERTIES COMPILE_FLAGS -fno-rtti)
endif()

target_link_libraries(rpcs3_emu
	PRIVATE
		3rdparty::stblib 3rdparty::libpng)

if(CMAKE_COMPILER_IS_GNUCXX)
	target_link_libraries(rpcs3_emu PUBLIC -latomic)
endif()


# CPU
file(GLOB_RECURSE CPU_SRC_FILES "CPU/*.cpp")
target_sources(rpcs3_emu PRIVATE ${CPU_SRC_FILES})

target_link_libraries(rpcs3_emu
	PUBLIC 3rdparty::llvm 3rdparty::asmjit)


# Io
file(GLOB_RECURSE IO_SRC_FILES "Io/*.cpp")
target_sources(rpcs3_emu PRIVATE ${IO_SRC_FILES})


# Memory
file(GLOB_RECURSE MEMORY_SRC_FILES "Memory/*.cpp")
target_sources(rpcs3_emu PRIVATE ${MEMORY_SRC_FILES})


# RSX
file(GLOB RSX_SRC_FILES
	"RSX/*.cpp"
	"RSX/Common/*.cpp"
	"RSX/Null/*.cpp"
	"RSX/Overlays/*.cpp"
	"RSX/Capture/*.cpp"
	"RSX/GL/*.cpp")

if(TARGET 3rdparty_vulkan)
	file(GLOB VULKAN_SRCS "RSX/VK/*.cpp")
	list(APPEND RSX_SRC_FILES ${VULKAN_SRCS})
endif()

if (WIN32)
	file(GLOB DX12_SRCS "RSX/D3D12/*.cpp")
	list(APPEND RSX_SRC_FILES ${DX12_SRCS})
endif()

target_sources(rpcs3_emu PRIVATE ${RSX_SRC_FILES})

target_link_libraries(rpcs3_emu
	PUBLIC
		3rdparty::ffmpeg 3rdparty::cereal
		3rdparty::opengl 3rdparty::stblib
		3rdparty::vulkan 3rdparty::glew
	PRIVATE
		3rdparty::gsl 3rdparty::xxhash
		3rdparty::dx12)


# Setup cotire
option(UNITY_BUILD_EMU "Use unity build for rpcs3_emu target" OFF)

set_target_properties(rpcs3_emu PROPERTIES
	COTIRE_CXX_PREFIX_HEADER_INIT "${RPCS3_SRC_DIR}/stdafx.h"
	COTIRE_ADD_UNITY_BUILD ${UNITY_BUILD_EMU})

cotire(rpcs3_emu)
