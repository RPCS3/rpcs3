#-------------------------------------------------------------------------------
#								Pcsx2 core & common libs
#-------------------------------------------------------------------------------
# Check for additional dependencies.
# If all dependencies are available, including OS, build it
#-------------------------------------------------------------------------------

#---------------------------------------
#			Common libs
# requires: -wx
#---------------------------------------
if(wxWidgets_FOUND)
    set(common_libs TRUE)
else(wxWidgets_FOUND)
    set(common_libs FALSE)
    message(STATUS "Skip build of common libraries: miss some dependencies")
endif(wxWidgets_FOUND)

#---------------------------------------
#			Pcsx2 core
# requires: -wx
#           -gtk2 (linux)
#           -zlib
#           -common_libs
#---------------------------------------
# Common dependancy
if(wxWidgets_FOUND AND ZLIB_FOUND AND common_libs)
    set(pcsx2_core TRUE)
else(wxWidgets_FOUND AND ZLIB_FOUND AND common_libs)
    set(pcsx2_core FALSE)
    message(STATUS "Skip build of pcsx2 core: miss some dependencies")
endif(wxWidgets_FOUND AND ZLIB_FOUND AND common_libs)
# Linux need also gtk2
if(Linux AND NOT GTK2_FOUND)
    set(pcsx2_core FALSE)
    message(STATUS "Skip build of pcsx2 core: miss some dependencies")
endif(Linux AND NOT GTK2_FOUND)


#-------------------------------------------------------------------------------
#								Plugins
#-------------------------------------------------------------------------------
# Check all plugins for additional dependencies.
# If all dependencies of a plugin are available, including OS, the plugin will
# be build.
#-------------------------------------------------------------------------------

#---------------------------------------
#			CDVDnull
#---------------------------------------
set(CDVDnull TRUE)
#---------------------------------------

#---------------------------------------
#			CDVDiso
#---------------------------------------
# requires: -BZip2
#---------------------------------------
if(BZIP2_FOUND)
    set(CDVDiso TRUE)
else(BZIP2_FOUND)
    set(CDVDiso FALSE)
    message(STATUS "Skip build of CDVDiso: miss some dependencies")
endif(BZIP2_FOUND)

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
#			zerogs
#---------------------------------------
# requires:	-GLEW
#			-OpenGL
#			-X11
#			-CG
#---------------------------------------
if(GLEW_FOUND AND OPENGL_FOUND AND X11_FOUND AND CG_FOUND)
	set(zerogs TRUE)
else(GLEW_FOUND AND OPENGL_FOUND AND X11_FOUND AND CG_FOUND)
	set(zerogs FALSE)
    message(STATUS "Skip build of zerogs: miss some dependencies")
endif(GLEW_FOUND AND OPENGL_FOUND AND X11_FOUND AND CG_FOUND)
#---------------------------------------

#---------------------------------------
#			zzogl-pg
#---------------------------------------
# requires:	-GLEW
#			-OpenGL
#			-X11
#			-CG
#           -common_libs
#---------------------------------------
if(GLEW_FOUND AND OPENGL_FOUND AND X11_FOUND AND CG_FOUND AND common_libs)
	set(zzogl TRUE)
else(GLEW_FOUND AND OPENGL_FOUND AND X11_FOUND AND CG_FOUND AND common_libs)
	set(zzogl FALSE)
    message(STATUS "Skip build of zzogl: miss some dependencies")
endif(GLEW_FOUND AND OPENGL_FOUND AND X11_FOUND AND CG_FOUND AND common_libs)
#---------------------------------------

#---------------------------------------
#			PadNull
#---------------------------------------
set(PadNull TRUE)
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
    message(STATUS "Skip build of onepad: miss some dependencies")
endif(SDL_FOUND)
#---------------------------------------

#---------------------------------------
#			zeropad
#---------------------------------------
# requires: -SDL
#---------------------------------------
if(SDL_FOUND)
	set(zeropad TRUE)
else(SDL_FOUND)
	set(zeropad FALSE)
    message(STATUS "Skip build of zeropad: miss some dependencies")
endif(SDL_FOUND)
#---------------------------------------

#---------------------------------------
#			SPU2null
#---------------------------------------
set(SPU2null TRUE)
#---------------------------------------

#---------------------------------------
#			spu2-x
#---------------------------------------
# requires: -SoundTouch
#			-ALSA
#           -Portaudio
#           -A52
#           -common_libs
#---------------------------------------
if(A52_FOUND AND ALSA_FOUND AND PORTAUDIO_FOUND AND SOUNDTOUCH_FOUND AND common_libs)
	set(spu2-x TRUE)
else(A52_FOUND AND ALSA_FOUND AND PORTAUDIO_FOUND AND SOUNDTOUCH_FOUND AND common_libs)
	set(spu2-x FALSE)
    message(STATUS "Skip build of spu2-x: miss some dependencies")
endif(A52_FOUND AND ALSA_FOUND AND PORTAUDIO_FOUND AND SOUNDTOUCH_FOUND AND common_libs)
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
    message(STATUS "Skip build of zerospu2: miss some dependencies")
endif(SOUNDTOUCH_FOUND AND ALSA_FOUND)
#---------------------------------------

#---------------------------------------
#			USBnull
#---------------------------------------
set(USBnull TRUE)
#---------------------------------------

#-------------------------------------------------------------------------------
#			[TODO] Write CMakeLists.txt for these plugins.
set(cdvdGigaherz FALSE)
set(CDVDisoEFP FALSE)
set(CDVDlinuz FALSE)
set(CDVDolio FALSE)
set(CDVDpeops FALSE)
set(GSdx FALSE)
set(LilyPad FALSE)
set(PeopsSPU2 FALSE)
set(SSSPSXPAD FALSE)
set(xpad FALSE)
#-------------------------------------------------------------------------------
