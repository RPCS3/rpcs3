# SoundTouch library

# library name
set(SoundTouchName SoundTouch)

# Debug - Build
if(CMAKE_BUILD_TYPE STREQUAL Debug)
	# add defines
	add_definitions(-g -O0 -march=athlon-xp -march=prescott -m32)
endif(CMAKE_BUILD_TYPE STREQUAL Debug)

# Devel - Build
if(CMAKE_BUILD_TYPE STREQUAL Devel)
	# add defines
	add_definitions(-O1 -g -W -march=athlon-xp -march=prescott -m32)
endif(CMAKE_BUILD_TYPE STREQUAL Devel)

# Release - Build
if(CMAKE_BUILD_TYPE STREQUAL Release)
	# add defines
	add_definitions(-fexpensive-optimizations -O3 -Os -W -march=athlon-xp -march=prescott -m32)
endif(CMAKE_BUILD_TYPE STREQUAL Release)

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
	STTypes.h
	SoundTouch.h
	TDStretch.h
	WavFile.h
	cpu_detect.h)

# add library
add_library(${SoundTouchName} STATIC ${SoundTouchSources} ${SoundTouchHeaders})

