# SPU2null Plugin

# plugin name
set(SPU2nullName SPU2null)

set(CommonFlags
    -Wall
    -m32
    -fPIC
    )

set(OptimizationFlags
    -O2
    -DNDEBUG
    )

# Debug - Build
if(CMAKE_BUILD_TYPE STREQUAL Debug)
	# add defines
    add_definitions(${CommonFlags} -g)
endif(CMAKE_BUILD_TYPE STREQUAL Debug)

# Devel - Build
if(CMAKE_BUILD_TYPE STREQUAL Devel)
	# add defines
	add_definitions(${CommonFlags} ${OptimizationFlags})
endif(CMAKE_BUILD_TYPE STREQUAL Devel)

# Release - Build
if(CMAKE_BUILD_TYPE STREQUAL Release)
	# add defines
	add_definitions(${CommonFlags} ${OptimizationFlags})
endif(CMAKE_BUILD_TYPE STREQUAL Release)

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

# add additional include directories
include_directories(.)

# add library
add_library(${SPU2nullName} SHARED
			${SPU2nullSources}
			${SPU2nullHeaders}
			${SPU2nullLinuxSources}
			${SPU2nullLinuxHeaders})


# Force the linker into 32 bits mode
target_link_libraries(${SPU2nullName} -m32)

# Linker strip option
if (CMAKE_BUILD_STRIP)
    target_link_libraries(${SPU2nullName} -s)
endif (CMAKE_BUILD_STRIP)

# set output directory
set_target_properties(${SPU2nullName} PROPERTIES
					  LIBRARY_OUTPUT_DIRECTORY ${PROJECT_SOURCE_DIR}/bin/plugins)

