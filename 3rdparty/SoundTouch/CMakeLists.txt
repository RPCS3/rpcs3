# SoundTouch library

# library name
set(SoundTouchName SoundTouch)

# variable with all sources of this library
set(SoundTouchSources
	AAFilter.cpp
	FIFOSampleBuffer.cpp
	FIRFilter.cpp
	RateTransposer.cpp
	SoundTouch.cpp
	TDStretch.cpp
	WavFile.cpp
	cpu_detect_x86_gcc.cpp
	mmx_optimized.cpp
	sse_optimized.cpp)

# variable with all headers of this library
set(SoundTouchHeaders
	AAFilter.h
	BPMDetect.h
	FIFOSampleBuffer.h
	FIFOSamplePipe.h
	FIRFilter.h
	RateTransposer.h
	SITypes.h
	SoundTouch.h
	TDStretch.h
	WavFile.h
	cpu_detect.h)

# add library
add_library(${SoundTouchName} STATIC ${SoundTouchSources})

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

