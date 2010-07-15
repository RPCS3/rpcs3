# Check that people use the good file
if(NOT TOP_CMAKE_WAS_SOURCED)
    message(FATAL_ERROR "
    You did not 'cmake' the good CMakeLists.txt file. Use the one in the top dir.
    It is advice to delete all wrongly generated cmake stuff => CMakeFiles & CMakeCache.txt")
endif(NOT TOP_CMAKE_WAS_SOURCED)


# plugin name
set(dev9nullName dev9null)

set(CommonFlags
    -Wall
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

# additional include directories
include_directories(.)

# add library
add_library(${dev9nullName} SHARED
			${dev9nullSources}
			${dev9nullHeaders}
			${dev9nullLinuxSources}
			${dev9nullLinuxHeaders})

# set output directory
set_target_properties(${dev9nullName} PROPERTIES
					  LIBRARY_OUTPUT_DIRECTORY ${PROJECT_SOURCE_DIR}/bin/plugins)
