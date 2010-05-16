# dev9null Plugin

# plugin name
set(dev9nullName dev9null)

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

# dev9null sources
set(dev9nullSources
	Config.cpp
	DEV9.cpp)
	
# dev9null headers
set(dev9nullHeaders
	Config.h
	DEV9.h)

# dev9null Linux sources
set(dev9nullLinuxSources
)

# dev9null Linux headers
set(dev9nullLinuxHeaders
)

# dev9null Windows sources
set(dev9nullWindowsSources
	Windows/dev9null.def)

# dev9null Windows headers
set(dev9nullWindowsHeaders
	)

# additional include directories
include_directories(.)

# add library
add_library(${dev9nullName} SHARED 
			${dev9nullSources}
			${dev9nullHeaders}
			${dev9nullLinuxSources}
			${dev9nullLinuxHeaders})
 
# Linker strip option
if (CMAKE_BUILD_STRIP)
    target_link_libraries(${dev9nullName} -s)
endif (CMAKE_BUILD_STRIP)

# set output directory
set_target_properties(${dev9nullName} PROPERTIES
					  LIBRARY_OUTPUT_DIRECTORY ${PROJECT_SOURCE_DIR}/bin/plugins)

