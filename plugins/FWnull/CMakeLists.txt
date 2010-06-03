# FWnull Plugin

# plugin name
set(FWnullName FWnull)

set(CommonFlags
    -Wall
    -m32
    -fPIC
    )

set(OptimizationFlags
    -O2
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

# FWnull sources
set(FWnullSources
	Config.cpp
	FW.cpp)

# FWnull headers
set(FWnullHeaders
	Config.h
	FW.h)

# FWnull Linux sources
set(FWnullLinuxSources
)

# FWnull Linux headers
set(FWnullLinuxHeaders
)

# FWnull Windows sources
set(FWnullWindowsSources
	Windows/Config.cpp
	Windows/FireWireNull.def
	Windows/FireWireNul.rc
	Windows/Win32.cpp)

# FWnull Windows headers
set(FWnullWindowsHeaders
	Windows/resource.h)

# add additional include directories
include_directories(.)

# add library
add_library(${FWnullName} SHARED
			${FWnullSources}
			${FWnullHeaders}
			${FWnullLinuxSources}
			${FWnullLinuxHeaders})

# Force the linker into 32 bits mode
target_link_libraries(${FWnullName} -m32)

# Linker strip option
if (CMAKE_BUILD_STRIP)
    target_link_libraries(${FWnullName} -s)
endif (CMAKE_BUILD_STRIP)

# set output directory
set_target_properties(${FWnullName} PROPERTIES
					  LIBRARY_OUTPUT_DIRECTORY ${PROJECT_SOURCE_DIR}/bin/plugins)

