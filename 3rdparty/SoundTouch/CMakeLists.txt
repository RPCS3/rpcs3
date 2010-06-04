# SoundTouch library

# library name
set(SoundTouchName SoundTouch)

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

# Force the linker into 32 bits mode
target_link_libraries(${SoundTouchName} -m32)
