# bin2cpp tool

# executable name
set(bin2cppName bin2cpp)

# variable with all sources of this executable
set(bin2cppSources
	bin2cpp.cpp)

set(bin2cppHeaders
	)

# add executable
add_executable(${bin2cppName} ${bin2cppSources} ${bin2cppHeaders})

# Debug - Build
if(CMAKE_BUILD_TYPE STREQUAL Debug)
	set(CMAKE_C_FLAGS_DEBUG "-O2 -s -Wall -fexceptions")
	set(CMAKE_CXX_FLAGS_DEBUG "-O2 -s -Wall -fexceptions")
	
	# add defines
	add_definitions()
endif(CMAKE_BUILD_TYPE STREQUAL Debug)

# Devel - Build
if(CMAKE_BUILD_TYPE STREQUAL Devel)
	set(CMAKE_C_FLAGS_DEVEL "-O2 -s -Wall -fexceptions")
	set(CMAKE_CXX_FLAGS_DEVEL "-O2 -s -Wall -fexceptions")
	
	# add defines
	add_definitions()
endif(CMAKE_BUILD_TYPE STREQUAL Devel)

# Release - Build
if(CMAKE_BUILD_TYPE STREQUAL Release)
	set(CMAKE_C_FLAGS_RELEASE "-O2 -s -Wall -fexceptions")
	set(CMAKE_CXX_FLAGS_RELEASE "-O2 -s -Wall -fexceptions")
	
	# add defines
	add_definitions()
endif(CMAKE_BUILD_TYPE STREQUAL Release)

# set output directory
set_target_properties(${bin2cppName} PROPERTIES
					  RUNTIME_OUTPUT_DIRECTORY ${PROJECT_SOURCE_DIR}/tools/bin)

