# zerospu2 Plugin

# plugin name
set(zerospu2Name zerospu2)

# zerospu2 sources
set(zerospu2Sources
	voices.cpp
	zerodma.cpp
	zerospu2.cpp)
	
# zerospu2 headers
set(zerospu2Headers
	misc.h
	reg.h
	zerospu2.h)

# zerospu2 Linux sources
set(zerospu2LinuxSources
#	Linux/callbacks.c
	Linux/interface.c
	Linux/Linux.cpp
	Linux/support.c
	Targets/Alsa.cpp
	Targets/OSS.cpp
#	Targets/PortAudio.cpp
	Targets/SoundTargets.cpp)

# zerospu2 Linux headers
set(zerospu2LinuxHeaders
	Linux/callbacks.h
	Linux/interface.h
	Linux/Linux.h
	Linux/support.h
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

# add library
add_library(${zerospu2Name} SHARED 
			${zerospu2Sources}
			${zerospu2Headers}
			${zerospu2LinuxSources}
			${zerospu2LinuxHeaders})

# set output directory
set_target_properties(${zerospu2Name} PROPERTIES
					  LIBRARY_OUTPUT_DIRECTORY ${PROJECT_SOURCE_DIR}/bin/plugins)

# Debug - Build
if(CMAKE_BUILD_TYPE STREQUAL Debug)
	# add defines
	add_definitions(-Wall -fPIC -m32 -msse2 -g)
endif(CMAKE_BUILD_TYPE STREQUAL Debug)

# Devel - Build
if(CMAKE_BUILD_TYPE STREQUAL Devel)
	# add defines
	add_definitions(-Wall -fPIC -m32 -msse2 -O2)
endif(CMAKE_BUILD_TYPE STREQUAL Devel)

# Release - Build
if(CMAKE_BUILD_TYPE STREQUAL Release)
	# add defines
	add_definitions(-Wall -fPIC -m32 -msse2 -O2 -s)
endif(CMAKE_BUILD_TYPE STREQUAL Release)

# link target with ALSA
target_link_libraries(${zerospu2Name} ${ALSA_LIBRARIES})

# link target with SoundTouch
if(localSoundTouch)
	target_link_libraries(${zerospu2Name} SoundTouch)
else(localSoundTouch)
	target_link_libraries(${zerospu2Name} ${SOUNDTOUCH_LIBRARIES})
endif(localSoundTouch)

# add additional include directories
include_directories(.
				./Target
				./Linux
					../../../common/include)
