# CDVDnull Plugin

# plugin name
set(CDVDnullName CDVDnull)

# Debug - Build
if(CMAKE_BUILD_TYPE STREQUAL Debug)
	# add defines
	add_definitions(-Wall -fPIC -m32 -g)
endif(CMAKE_BUILD_TYPE STREQUAL Debug)

# Devel - Build
if(CMAKE_BUILD_TYPE STREQUAL Devel)
	# add defines
	add_definitions(-Wall -fPIC -m32 -O2)
endif(CMAKE_BUILD_TYPE STREQUAL Devel)

# Release - Build
if(CMAKE_BUILD_TYPE STREQUAL Release)
	# add defines
	add_definitions(-Wall -fPIC -m32 -O2)
endif(CMAKE_BUILD_TYPE STREQUAL Release)

# CDVDnull sources
set(CDVDnullSources
	CDVD.cpp)
	
# CDVDnull headers
set(CDVDnullHeaders
	CDVD.h)

# CDVDnull Linux sources
set(CDVDnullLinuxSources
	Linux/callbacks.c
	Linux/Config.cpp
	Linux/interface.c
	Linux/support.c)

# CDVDnull Linux headers
set(CDVDnullLinuxHeaders
	Linux/callbacks.h
	Linux/Config.h
	Linux/interface.h
	Linux/support.h)

# CDVDnull Windows sources
set(CDVDnullWindowsSources
	Windows/CDVDnull.def
	Windows/plugin.def)

# CDVDnull Windows headers
set(CDVDnullWindowsHeaders
	)

# add additional include directories
include_directories(.)

# add library
add_library(${CDVDnullName} SHARED 
			${CDVDnullSources}
			${CDVDnullHeaders}
			${CDVDnullLinuxSources}
			${CDVDnullLinuxHeaders})
 
# Linker strip option
if (CMAKE_BUILD_STRIP)
    target_link_libraries(${CDVDnullName} -s)
endif (CMAKE_BUILD_STRIP)

# set output directory
set_target_properties(${CDVDnullName} PROPERTIES
					  LIBRARY_OUTPUT_DIRECTORY ${PROJECT_SOURCE_DIR}/bin/plugins)

