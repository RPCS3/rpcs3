# USBnull Plugin

# plugin name
set(USBnullName USBnull)

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

# add library
add_library(${USBnullName} SHARED 
			${USBnullSources}
			${USBnullHeaders}
			${USBnullLinuxSources}
			${USBnullLinuxHeaders})

# set output directory
set_target_properties(${USBnullName} PROPERTIES
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
target_link_libraries(${USBnullName})

# add additional include directories
include_directories(.
					../../common/include)

