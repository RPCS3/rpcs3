file(GLOB SRC_FILES "*.cpp")

add_library(rpcs3_rsx_gl ${SRC_FILES})
target_link_libraries(rpcs3_rsx_gl
	PRIVATE
	  rpcs3_stdafx 3rdparty::opengl 3rdparty::gsl 3rdparty::optional 3rdparty::cereal)
