#-------------------------------------------------------------------------------
#						Search all libraries on the system
#-------------------------------------------------------------------------------
# Do not search Lib in /usr/lib64. Hope it is not overwritten in find_package or others macro
SET(FIND_LIBRARY_USE_LIB64_PATHS FALSE)

# Linux libraries
if(Linux)
    find_package(GTK2 REQUIRED gtk)
    find_package(X11)
    # Manually find Xxf86vm because it is not done in the module...
    FIND_LIBRARY(X11_Xxf86vm_LIB Xxf86vm       ${X11_LIB_SEARCH_PATH})
    MARK_AS_ADVANCED(X11_Xxf86vm_LIB)
endif(Linux)
# Use cmake package to find module
find_package(ALSA)
find_package(BZip2)
find_package(OpenGL)
# Tell cmake that we use SDL as a library and not as an application
set(SDL_BUILDING_LIBRARY TRUE)
find_package(SDL)
find_package(Subversion)
find_package(wxWidgets REQUIRED base core adv)
find_package(ZLIB)
# Use pcsx2 package to find module
include(FindA52)
include(FindCg)
include(FindGlew)
include(FindPortAudio)
include(FindSoundTouch)

# Note for include_directory: The order is important to avoid a mess between include file from your system and the one of pcsx2
# If you include first 3rdparty, all 3rdpary include will have a higer priority...
# If you include first /usr/include, all system include will have a higer priority over the pcsx2 one...
# Current implementation: 
# 1/ include 3rdparty sub-directory that we will used (either request or fallback)
# 2/ include system one
# 3/ include 3rdpary main (just in case we miss some include files)
#----------------------------------------
#         Fallback on 3rdparty libraries
#----------------------------------------
# Note to avoid some conflict with system include, we must include 3rdparty first
if(NOT ZLIB_FOUND OR FORCE_INTERNAL_ZLIB)
	# use project one
	set(projectZLIB TRUE)
    set(ZLIB_FOUND TRUE)
    # Set path
    set(ZLIB_LIBRARIES zlib)
    # XXX Need to be updated when repository will be updated
    # For the moment include are done with the last fallback in bottom of the file
    # include_directories(${PROJECT_SOURCE_DIR}/3rdparty/zlib)
endif(NOT ZLIB_FOUND OR FORCE_INTERNAL_ZLIB)

if(NOT BZIP2_FOUND OR FORCE_INTERNAL_BZIP2)
	# use project one
	set(projectBZip2 TRUE)
    set(BZIP2_FOUND TRUE)
    # Set path
	set(BZIP2_LIBRARIES bzip2)
    # XXX Need to be updated when repository will be updated
    # For the moment include are done with the last fallback in bottom of the file
    # include_directories(${PROJECT_SOURCE_DIR}/3rdparty/bzip)
endif(NOT BZIP2_FOUND OR FORCE_INTERNAL_BZIP2)

if(NOT SOUNDTOUCH_FOUND OR FORCE_INTERNAL_SOUNDTOUCH)
	# use project one
	set(projectSoundTouch TRUE)
	set(SOUNDTOUCH_FOUND TRUE)
    # Set path
	set(SOUNDTOUCH_LIBRARIES SoundTouch)
    # XXX Need to be updated when repository will be updated
    # For the moment include are done with the last fallback in bottom of the file
    # include_directories(${PROJECT_SOURCE_DIR}/3rdparty/soundtouch_internal)
endif(NOT SOUNDTOUCH_FOUND OR FORCE_INTERNAL_SOUNDTOUCH)

if(NOT A52_FOUND OR FORCE_INTERNAL_A52)
	# use project one
	set(projectA52 TRUE)
    set(A52_FOUND TRUE)
    # Set path
    set(A52_LIBRARIES A52)
    # XXX Need to be updated when repository will be updated
    # For the moment include are done with the last fallback in bottom of the file
    # include_directories(${PROJECT_SOURCE_DIR}/3rdparty/a52_internal)
endif(NOT A52_FOUND OR FORCE_INTERNAL_A52)

#----------------------------------------
#		    Use system include (if not 3rdparty one)
#----------------------------------------
if(Linux)
    # GTK2
	if(GTK2_FOUND)
		include_directories(${GTK2_INCLUDE_DIRS})
	endif(GTK2_FOUND)

	# x11
	if(X11_FOUND)
		include_directories(${X11_INCLUDE_DIR})
	endif(X11_FOUND)
endif(Linux)

# A52
if(A52_FOUND AND NOT projectA52)
	include_directories(${A52_INCLUDE_DIR})
endif(A52_FOUND AND NOT projectA52)

# ALSA
if(ALSA_FOUND)
	include_directories(${ALSA_INCLUDE_DIRS})
endif(ALSA_FOUND)

# bzip2
if(BZIP2_FOUND AND NOT projectBZip2)
	include_directories(${BZIP2_INCLUDE_DIR})
endif(BZIP2_FOUND AND NOT projectBZip2)

# Cg
if(CG_FOUND)
	include_directories(${CG_INCLUDE_DIR})
endif(CG_FOUND)

# GLEW
if(GLEW_FOUND)
	include_directories(${GLEW_INCLUDE_PATH})
endif(GLEW_FOUND)

# OpenGL
if(OPENGL_FOUND)
	include_directories(${OPENGL_INCLUDE_DIR})
endif(OPENGL_FOUND)

# PortAudio
if(PORTAUDIO_FOUND)
	include_directories(${PORTAUDIO_INCLUDE_DIR})
endif(PORTAUDIO_FOUND)

# SDL
if(SDL_FOUND)
	include_directories(${SDL_INCLUDE_DIR})
endif(SDL_FOUND)

# SoundTouch
if(SOUNDTOUCH_FOUND AND NOT projectSoundTouch)
	include_directories(${SOUNDTOUCH_INCLUDE_DIR})
endif(SOUNDTOUCH_FOUND AND NOT projectSoundTouch)

# Note: subversion it only used to detect the current revision of your build
# Subversion optional
if(Subversion_FOUND)
	set(SVN TRUE)
else(Subversion_FOUND)
	set(SVN FALSE)
endif(Subversion_FOUND)

# Wx
if(wxWidgets_FOUND)
    if(Linux)
        # Force the use of 32 bit library
        STRING(REGEX REPLACE "/usr/lib/wx" "${32_LD_LIBRARY_PATH}/wx"
            wxWidgets_INCLUDE_DIRS "${wxWidgets_INCLUDE_DIRS}")
    endif(Linux)
	include(${wxWidgets_USE_FILE})
endif(wxWidgets_FOUND)

# Zlib
if(ZLIB_FOUND AND NOT projectZLIB)
	include_directories(${ZLIB_INCLUDE_DIRS})
endif(ZLIB_FOUND AND NOT projectZLIB)

#----------------------------------------
#		    In all others situation fallback to the 3rdparty
#----------------------------------------
include_directories(${PROJECT_SOURCE_DIR}/3rdparty)
