# spu2x Plugin

# plugin name
set(spu2xName spu2x)

# Clear default flags
set(CMAKE_C_FLAGS "")
set(CMAKE_CXX_FLAGS "")
set(CMAKE_C_FLAGS_DEBUG "")
set(CMAKE_CXX_FLAGS_DEBUG "")
set(CMAKE_C_FLAGS_DEVEL "")
set(CMAKE_CXX_FLAGS_DEVEL "")
set(CMAKE_C_FLAGS_RELEASE "")
set(CMAKE_CXX_FLAGS_RELEASE "")

# Debug - Build
if(CMAKE_BUILD_TYPE STREQUAL Debug)
	# add defines
	add_definitions(-Wall -fPIC -m32 -msse2 -g)
endif(CMAKE_BUILD_TYPE STREQUAL Debug)

# Devel - Build
if(CMAKE_BUILD_TYPE STREQUAL Devel)
	# add defines
	add_definitions(-Wall -fPIC -m32 -msse2 -DNDEBUG)
endif(CMAKE_BUILD_TYPE STREQUAL Devel)

# Release - Build
if(CMAKE_BUILD_TYPE STREQUAL Release)
	# add defines
	add_definitions(-Wall -fPIC -m32 -msse2 -DNDEBUG)
endif(CMAKE_BUILD_TYPE STREQUAL Release)

# spu2x sources
set(spu2xSources
    ADSR.cpp
    ConvertUTF.cpp
    Debug.cpp
    Decoder.cpp
    Dma.cpp
    iconvert.cpp
    Lowpass.cpp
    Mixer.cpp
    PrecompiledHeader.cpp
    PS2E-spu2.cpp
    ReadInput.cpp
    RegLog.cpp
    RegTable.cpp
    Reverb.cpp
    SndOut.cpp
    SndOut_Portaudio.cpp
    spu2freeze.cpp
    Spu2replay.cpp
    spu2sys.cpp
    Timestretcher.cpp
    utf8.cpp
    Wavedump_wav.cpp)

# spu2x headers
set(spu2xHeaders
    Config.h
    ConvertUTF.h
    Debug.h
    defs.h
    Dma.h
    DPLII.h
    Global.h
    Lowpass.h
    Mixer.h
    PS2E-spu2.h
    regs.h
    SndOut.h
    spdif.h
    Spu2replay.h
    utf8.h)


# spu2x Linux sources
set(spu2xLinuxSources
    Linux/AboutBox.cpp
    Linux/Alsa.cpp
    Linux/CfgHelpers.cpp
    Linux/Config.cpp
    Linux/ConfigDebug.cpp
    Linux/ConfigSoundTouch.cpp
    Linux/Dialogs.cpp)

# spu2x Linux headers
set(spu2xLinuxHeaders
    Linux/Alsa.h
    Linux/Config.h
    Linux/Dialogs.h)

# add additional include directories
include_directories(.
					Linux)

# add library
add_library(${spu2xName} SHARED
			${spu2xSources}
			${spu2xHeaders}
			${spu2xLinuxSources}
			${spu2xLinuxHeaders})


# set output directory
set_target_properties(${spu2xName} PROPERTIES
					  LIBRARY_OUTPUT_DIRECTORY ${PROJECT_SOURCE_DIR}/bin/plugins)

# link target with project internal libraries
target_link_libraries(${spu2xName} Utilities x86emitter Utilities)

# link target with ALSA
target_link_libraries(${spu2xName} ${ALSA_LIBRARIES})

# # link target with PortAudio
if(PORTAUDIO_FOUND)
    target_link_libraries(${spu2xName} ${PORTAUDIO_LIBRARIES})
endif(PORTAUDIO_FOUND)

# link target with SoundTouch
if(projectSoundTouch)
	target_link_libraries(${spu2xName} SoundTouch)
else(projectSoundTouch)
	target_link_libraries(${spu2xName} ${SOUNDTOUCH_LIBRARIES})
endif(projectSoundTouch)

# link target with A52
target_link_libraries(${spu2xName} a52)

# Force the linker into 32 bits mode
target_link_libraries(${spu2xName} -m32)

# Linker strip option
if (CMAKE_BUILD_STRIP)
    target_link_libraries(${spu2xName} -s)
endif (CMAKE_BUILD_STRIP)
