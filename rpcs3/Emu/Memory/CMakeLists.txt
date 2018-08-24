set(SRC_FILES vm.cpp)

add_library(rpcs3_memory ${SRC_FILES})
target_link_libraries(rpcs3_memory
	PRIVATE
		rpcs3_stdafx
		rpcs3_utilities
		rpcs3_rsx)
