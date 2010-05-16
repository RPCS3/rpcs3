# GSnull Plugin

# plugin name
set(GSnullName GSnull)

# Debug - Build
if(CMAKE_BUILD_TYPE STREQUAL Debug)
	# add defines
	add_definitions(-Wall -fPIC -m32 -msse2 -g)
endif(CMAKE_BUILD_TYPE STREQUAL Debug)

# Devel - Build
if(CMAKE_BUILD_TYPE STREQUAL Devel)
	# add defines
	add_definitions(-Wall -fPIC -m32 -msse2 -O2)
endif(CMAKE_BUILD_TYPE STREQUAL Devel)

# Release - Build
if(CMAKE_BUILD_TYPE STREQUAL Release)
	# add defines
	add_definitions(-Wall -fPIC -m32 -O2 -msse2)
endif(CMAKE_BUILD_TYPE STREQUAL Release)

# GSnull sources
set(GSnullSources
	GifTransfer.cpp
	GS.cpp)
	
# GSnull headers
set(GSnullHeaders
	GifTransfer.h
	GS.h
	Registers.h)

# GSnull Linux sources
set(GSnullLinuxSources
#	Linux/callbacks.c
	Linux/Config.cpp
	Linux/GSLinux.cpp
	Linux/interface.c
	Linux/Linux.cpp
	Linux/support.c)

# GSnull Linux headers
set(GSnullLinuxHeaders
	Linux/callbacks.h
	Linux/Config.h
	Linux/GSLinux.h
	Linux/interface.h
	Linux/Linux.h
	Linux/support.h)

# GSnull null sources
set(GSnullnullSources
	null/GSnull.cpp)
	
# GSnull null headers
set(GSnullnullHeaders
	null/GSnull.h)

# GSnull Windows sources
set(GSnullWindowsSources
	Windows/Config.cpp
	Windows/GS.def
	Windows/GS.rc
	Windows/GSwin.cpp
	Windows/Win32.cpp)

# GSnull Windows headers
set(GSnullWindowsHeaders
	)

# add additional include directories
include_directories(.)

# add library
add_library(${GSnullName} SHARED 
			${GSnullSources}
			${GSnullHeaders}
			${GSnullnullSources}
			${GSnullnullHeaders}
			${GSnullLinuxSources}
			${GSnullLinuxHeaders})
 
# Force the linker into 32 bits mode
target_link_libraries(${GSnullName} -m32)

# Linker strip option
if (CMAKE_BUILD_STRIP)
    target_link_libraries(${GSnullName} -s)
endif (CMAKE_BUILD_STRIP)

# set output directory
set_target_properties(${GSnullName} PROPERTIES
					  LIBRARY_OUTPUT_DIRECTORY ${PROJECT_SOURCE_DIR}/bin/plugins)

