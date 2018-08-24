file(GLOB SRC_FILES "*.cpp")

file(GLOB UI_FILES "*.ui")

add_library(rpcs3_ui ${SRC_FILES} ${UI_FILES})

target_link_libraries(rpcs3_ui
	PUBLIC
		${RPCS3_QT_LIBS} 3rdparty::yaml-cpp

	PRIVATE
		rpcs3_emu rpcs3_utilities
		3rdparty::zlib 3rdparty::pugixml
		3rdparty::discord-rpc
		3rdparty::hidapi)

