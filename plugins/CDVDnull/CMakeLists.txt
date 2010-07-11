# Check that people use the good file
if(NOT TOP_CMAKE_WAS_SOURCED)
    message(FATAL_ERROR "
    You did not 'cmake' the good CMakeLists.txt file. Use the one in the top dir.
    It is advice to delete all wrongly generated cmake stuff => CMakeFiles & CMakeCache.txt")
endif(NOT TOP_CMAKE_WAS_SOURCED)


# plugin name
set(CDVDnullName CDVDnull)

set(CommonFlags
    -Wall
    -m32
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

# CDVDnull sources
set(CDVDnullSources
	CDVD.cpp)

# CDVDnull headers
set(CDVDnullHeaders
	CDVD.h)

# # CDVDnull Linux sources
# set(CDVDnullLinuxSources
# 	Linux/callbacks.c
# 	Linux/Config.cpp
# 	Linux/interface.c
# 	Linux/support.c)

# # CDVDnull Linux headers
# set(CDVDnullLinuxHeaders
# 	Linux/callbacks.h
# 	Linux/Config.h
# 	Linux/interface.h
# 	Linux/support.h)

# CDVDnull Windows sources
set(CDVDnullWindowsSources
	Windows/CDVDnull.def
	Windows/plugin.def)

# CDVDnull Windows headers
set(CDVDnullWindowsHeaders
	)

# add additional include directories
include_directories(.)

# add library
add_library(${CDVDnullName} SHARED
			${CDVDnullSources}
			${CDVDnullHeaders}
            #${CDVDnullLinuxSources}
            #${CDVDnullLinuxHeaders}
            )

# Force the linker into 32 bits mode
target_link_libraries(${CDVDnullName} -m32)

# Linker strip option
if (CMAKE_BUILD_STRIP)
    target_link_libraries(${CDVDnullName} -s)
endif (CMAKE_BUILD_STRIP)

# set output directory
set_target_properties(${CDVDnullName} PROPERTIES
					  LIBRARY_OUTPUT_DIRECTORY ${PROJECT_SOURCE_DIR}/bin/plugins)

