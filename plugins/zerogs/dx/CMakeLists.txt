# zerogs Plugin

# plugin name
set(zerogsName zerogs)

# zerogs sources
set(zerogsSources
	CDVD.cpp)
	
# zerogs headers
set(zerogsHeaders
	CDVD.h)

# zerogs Linux sources
set(zerogsLinuxSources
	Linux/callbacks.c
	Linux/Config.cpp
	Linux/interface.c
	Linux/support.c)

# zerogs Linux headers
set(zerogsLinuxHeaders
	Linux/callbacks.h
	Linux/Config.h
	Linux/interface.h
	Linux/support.h)

# zerogs Windows sources
set(zerogsWindowsSources
	Windows/zerogs.def
	Windows/plugin.def)

# zerogs Windows headers
set(zerogsWindowsHeaders
	)

# add library
add_library(${zerogsName} SHARED 
			${zerogsSources}
			${zerogsHeaders}
			${zerogsLinuxSources}
			${zerogsLinuxHeaders})

# set output directory
set_target_properties(${zerogsName} PROPERTIES
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
target_link_libraries(${zerogsName})

# add additional include directories
include_directories(.
					../../common/include)

