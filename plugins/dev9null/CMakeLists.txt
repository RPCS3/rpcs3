# dev9null Plugin

# plugin name
set(dev9nullName dev9null)

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

# add library
add_library(${dev9nullName} SHARED 
			${dev9nullSources}
			${dev9nullHeaders}
			${dev9nullLinuxSources}
			${dev9nullLinuxHeaders})

# set output directory
set_target_properties(${dev9nullName} PROPERTIES
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

