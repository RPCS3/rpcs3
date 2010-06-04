# Try to find PortAudio
# Once done, this will define
#
# PORTAUDIO_FOUND - system has PortAudio
# PORTAUDIO_INCLUDE_DIR - the PortAudio include directories
# PORTAUDIO_LIBRARIES - link these to use PortAudio

if(PORTAUDIO_INCLUDE_DIR AND PORTAUDIO_LIBRARIES)
    set(PORTAUDIO_FIND_QUIETLY TRUE)
endif(PORTAUDIO_INCLUDE_DIR AND PORTAUDIO_LIBRARIES)

# include dir
find_path(PORTAUDIO_INCLUDE_DIR portaudio.h)

# finally the library itself
find_library(libPortAudio NAMES portaudio)
# Seems to run OK without libportaudiocpp so do not pull additional dependency
# find_library(libPortAudioCpp NAMES portaudiocpp)
# set(PORTAUDIO_LIBRARIES ${libPortAudio} ${libPortAudioCpp})
set(PORTAUDIO_LIBRARIES ${libPortAudio})

# handle the QUIETLY and REQUIRED arguments and set PORTAUDIO_FOUND to TRUE if 
# all listed variables are TRUE
include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(PortAudio DEFAULT_MSG PORTAUDIO_LIBRARIES PORTAUDIO_INCLUDE_DIR)

mark_as_advanced(PORTAUDIO_LIBRARIES PORTAUDIO_INCLUDE_DIR)

