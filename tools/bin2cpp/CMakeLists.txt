# bin2cpp tool

# executable name
set(bin2cppName bin2cpp)

# Debug - Build
if(CMAKE_BUILD_TYPE STREQUAL Debug)
	# add defines
	add_definitions(-O2 -s -Wall -fexceptions)
endif(CMAKE_BUILD_TYPE STREQUAL Debug)

# Devel - Build
if(CMAKE_BUILD_TYPE STREQUAL Devel)
	# add defines
	add_definitions(-O2 -s -Wall -fexceptions)
endif(CMAKE_BUILD_TYPE STREQUAL Devel)

# Release - Build
if(CMAKE_BUILD_TYPE STREQUAL Release)
	# add defines
	add_definitions(-O2 -s -Wall -fexceptions)
endif(CMAKE_BUILD_TYPE STREQUAL Release)

# variable with all sources of this executable
set(bin2cppSources
	bin2cpp.cpp)

set(bin2cppHeaders
	)

# add executable
add_executable(${bin2cppName} ${bin2cppSources} ${bin2cppHeaders})

# set output directory
# set_target_properties(${bin2cppName} PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${PROJECT_SOURCE_DIR}/tools/bin)

