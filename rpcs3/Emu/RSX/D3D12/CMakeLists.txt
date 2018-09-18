file(GLOB SRC_FILES "*.cpp")

add_library(rpcs3_rsx_dx12 ${SRC_FILES})
target_link_libraries(rpcs3_rsx_dx12
	PRIVATE
	  rpcs3_stdafx 3rdparty::gsl 3rdparty::optional)
