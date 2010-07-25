# SoundTouch library

# library name
set(Output pcsx2_SoundTouch)

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
	add_definitions(${CommonFlags} ${OptimizationFlags})
endif(CMAKE_BUILD_TYPE STREQUAL Devel)

# Release - Build
if(CMAKE_BUILD_TYPE STREQUAL Release)
	# add defines
	add_definitions(${CommonFlags} ${OptimizationFlags})
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
add_library(${Output} STATIC ${SoundTouchSources} ${SoundTouchHeaders})

# User flags options
if(NOT USER_CMAKE_LD_FLAGS STREQUAL "")
    target_link_libraries(${Output} "${USER_CMAKE_LD_FLAGS}")
endif(NOT USER_CMAKE_LD_FLAGS STREQUAL "")
