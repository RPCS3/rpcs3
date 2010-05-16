# a52 library

# library name
set(a52Name a52)

# Debug - Build
if(CMAKE_BUILD_TYPE STREQUAL Debug)
	# add defines
	add_definitions(-g -Wall)
endif(CMAKE_BUILD_TYPE STREQUAL Debug)

# Devel - Build
if(CMAKE_BUILD_TYPE STREQUAL Devel)
	# add defines
	add_definitions(-O2 -Wall -g -Os)
endif(CMAKE_BUILD_TYPE STREQUAL Devel)

# Release - Build
if(CMAKE_BUILD_TYPE STREQUAL Release)
	# add defines
	add_definitions(-O2 -Wall -g -Os)
endif(CMAKE_BUILD_TYPE STREQUAL Release)

# variable with all sources of this library
set(a52Sources
	bit_allocate.c
    bitstream.c
    downmix.c
    imdct.c
    parse.c)

# variable with all headers of this library
set(a52Headers
	a52.h
	a52_internal.h
    attributes.h
    bitstream.h
    config.h
    inttypes.h
    mm_accel.h
    tables.h
    tendra.h)

# add library
add_library(${a52Name} STATIC ${a52Sources} ${a52Headers}) 

# Force the linker into 32 bits mode
target_link_libraries(${a52Name} -m32)
