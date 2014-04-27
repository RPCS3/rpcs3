# Try to find SoundTouch
# Once done, this will define
#
# SOUNDTOUCH_FOUND - system has SoundTouch
# SOUNDTOUCH_INCLUDE_DIR - the SoundTouch include directories
# SOUNDTOUCH_LIBRARIES - link these to use SoundTouch

if(SOUNDTOUCH_INCLUDE_DIR AND SOUNDTOUCH_LIBRARIES)
    set(SOUNDTOUCH_FIND_QUIETLY TRUE)
endif(SOUNDTOUCH_INCLUDE_DIR AND SOUNDTOUCH_LIBRARIES)

# include dir
#find_path(SOUNDTOUCH_INCLUDE_DIR SoundTouch.h
#    /usr/include/soundtouch
#    /usr/local/include/soundtouch
#    )
find_path(SOUNDTOUCH_INCLUDE_DIR soundtouch/SoundTouch.h)

# finally the library itself
find_library(libSoundTouch NAMES SoundTouch)
set(SOUNDTOUCH_LIBRARIES ${libSoundTouch})

# handle the QUIETLY and REQUIRED arguments and set SOUNDTOUCH_FOUND to TRUE if 
# all listed variables are TRUE
include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(SoundTouch DEFAULT_MSG SOUNDTOUCH_LIBRARIES SOUNDTOUCH_INCLUDE_DIR)

mark_as_advanced(SOUNDTOUCH_LIBRARIES SOUNDTOUCH_INCLUDE_DIR)

