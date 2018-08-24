file(GLOB_RECURSE SRC_FILES "*.cpp")

if(WIN32)
	# Slimmed down version of minidx9 for XAudio2_7 only
	include_directories(BEFORE "${RPCS3_SRC_DIR}/../3rdparty/XAudio2_7")
endif()

add_library(rpcs3_audio ${SRC_FILES})
target_link_libraries(rpcs3_audio
	PRIVATE
		rpcs3_stdafx
		3rdparty::alsa
		3rdparty::pulse
		3rdparty::openal)
