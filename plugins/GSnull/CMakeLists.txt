# GSnull Plugin

# plugin name
set(GSnullName GSnull)

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

# add library
add_library(${GSnullName} SHARED 
			${GSnullSources}
			${GSnullHeaders}
			${GSnullnullSources}
			${GSnullnullHeaders}
			${GSnullLinuxSources}
			${GSnullLinuxHeaders})

# set output directory
set_target_properties(${GSnullName} PROPERTIES
					  LIBRARY_OUTPUT_DIRECTORY ${PROJECT_SOURCE_DIR}/bin/plugins)

# Debug - Build
if(CMAKE_BUILD_TYPE STREQUAL Debug)
	set(CMAKE_C_FLAGS_DEBUG "-Wall -fPIC -m32 -msse2 -g")
	set(CMAKE_CXX_FLAGS_DEBUG "-Wall -fPIC -m32 -msse2 -g")
	
	# add defines
	add_definitions()
endif(CMAKE_BUILD_TYPE STREQUAL Debug)

# Devel - Build
if(CMAKE_BUILD_TYPE STREQUAL Devel)
	set(CMAKE_C_FLAGS_DEVEL "-Wall -fPIC -m32 -msse2 -O2")
	set(CMAKE_CXX_FLAGS_DEVEL "-Wall -fPIC -m32 -msse2 -O2")
	
	# add defines
	add_definitions()
endif(CMAKE_BUILD_TYPE STREQUAL Devel)

# Release - Build
if(CMAKE_BUILD_TYPE STREQUAL Release)
	set(CMAKE_C_FLAGS_RELEASE "-Wall -fPIC -m32 -O2 -msse2 -s")
	set(CMAKE_CXX_FLAGS_RELEASE "-Wall -fPIC -m32 -O2 -msse2 -s")
	
	# add defines
	add_definitions()
endif(CMAKE_BUILD_TYPE STREQUAL Release)

# link target with wx
target_link_libraries(${GSnullName})

# add additional include directories
include_directories(.
					../../common/include)

