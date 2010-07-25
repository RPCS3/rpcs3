# bzip2 library

# library name
set(Output pcsx2_bzip2)

set(CommonFlags
    -march=athlon-xp
    -march=prescott
    )

set(OptimizationFlags
    -Os
    -W
    )

# Debug - Build
if(CMAKE_BUILD_TYPE STREQUAL Debug)
	# add defines
	add_definitions(${CommonFlags} -g)
endif(CMAKE_BUILD_TYPE STREQUAL Debug)

# Devel - Build
if(CMAKE_BUILD_TYPE STREQUAL Devel)
	# add defines
	add_definitions(${CommonFlags} ${OptimizationFlags} -g)
endif(CMAKE_BUILD_TYPE STREQUAL Devel)

# Release - Build
if(CMAKE_BUILD_TYPE STREQUAL Release)
	# add defines
	add_definitions(${CommonFlags} ${OptimizationFlags})
endif(CMAKE_BUILD_TYPE STREQUAL Release)

# variable with all sources of this library
set(bzip2Sources
	blocksort.c
	bzlib.c
	compress.c
	crctable.c
	decompress.c
	huffman.c
	randtable.c)

# variable with all headers of this library
set(bzip2Headers
	bzlib.h
	bzlib_private.h)

# add library
add_library(${Output} STATIC ${bzip2Sources} ${bzip2Headers})

# User flags options
if(NOT USER_CMAKE_LD_FLAGS STREQUAL "")
    target_link_libraries(${Output} "${USER_CMAKE_LD_FLAGS}")
endif(NOT USER_CMAKE_LD_FLAGS STREQUAL "")
