# zlib library

# library name
set(zlibName zlib)

# variable with all sources of this library
set(zlibSources
	adler32.c
	compress.c
	crc32.c
	deflate.c
	gzio.c
	infback.c
	inffast.c
	inflate.c
	inftrees.c
	trees.c
	uncompr.c
	zutil.c)

# variable with all headers of this library
set(zlibHeaders
	crc32.h
	deflate.h
	inffast.h
	inffixed.h
	inflate.h
	inftrees.h
	trees.h
	zconf.h
	zlib.h
	zutil.h)

# add library
add_library(${zlibName} STATIC ${zlibSources} ${zlibHeaders})

# Debug - Build
if(CMAKE_BUILD_TYPE STREQUAL Debug)
	set(CMAKE_C_FLAGS_DEBUG "-W -g -O0")
	set(CMAKE_CXX_FLAGS_DEBUG "-W -g -O0")
endif(CMAKE_BUILD_TYPE STREQUAL Debug)

# Devel - Build
if(CMAKE_BUILD_TYPE STREQUAL Devel)
	set(CMAKE_C_FLAGS_DEVEL "-O2 -W")
	set(CMAKE_CXX_FLAGS_DEVEL "-O2 -W")
endif(CMAKE_BUILD_TYPE STREQUAL Devel)

# Release - Build
if(CMAKE_BUILD_TYPE STREQUAL Release)
	set(CMAKE_C_FLAGS_RELEASE "-fexpensive-optimizations -O3 -W")
	set(CMAKE_CXX_FLAGS_RELEASE "-fexpensive-optimizations -O3 -W")
endif(CMAKE_BUILD_TYPE STREQUAL Release)

