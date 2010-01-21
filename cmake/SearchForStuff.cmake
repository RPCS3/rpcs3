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

#-------------------------------------------------------------------------------
#									FindStuff
#-------------------------------------------------------------------------------
if(Linux)
	# gtk required
	find_package(GTK2 REQUIRED gtk)

	# gtk found
	if(GTK2_FOUND)
		# add gtk include directories
		include_directories(${GTK2_INCLUDE_DIRS})
	#else(GTK2_FOUND)
	#	message(FATAL_ERROR "GTK2 libraries and include files not found. 
	#			 Please install GTK2 version ${minimal_GTK2_version} or higher.")
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

# zlib required
find_package(ZLIB)

# if we found zlib on the system, use it else use local one
if(ZLIB_FOUND)
	# add zlib include directories
	include_directories(${ZLIB_INCLUDE_DIRS})
else(ZLIB_FOUND)
	# use locale one
	set(localZLIB TRUE)
endif(ZLIB_FOUND)

#------------------------------------------------------------

# bzip2 optional
find_package(BZip2)

# if we found bzip2 on the system,
# use it else use local one
if(BZIP2_FOUND)
	# add zlib include directories
	include_directories(${BZIP2_INCLUDE_DIR})
else(BZIP2_FOUND)
	# use locale one
	set(localBZip2 TRUE)
endif(BZIP2_FOUND)

#------------------------------------------------------------

# OpenGL optional
find_package(OpenGL)

# opengl found
if(OPENGL_FOUND)
	# add OpenGL include directories
	include_directories(${OPENGL_INCLUDE_DIR})
endif(OPENGL_FOUND)

#------------------------------------------------------------

# SDL optional
find_package(SDL)

# SDL found
if(SDL_FOUND)
	# add SDL include directories
	include_directories(${SDL_INCLUDE_DIR})
endif(SDL_FOUND)

#------------------------------------------------------------

include(${PROJECT_SOURCE_DIR}/cmake/FindSoundTouch.cmake)

# found SoundTouch	
if(SOUNDTOUCH_FOUND)
	# add SoundTouch include directories
	include_directories(${SoundTouch_INCLUDE_DIR})
else(SOUNDTOUCH_FOUND)
	# use local one
	set(localSoundTouch TRUE)

	# found
	set(SOUNDTOUCH_FOUND TRUE)
endif(SOUNDTOUCH_FOUND)

#------------------------------------------------------------

include(${PROJECT_SOURCE_DIR}/cmake/FindCg.cmake)

# found Cg
if(CG_FOUND)
	# add Cg include directories
	include_directories(${CG_INCLUDE_DIR})
endif(CG_FOUND)

#------------------------------------------------------------

# ALSA optional
find_package(ALSA)

# ALSA found
if(ALSA_FOUND)
	# add ALSA include directories
	include_directories(${ALSA_INCLUDE_DIRS})
endif(ALSA_FOUND)

#------------------------------------------------------------

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
# requires: SDL
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
# requires: SoundTouch
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
# requires:OpenGL
#---------------------------------------
if(OPENGL_FOUND)
	set(zerogs TRUE)
else(OPENGL_FOUND)
	set(zerogs FALSE)
endif(OPENGL_FOUND)
#---------------------------------------

#---------------------------------------
#			zerospu2
#---------------------------------------
# requires: SoundTouch
#			ALSA
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

