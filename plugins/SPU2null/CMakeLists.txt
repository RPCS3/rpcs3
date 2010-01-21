# SPU2null Plugin

# plugin name
set(SPU2nullName SPU2null)

# SPU2null sources
set(SPU2nullSources
	SPU2.cpp)
	
# SPU2null headers
set(SPU2nullHeaders
	SPU2.h)

# SPU2null Linux sources
set(SPU2nullLinuxSources
#	Linux/callbacks.c
	Linux/Config.cpp
	Linux/interface.c
	Linux/support.c)

# SPU2null Linux headers
set(SPU2nullLinuxHeaders
	Linux/callbacks.h
	Linux/Config.h
	Linux/interface.h
	Linux/support.h)

# SPU2null Windows sources
set(SPU2nullWindowsSources
	Windows/Config.cpp
	Windows/SPU2null.def
	Windows/SPU2null.rc
	Windows/Win32.cpp)

# SPU2null Windows headers
set(SPU2nullWindowsHeaders
	Windows/resource.h)

# add library
add_library(${SPU2nullName} SHARED 
			${SPU2nullSources}
			${SPU2nullHeaders}
			${SPU2nullLinuxSources}
			${SPU2nullLinuxHeaders})

# set output directory
set_target_properties(${SPU2nullName} PROPERTIES
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
target_link_libraries(${SPU2nullName})

# add additional include directories
include_directories(.
					../../common/include)

