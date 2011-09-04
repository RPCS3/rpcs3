# Check that people use the good file
if(NOT TOP_CMAKE_WAS_SOURCED)
    message(FATAL_ERROR "
    You did not 'cmake' the good CMakeLists.txt file. Use the one in the top dir.
    It is advice to delete all wrongly generated cmake stuff => CMakeFiles & CMakeCache.txt")
endif(NOT TOP_CMAKE_WAS_SOURCED)


# plugin name
set(Output zerospu2)

set(CommonFlags
    -Wall
    -msse2
    )

set(OptimizationFlags
    -O2
    -DNDEBUG
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

# zerospu2 sources
set(zerospu2Sources
	voices.cpp
    WavFile.cpp
	zerodma.cpp
	zerospu2.cpp
	zeroworker.cpp)

# zerospu2 headers
set(zerospu2Headers
	misc.h
	reg.h
    WavFile.h
	zerodma.h
	zerospu2.h
	zeroworker.h)

# zerospu2 Linux sources
set(zerospu2LinuxSources
	Linux/Linux.cpp
	Targets/Alsa.cpp
	Targets/OSS.cpp
#	Targets/PortAudio.cpp
	Targets/SoundTargets.cpp)

# zerospu2 Linux headers
set(zerospu2LinuxHeaders
	Linux/Linux.h
	Targets/Alsa.h
	Targets/OSS.h
#	Targets/PortAudio.h
	Targets/SoundTargets.h)

# zerospu2 Windows sources
set(zerospu2WindowsSources
	Targets/dsound51.cpp
	Windows/Win32.cpp
	Windows/ZeroSPU2.def
	Windows/ZeroSPU2.rc)

# zerospu2 Windows headers
set(zerospu2WindowsHeaders
	resources.h
	Targets/dsound51.h)

# add additional include directories
include_directories(.
					Linux
					Targets)

# add library
add_library(${Output} SHARED
			${zerospu2Sources}
			${zerospu2Headers}
			${zerospu2LinuxSources}
			${zerospu2LinuxHeaders})

# link target with ALSA
target_link_libraries(${Output} ${ALSA_LIBRARIES})

if(PORTAUDIO_FOUND)
	# link target with PortAudio
	#target_link_libraries(${Output} ${PORTAUDIO_LIBRARIES})
endif(PORTAUDIO_FOUND)

# link target with SoundTouch
target_link_libraries(${Output} ${SOUNDTOUCH_LIBRARIES})

# User flags options
if(NOT USER_CMAKE_LD_FLAGS STREQUAL "")
    target_link_libraries(${Output} "${USER_CMAKE_LD_FLAGS}")
endif(NOT USER_CMAKE_LD_FLAGS STREQUAL "")

if(PACKAGE_MODE)
    install(TARGETS ${Output} DESTINATION ${PLUGIN_DIR})
else(PACKAGE_MODE)
    install(TARGETS ${Output} DESTINATION ${CMAKE_SOURCE_DIR}/bin/plugins)
endif(PACKAGE_MODE)
