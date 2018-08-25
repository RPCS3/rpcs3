file(GLOB SRC_FILES "*.cpp" "Common/*.cpp" "Null/*.cpp" "Overlays/*.cpp" "Capture/*.cpp" "GL/*.cpp")

if(TARGET 3rdparty_vulkan)
	file(GLOB VULKAN_SRCS "VK/*.cpp")
	list(APPEND SRC_FILES ${VULKAN_SRCS})
endif()

if (WIN32)
	file(GLOB DX12_SRCS "D3D12/*.cpp")
	list(APPEND SRC_FILES ${DX12_SRCS})
endif()


add_library(rpcs3_rsx ${SRC_FILES})

target_link_libraries(rpcs3_rsx
	PRIVATE
		rpcs3_stdafx
	PUBLIC
		3rdparty::cereal
		3rdparty::opengl 3rdparty::gsl
		3rdparty::stblib 3rdparty::xxhash
		3rdparty::ffmpeg 3rdparty::vulkan
		3rdparty::dx12 3rdparty::glew)
