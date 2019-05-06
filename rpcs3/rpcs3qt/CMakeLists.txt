file(GLOB SRC_FILES "*.cpp")
file(GLOB UI_FILES "*.ui")

set(RES_FILES "../resources.qrc")

if(WIN32)
	list(APPEND RES_FILES "../windows.qrc")
endif()

add_library(rpcs3_ui ${SRC_FILES} ${UI_FILES} ${RES_FILES})

set_target_properties(rpcs3_ui
	PROPERTIES
		AUTOMOC ON
		AUTOUIC ON
		AUTORCC ON)

target_link_libraries(rpcs3_ui
	PUBLIC
		3rdparty::qt5 3rdparty::yaml-cpp

	PRIVATE
		rpcs3_emu
		3rdparty::zlib 3rdparty::pugixml
		3rdparty::discord-rpc
		3rdparty::hidapi
		3rdparty::libusb)

