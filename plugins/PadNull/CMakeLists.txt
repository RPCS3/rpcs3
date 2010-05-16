# PadNull Plugin

# plugin name
set(PadNullName PadNull)

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

# PadNull sources
set(PadNullSources
	Pad.cpp)
	
# PadNull headers
set(PadNullHeaders
	Pad.h)

# PadNull Linux sources
set(PadNullLinuxSources
#	Linllux/callbacks.c
	Linux/Config.cpp
	Linux/interface.c
	Linux/PadLinux.cpp
	Linux/support.c)

# PadNull Linux headers
set(PadNullLinuxHeaders
	Linux/callbacks.h
	Linux/Config.h
	Linux/interface.h
	Linux/PadLinux.h
	Linux/support.h)

# PadNull Windows sources
set(PadNullWindowsSources
	Windows/Config.cpp
	Windows/PadNull.rc
	Windows/PadWin.cpp
	Windows/Win32.cpp)

# PadNull Windows headers
set(PadNullWindowsHeaders
	Windows/PadWin.h
	Windows/resource.h)

# add additional include directories
include_directories(.
					Linux)

# add library
add_library(${PadNullName} SHARED 
			${PadNullSources}
			${PadNullHeaders}
			${PadNullLinuxSources}
			${PadNullLinuxHeaders})
 
# Linker strip option
if (CMAKE_BUILD_STRIP)
    target_link_libraries(${PadNullName} -s)
endif (CMAKE_BUILD_STRIP)

# set output directory
set_target_properties(${PadNullName} PROPERTIES
					  LIBRARY_OUTPUT_DIRECTORY ${PROJECT_SOURCE_DIR}/bin/plugins)

