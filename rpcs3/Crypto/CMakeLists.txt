file(GLOB SRC_FILES "*.cpp")

add_library(rpcs3_crypto ${SRC_FILES})
target_include_directories(rpcs3_crypto PUBLIC .)

target_link_libraries(rpcs3_crypto
	PRIVATE
		rpcs3_stdafx
	PUBLIC
		3rdparty::zlib)

