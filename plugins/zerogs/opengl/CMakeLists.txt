# zerogs Plugin

# plugin name
set(Output zerogs)

set(CommonFlags
    -Wall
    )

set(OptimizationFlags
    -O2
    )

# Debug - Build
if(CMAKE_BUILD_TYPE STREQUAL Debug)
	# add defines
    add_definitions(${CommonFlags} -g)
endif(CMAKE_BUILD_TYPE STREQUAL Debug)

# Devel - Build
if(CMAKE_BUILD_TYPE STREQUAL Devel)
	# add defines
	add_definitions(${CommonFlags} ${OptimizationFlags})
endif(CMAKE_BUILD_TYPE STREQUAL Devel)

# Release - Build
if(CMAKE_BUILD_TYPE STREQUAL Release)
	# add defines
	add_definitions(${CommonFlags} ${OptimizationFlags})
endif(CMAKE_BUILD_TYPE STREQUAL Release)

# zerogs sources
set(zerogsSources
	GifTransfer.cpp
	glprocs.c
	GSmain.cpp
	GLWinX11.cpp
	Mem.cpp
	memcpy_amd.cpp
	rasterfont.cpp
	Regs.cpp
	targets.cpp
	x86.cpp
	zerogs.cpp
	zpipe.cpp)

# zerogs headers
set(zerogsHeaders
	common.h
	GifTransfer.h
	glprocs.h
	GS.h
	Mem.h
	rasterfont.h
	Regs.h
	targets.h
	x86.h
	zerogs.h
	zerogsmath.h
	zpipe.h)

# zerogs S sources
set(zerogsSSources
	x86-32.S)

# zerogs shader sources
set(zerogsShaderSources
	ctx0/ps2hw_ctx.fx
	ctx1/ps2hw_ctx.fx)

# zerogs Linux sources
set(zerogsLinuxSources
#	Linux/callbacks.c
	Linux/Conf.cpp
	Linux/interface.c
	Linux/Linux.cpp
	Linux/support.c)

# zerogs Linux headers
set(zerogsLinuxHeaders
	Linux/callbacks.h
	Linux/interface.h
	Linux/Linux.h
	Linux/support.h)

# zerogs Windows sources
set(zerogsWindowsSources
	Win32/Conf.cpp
	Win32/Win32.cpp
	Win32/zerogs.def
	Win32/zerogs.rc)

# zerogs Windows headers
set(zerogsWindowsHeaders
	aviUtil.h
	jcofig.h
	jmorecfg.h
	jpeglib.h
	resource.h
	resrc1.h
	Win32.h)

# change language of .S-files to c++
set_source_files_properties(${zerogsSSources} PROPERTIES LANGUAGE CXX)

# add additional include directories
include_directories(.)

# add library
add_library(${Output} SHARED
			${zerogsSources}
			${zerogsHeaders}
			${zerogsSSources}
			${zerogsShaderSources}
			${zerogsLinuxSources}
			${zerogsLinuxHeaders})

# set output directory
set_target_properties(${Output} PROPERTIES
					  LIBRARY_OUTPUT_DIRECTORY ${PROJECT_SOURCE_DIR}/bin/plugins)

# copy ps2hw.dat to plugins folder
add_custom_command(TARGET ${Output} POST_BUILD
				   COMMAND ${CMAKE_COMMAND} -E copy ${PROJECT_SOURCE_DIR}/plugins/zerogs/opengl/Win32/ps2hw.dat ${PROJECT_SOURCE_DIR}/bin/plugins)

# link target with Cg
target_link_libraries(${Output} ${CG_LIBRARIES})

# link target with glew
target_link_libraries(${Output} ${GLEW_LIBRARY})

# link target with opengl
target_link_libraries(${Output} ${OPENGL_LIBRARIES})

# link target with X11
target_link_libraries(${Output} ${X11_LIBRARIES})

# link target with X11 videomod
target_link_libraries(${Output} ${X11_Xxf86vm_LIB})

# User flags options
if(NOT USER_CMAKE_LD_FLAGS STREQUAL "")
    target_link_libraries(${Output} "${USER_CMAKE_LD_FLAGS}")
endif(NOT USER_CMAKE_LD_FLAGS STREQUAL "")
