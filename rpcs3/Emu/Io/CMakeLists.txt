file(GLOB_RECURSE SRC_FILES "*.cpp")

add_library(rpcs3_io ${SRC_FILES})
target_link_libraries(rpcs3_io
	PRIVATE
		rpcs3_stdafx rpcs3_utilities)
