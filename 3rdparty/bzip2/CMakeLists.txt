# bzip2 library

# library name
set(bzip2Name bzip2)

set(CommonFlags
    -m32
    -march=athlon-xp
    -march=prescott
    )

# Debug - Build
if(CMAKE_BUILD_TYPE STREQUAL Debug)
	# add defines
	add_definitions(${CommonFlags} -g -O0)
endif(CMAKE_BUILD_TYPE STREQUAL Debug)

# Devel - Build
if(CMAKE_BUILD_TYPE STREQUAL Devel)
	# add defines
	add_definitions(${CommonFlags} -O1 -g -W)
endif(CMAKE_BUILD_TYPE STREQUAL Devel)

# Release - Build
if(CMAKE_BUILD_TYPE STREQUAL Release)
	# add defines
	add_definitions(${CommonFlags} -fexpensive-optimizations -O3 -Os -W)
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
add_library(${bzip2Name} STATIC ${bzip2Sources} ${bzip2Headers})

# Force the linker into 32 bits mode
target_link_libraries(${bzip2Name} -m32)
