# zerogs Plugin

# plugin name
set(zerogsName zerogs)

include(${PROJECT_SOURCE_DIR}/cmake/FindGlew.cmake)
include(${PROJECT_SOURCE_DIR}/cmake/FindCg.cmake)

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

# add library
add_library(${zerogsName} SHARED 
			${zerogsSources}
			${zerogsHeaders}
			${zerogsSSources}
			${zerogsShaderSources}
			${zerogsLinuxSources}
			${zerogsLinuxHeaders})

# set output directory
set_target_properties(${zerogsName} PROPERTIES
					  LIBRARY_OUTPUT_DIRECTORY ${PROJECT_SOURCE_DIR}/bin/plugins)

# Debug - Build
if(CMAKE_BUILD_TYPE STREQUAL Debug)
	set(CMAKE_C_FLAGS_DEBUG "-Wall -m32 -g")
	set(CMAKE_CXX_FLAGS_DEBUG "-Wall -m32 -g")
	
	# add defines
	add_definitions()
endif(CMAKE_BUILD_TYPE STREQUAL Debug)

# Devel - Build
if(CMAKE_BUILD_TYPE STREQUAL Devel)
	set(CMAKE_C_FLAGS_DEVEL "-Wall -m32 -O2")
	set(CMAKE_CXX_FLAGS_DEVEL "-Wall -m32 -O2")
	
	# add defines
	add_definitions()
endif(CMAKE_BUILD_TYPE STREQUAL Devel)

# Release - Build
if(CMAKE_BUILD_TYPE STREQUAL Release)
	set(CMAKE_C_FLAGS_RELEASE "-Wall -m32 -O2 -s")
	set(CMAKE_CXX_FLAGS_RELEASE "-Wall -m32 -O2 -s")
	
	# add defines
	add_definitions()
endif(CMAKE_BUILD_TYPE STREQUAL Release)

# link target with
target_link_libraries(${zerogsName} ${GLEW_LIBRARY} ${CG_LIBRARIES})

# add additional include directories
include_directories(.
					${GLEW_INCLUDE_PATH}
					${CG_INCLUDE_DIRS}
					../../../common/include)

