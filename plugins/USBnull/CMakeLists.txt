# USBnull Plugin

# plugin name
set(USBnullName USBnull)

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

# USBnull sources
set(USBnullSources
	Config.cpp
	USB.cpp)
	
# USBnull headers
set(USBnullHeaders
	Config.h
	USB.h)

# USBnull Linux sources
set(USBnullLinuxSources
)

# USBnull Linux headers
set(USBnullLinuxHeaders
)

# USBnull Windows sources
set(USBnullWindowsSources
	Windows/Config.cpp
	Windows/USBnull.def
	Windows/USBnull.rc
	Windows/Win32.cpp)

# USBnull Windows headers
set(USBnullWindowsHeaders
	Windows/resource.h)

# additonal include directories
include_directories(.)

# add library
add_library(${USBnullName} SHARED 
			${USBnullSources}
			${USBnullHeaders}
			${USBnullLinuxSources}
			${USBnullLinuxHeaders})

# set output directory
set_target_properties(${USBnullName} PROPERTIES
					  LIBRARY_OUTPUT_DIRECTORY ${PROJECT_SOURCE_DIR}/bin/plugins)

