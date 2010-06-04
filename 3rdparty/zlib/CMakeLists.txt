# zlib library

# library name
set(zlibName zlib)

set(CommonFlags
    -W
    -m32
    )

# Debug - Build
if(CMAKE_BUILD_TYPE STREQUAL Debug)
	# add defines
	add_definitions(${CommonFlags} -g -O0)
endif(CMAKE_BUILD_TYPE STREQUAL Debug)

# Devel - Build
if(CMAKE_BUILD_TYPE STREQUAL Devel)
	# add defines
	add_definitions(${CommonFlags} -O2)
endif(CMAKE_BUILD_TYPE STREQUAL Devel)

# Release - Build
if(CMAKE_BUILD_TYPE STREQUAL Release)
	# add defines
	add_definitions(${CommonFlags} -fexpensive-optimizations -O3)
endif(CMAKE_BUILD_TYPE STREQUAL Release)

# variable with all sources of this library
set(zlibSources
compress.c
deflate.c
gzwrite.c
trees.c
crc32.c
gzlib.c
infback.c
inftrees.c
adler32.c
gzclose.c
gzread.c
inffast.c
inflate.c
uncompr.c
zutil.c)

# variable with all headers of this library
set(zlibHeaders
gzguts.h
inffast.h
inflate.h
zconf.h
zlib.h
zutil.h
deflate.h
inffixed.h
trees.h
crc32.h
inftrees.h )

# add library
add_library(${zlibName} STATIC ${zlibSources} ${zlibHeaders})

# Force the linker into 32 bits mode
target_link_libraries(${zlibName} -m32)

