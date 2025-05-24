# - Try to find ffmpeg libraries (libavcodec, libavformat, libavutil, libswresample and libswscale)
# Once done this will define
#
#  FFMPEG_FOUND - system has ffmpeg or libav
#  FFMPEG_INCLUDE_DIR - the ffmpeg include directory
#  FFMPEG_LIBRARIES - Link these to use ffmpeg
#  FFMPEG_LIBAVCODEC
#  FFMPEG_LIBAVFORMAT
#  FFMPEG_LIBAVUTIL
#  FFMPEG_LIBSWSCALE
#  FFMPEG_LIBSWRESAMPLE
#
#  Copyright (c) 2008 Andreas Schneider <mail@cynapses.org>
#  Modified for other libraries by Lasse Kärkkäinen <tronic>
#  Modified for Hedgewars by Stepik777
#
#  Redistribution and use is allowed according to the terms of the New
#  BSD license.
#

if (FFMPEG_LIBRARIES AND FFMPEG_INCLUDE_DIR)
  # in cache already
  set(FFMPEG_FOUND TRUE)
else ()
  # use pkg-config to get the directories and then use these values
  # in the FIND_PATH() and FIND_LIBRARY() calls
  find_package(PkgConfig)
  if (PKG_CONFIG_FOUND)
    pkg_check_modules(_FFMPEG_AVCODEC libavcodec)
    pkg_check_modules(_FFMPEG_AVFORMAT libavformat)
    pkg_check_modules(_FFMPEG_AVUTIL libavutil)
    pkg_check_modules(_FFMPEG_SWSCALE libswscale)
    pkg_check_modules(_FFMPEG_SWRESAMPLE libswresample)
  endif ()

  find_path(FFMPEG_AVCODEC_INCLUDE_DIR
    NAMES libavcodec/avcodec.h
    PATHS ${_FFMPEG_AVCODEC_INCLUDE_DIRS} /usr/include /usr/local/include /opt/local/include /sw/include
    PATH_SUFFIXES ffmpeg libav
  )

  find_library(FFMPEG_LIBAVCODEC
    NAMES avcodec
    PATHS ${_FFMPEG_AVCODEC_LIBRARY_DIRS} /usr/lib /usr/local/lib /opt/local/lib /sw/lib
  )

  find_library(FFMPEG_LIBAVFORMAT
    NAMES avformat
    PATHS ${_FFMPEG_AVFORMAT_LIBRARY_DIRS} /usr/lib /usr/local/lib /opt/local/lib /sw/lib
  )

  find_library(FFMPEG_LIBAVUTIL
    NAMES avutil
    PATHS ${_FFMPEG_AVUTIL_LIBRARY_DIRS} /usr/lib /usr/local/lib /opt/local/lib /sw/lib
  )

  find_library(FFMPEG_LIBSWSCALE
    NAMES swscale
    PATHS ${_FFMPEG_SWSCALE_LIBRARY_DIRS} /usr/lib /usr/local/lib /opt/local/lib /sw/lib
  )

  find_library(FFMPEG_LIBSWRESAMPLE
    NAMES swresample
    PATHS ${_FFMPEG_SWRESAMPLE_LIBRARY_DIRS} /usr/lib /usr/local/lib /opt/local/lib /sw/lib
  )

  find_package_handle_standard_args(FFMPEG
    DEFAULT_MSG
    FFMPEG_LIBAVCODEC FFMPEG_LIBAVFORMAT FFMPEG_LIBSWSCALE FFMPEG_LIBSWRESAMPLE
  )

  if (FFMPEG_FOUND)
    set(FFMPEG_INCLUDE_DIR ${FFMPEG_AVCODEC_INCLUDE_DIR})

    set(FFMPEG_LIBRARIES
      ${FFMPEG_LIBAVCODEC}
      ${FFMPEG_LIBAVFORMAT}
      ${FFMPEG_LIBAVUTIL}
      ${FFMPEG_LIBSWSCALE}
      ${FFMPEG_LIBSWRESAMPLE}
    )

  endif ()

endif ()
