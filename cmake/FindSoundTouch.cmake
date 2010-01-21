# Try to find SoundTouch
# Once done, this will define
#
# SOUNDTOUCH_FOUND - system has SoundTouch
# SOUNDTOUCH_INCLUDE_DIR - the SoundTouch include directories
# SOUNDTOUCH_LIBRARIES - link these to use SoundTouch

if(SoundTouch_INCLUDE_DIR AND SoundTouch_LIBRARIES)
    set(SoundTouch_FIND_QUIETLY TRUE)
endif(SoundTouch_INCLUDE_DIR AND SoundTouch_LIBRARIES)

# include dir
find_path(SoundTouch_INCLUDE_DIR soundtouch/SoundTouch.h)

# finally the library itself
find_library(tmpLibBPM NAMES BPM SoundTouch)
find_library(tmpLibST NAMES SoundTouch)
set(SoundTouch_LIBRARIES ${tmpLibBPM} ${tmpLibST})

# handle the QUIETLY and REQUIRED arguments and set SOUNDTOUCH_FOUND to TRUE if 
# all listed variables are TRUE
include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(SoundTouch DEFAULT_MSG SoundTouch_LIBRARIES SoundTouch_INCLUDE_DIR)

mark_as_advanced(SoundTouch_LIBRARIES SoundTouch_INCLUDE_DIR)

