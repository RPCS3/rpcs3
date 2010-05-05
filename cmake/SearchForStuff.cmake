# Search for additional software.

#-------------------------------------------------------------------------------
# Minmal required version of libraries
#-------------------------------------------------------------------------------
set(minimal_wxWidgets_version 2.8.0)
set(minimal_GTK2_version 2.10)
set(minimal_SDL_version 1.2)

# to set the proper dependencies and decide which plugins should be build we
# need to know on which OS we are currenty working/running
detectOperatingSystem()

SET(CMAKE_SHARED_LIBRARY_LINK_C_FLAGS " ")
SET(CMAKE_SHARED_LIBRARY_LINK_CXX_FLAGS " ")

#-------------------------------------------------------------------------------
#									FindStuff
#-------------------------------------------------------------------------------
#----------------------------------------
#						Required
#----------------------------------------
if(Linux)			# Linux only
	# gtk required
	find_package(GTK2 REQUIRED gtk)

	# gtk found
	if(GTK2_FOUND)
		# add gtk include directories
		include_directories(${GTK2_INCLUDE_DIRS})
	#else(GTK2_FOUND)
	#	message(FATAL_ERROR "GTK2 libraries and include files not found. 
	#			Please install GTK2 version ${minimal_GTK2_version} or higher.")
	endif(GTK2_FOUND)

endif(Linux)

#------------------------------------------------------------

# wx required
find_package(wxWidgets REQUIRED base core adv)

# wx found
if(wxWidgets_FOUND)
	include(${wxWidgets_USE_FILE})
#else(wxWidgets_FOUND)
#	message(FATAL_ERROR "wxWidgets libraries and include files not found.\
#			Please install wxWidgets version ${minimal_wxWidgets_version} \
#			or higher.")
endif(wxWidgets_FOUND)

#------------------------------------------------------------

# zlib required (no require flag, because we can use project one as fallback)
find_package(ZLIB)

# if we found zlib on the system, use it else use project one
#if(ZLIB_FOUND)
#	# add zlib include directories
#	include_directories(${ZLIB_INCLUDE_DIRS})
#else(ZLIB_FOUND)
	# use project one
	set(projectZLIB TRUE)
#endif(ZLIB_FOUND)

#------------------------------------------------------------


#----------------------------------------
#						Optional
#----------------------------------------
if(Linux)			# Linux only
	# x11 optional
	find_package(X11)

	# x11 found
	if(X11_FOUND)
		# add x11 include directories
		include_directories(${X11_INCLUDE_DIR})
	#else(X11_FOUND)
	#	message(FATAL_ERROR "X11 libraries and include files not found. 
	#			Please install X11.")
	endif(X11_FOUND)
endif(Linux)

    # Manually find Xxf86vm because it is not done in the module...
    FIND_LIBRARY(X11_Xxf86vm_LIB Xxf86vm       ${X11_LIB_SEARCH_PATH})
    MARK_AS_ADVANCED(X11_Xxf86vm_LIB)

#------------------------------------------------------------

# ALSA optional
find_package(ALSA)

# ALSA found
if(ALSA_FOUND)
	# add ALSA include directories
	include_directories(${ALSA_INCLUDE_DIRS})
endif(ALSA_FOUND)

#------------------------------------------------------------

# bzip2 optional
find_package(BZip2)

# if we found bzip2 on the system,
# use it else use project one
#if(BZIP2_FOUND)
#	# add zlib include directories
#	include_directories(${BZIP2_INCLUDE_DIR})
#else(BZIP2_FOUND)
	# use project one
	set(projectBZip2 TRUE)
#endif(BZIP2_FOUND)

#------------------------------------------------------------

# Cg optional
include(${PROJECT_SOURCE_DIR}/cmake/FindCg.cmake)

# found Cg
if(CG_FOUND)
	# add Cg include directories
	include_directories(${CG_INCLUDE_DIR})
endif(CG_FOUND)

#------------------------------------------------------------

# GLEW optional
include(${PROJECT_SOURCE_DIR}/cmake/FindGlew.cmake)

# found GLEW
if(GLEW_FOUND)
	# add GLEW include directories
	include_directories(${GLEW_INCLUDE_PATH})
endif(GLEW_FOUND)

#------------------------------------------------------------

# OpenGL optional
find_package(OpenGL)

# opengl found
if(OPENGL_FOUND)
	# add OpenGL include directories
	include_directories(${OPENGL_INCLUDE_DIR})
endif(OPENGL_FOUND)

#------------------------------------------------------------

# PortAudio optional
include(${PROJECT_SOURCE_DIR}/cmake/FindPortAudio.cmake)

# found PortAudio
if(PORTAUDIO_FOUND)
	# add PortAudio include directories
	include_directories(${PORTAUDIO_INCLUDE_DIR})
endif(PORTAUDIO_FOUND)

#------------------------------------------------------------

# SDL optional
set(SDL_BUILDING_LIBRARY TRUE)
find_package(SDL)

# SDL found
if(SDL_FOUND)
	# add SDL include directories
	include_directories(${SDL_INCLUDE_DIR})
endif(SDL_FOUND)

#------------------------------------------------------------

# SoundTouch optional
#include(${PROJECT_SOURCE_DIR}/cmake/FindSoundTouch.cmake)

# found SoundTouch	
#if(SOUNDTOUCH_FOUND)
#	# add SoundTouch include directories
#	include_directories(${SOUNDTOUCH_INCLUDE_DIR})
#else(SOUNDTOUCH_FOUND)
	# use project one
	set(projectSoundTouch TRUE)

	# found
	set(SOUNDTOUCH_FOUND TRUE)
#endif(SOUNDTOUCH_FOUND)

#------------------------------------------------------------

# Subversion optional
find_package(Subversion)

# subversion found
if(Subversion_FOUND)
	set(SVN TRUE)
else(Subversion_FOUND)
	set(SVN FALSE)
endif(Subversion_FOUND)

#-------------------------------------------------------------------------------
#								Plugins
#-------------------------------------------------------------------------------
# Check all plugins for additional dependencies.
# If all dependencies of a plugin are available, including OS, the plugin will
# be build.
#-------------------------------------------------------------------------------
# null plugins should work on every platform, enable them all
#---------------------------------------
#			CDVDnull
#---------------------------------------
set(CDVDnull TRUE)
#---------------------------------------

#---------------------------------------
#			dev9null
#---------------------------------------
set(dev9null TRUE)
#---------------------------------------

#---------------------------------------
#			FWnull
#---------------------------------------
set(FWnull TRUE)
#---------------------------------------

#---------------------------------------
#			GSnull
#---------------------------------------
set(GSnull TRUE)
#---------------------------------------

#---------------------------------------
#			PadNull
#---------------------------------------
set(PadNull TRUE)
#---------------------------------------

#---------------------------------------
#			SPU2null
#---------------------------------------
set(SPU2null TRUE)
#---------------------------------------

#---------------------------------------
#			USBnull
#---------------------------------------
set(USBnull TRUE)
#---------------------------------------

#---------------------------------------
#			onepad
#---------------------------------------
# requires: -SDL
#---------------------------------------
if(SDL_FOUND)
	set(onepad TRUE)
else(SDL_FOUND)
	set(onepad FALSE)
endif(SDL_FOUND)
#---------------------------------------

#---------------------------------------
#			spu2-x
#---------------------------------------
# requires: -SoundTouch
#---------------------------------------
if(SOUNDTOUCH_FOUND)
	set(spu2-x TRUE)
else(SOUNDTOUCH_FOUND)
	set(spu2-x FALSE)
endif(SOUNDTOUCH_FOUND)
#---------------------------------------

#---------------------------------------
#			zerogs
#---------------------------------------
# requires: -GLEW
#			-OpenGL
#			-X11
#---------------------------------------
if(GLEW_FOUND AND OPENGL_FOUND AND X11_FOUND)
	set(zerogs TRUE)
else(GLEW_FOUND AND OPENGL_FOUND AND X11_FOUND)
	set(zerogs FALSE)
endif(GLEW_FOUND AND OPENGL_FOUND AND X11_FOUND)
#---------------------------------------

#---------------------------------------
#			zerospu2
#---------------------------------------
# requires: -SoundTouch
#			-ALSA
#			-PortAudio
#---------------------------------------
if(SOUNDTOUCH_FOUND AND ALSA_FOUND)
	set(zerospu2 TRUE)
else(SOUNDTOUCH_FOUND AND ALSA_FOUND)
	set(zerospu2 FALSE)
endif(SOUNDTOUCH_FOUND AND ALSA_FOUND)
#---------------------------------------

#-------------------------------------------------------------------------------
#			[TODO] Write CMakeLists.txt for these plugins.
set(cdvdGigaherz FALSE)
set(CDVDiso FALSE)
set(CDVDisoEFP FALSE)
set(CDVDlinuz FALSE)
set(CDVDolio FALSE)
set(CDVDpeops FALSE)
set(GSdx FALSE)
set(LilyPad FALSE)
set(PeopsSPU2 FALSE)
set(SSSPSXPAD FALSE)
set(xpad FALSE)
set(zeropad FALSE)
#-------------------------------------------------------------------------------

