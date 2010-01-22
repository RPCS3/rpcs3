# FWnull Plugin

# plugin name
set(FWnullName FWnull)

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
	add_definitions(-Wall -fPIC -m32 -O2 -s)
endif(CMAKE_BUILD_TYPE STREQUAL Release)

# FWnull sources
set(FWnullSources
	Config.cpp
	FW.cpp)
	
# FWnull headers
set(FWnullHeaders
	Config.h
	FW.h)

# FWnull Linux sources
set(FWnullLinuxSources
)

# FWnull Linux headers
set(FWnullLinuxHeaders
)

# FWnull Windows sources
set(FWnullWindowsSources
	Windows/Config.cpp
	Windows/FireWireNull.def
	Windows/FireWireNul.rc
	Windows/Win32.cpp)

# FWnull Windows headers
set(FWnullWindowsHeaders
	Windows/resource.h)

# add additional include directories
include_directories(.)

# add library
add_library(${FWnullName} SHARED 
			${FWnullSources}
			${FWnullHeaders}
			${FWnullLinuxSources}
			${FWnullLinuxHeaders})

# set output directory
set_target_properties(${FWnullName} PROPERTIES
					  LIBRARY_OUTPUT_DIRECTORY ${PROJECT_SOURCE_DIR}/bin/plugins)

