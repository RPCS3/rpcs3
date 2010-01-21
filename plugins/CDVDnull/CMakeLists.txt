# CDVDnull Plugin

# plugin name
set(CDVDnullName CDVDnull)

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

# add library
add_library(${CDVDnullName} SHARED 
			${CDVDnullSources}
			${CDVDnullHeaders}
			${CDVDnullLinuxSources}
			${CDVDnullLinuxHeaders}
			)

# set output directory
set_target_properties(${CDVDnullName} PROPERTIES
					  LIBRARY_OUTPUT_DIRECTORY ${PROJECT_SOURCE_DIR}/bin/plugins)

# Debug - Build
if(CMAKE_BUILD_TYPE STREQUAL Debug)
	set(CMAKE_C_FLAGS_DEBUG "-Wall -fPIC -m32 -g")
	set(CMAKE_CXX_FLAGS_DEBUG "-Wall -fPIC -m32 -g")

	# add defines
	add_definitions()
endif(CMAKE_BUILD_TYPE STREQUAL Debug)

# Devel - Build
if(CMAKE_BUILD_TYPE STREQUAL Devel)
	set(CMAKE_C_FLAGS_DEVEL "-Wall -fPIC -m32 -O2")
	set(CMAKE_CXX_FLAGS_DEVEL "-Wall -fPIC -m32 -O2")
	
	# add defines
	add_definitions()
endif(CMAKE_BUILD_TYPE STREQUAL Devel)

# Release - Build
if(CMAKE_BUILD_TYPE STREQUAL Release)
	set(CMAKE_C_FLAGS_RELEASE "-Wall -fPIC -m32 -O2 -s")
	set(CMAKE_CXX_FLAGS_RELEASE "-Wall -fPIC -m32 -O2 -s")
	
	# add defines
	add_definitions()
endif(CMAKE_BUILD_TYPE STREQUAL Release)

# link target with wx
target_link_libraries(${CDVDnullName})

# add additional include directories
include_directories(.
					../../common/include)
