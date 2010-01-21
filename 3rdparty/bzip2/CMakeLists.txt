# bzip2 library

# library name
set(bzip2Name bzip2)

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

# Debug - Build
if(CMAKE_BUILD_TYPE STREQUAL Debug)
	set(CMAKE_C_FLAGS_DEBUG "-g -O0 -march=athlon-xp -march=prescott")
	set(CMAKE_CXX_FLAGS_DEBUG "-g -O0 -march=athlon-xp -march=prescott")
endif(CMAKE_BUILD_TYPE STREQUAL Debug)

# Devel - Build
if(CMAKE_BUILD_TYPE STREQUAL Devel)
	set(CMAKE_C_FLAGS_DEVEL "-O1 -g -W -march=athlon-xp -march=prescott")
	set(CMAKE_CXX_FLAGS_DEVEL "-O1 -g -W -march=athlon-xp -march=prescott")
endif(CMAKE_BUILD_TYPE STREQUAL Devel)

# Release - Build
if(CMAKE_BUILD_TYPE STREQUAL Release)
	set(CMAKE_C_FLAGS_RELEASE "-fexpensive-optimizations -O3 -Os -W -march=athlon-xp -march=prescott")
	set(CMAKE_CXX_FLAGS_RELEASE "-fexpensive-optimizations -O3 -Os -W -march=athlon-xp -march=prescott")
endif(CMAKE_BUILD_TYPE STREQUAL Release)


