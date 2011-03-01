#-------------------------------------------------------------------------------
#                              Dependency message print
#-------------------------------------------------------------------------------
set(msg_dep_common_libs "check these libraries -> wxWidgets (>=2.8.10), sparsehash (>=1.5)")
set(msg_dep_pcsx2       "check these libraries -> wxWidgets (>=2.8.10), gtk2 (>=2.16), zlib (>=1.2.4), pcsx2 common libs")
set(msg_dep_cdvdiso     "check these libraries -> bzip2 (>=1.0.5)")
set(msg_dep_zerogs      "check these libraries -> glew (>=1.5), opengl, X11, nvidia-cg-toolkit (>=2.1)")
set(msg_dep_gsdx        "check these libraries -> opengl, X11, pcsx2 SDL")
set(msg_dep_zzogl       "check these libraries -> glew (>=1.5), jpeg (>=6.2), opengl, X11, nvidia-cg-toolkit (>=2.1), pcsx2 common libs")
set(msg_dep_onepad      "check these libraries -> sdl (>=1.2)")
set(msg_dep_zeropad     "check these libraries -> sdl (>=1.2)")
set(msg_dep_spu2x       "check these libraries -> soundtouch (>=1.5), alsa, portaudio (>=1.9), pcsx2 common libs")
set(msg_dep_zerospu2    "check these libraries -> soundtouch (>=1.5), alsa")

#-------------------------------------------------------------------------------
#								Pcsx2 core & common libs
#-------------------------------------------------------------------------------
# Check for additional dependencies.
# If all dependencies are available, including OS, build it
#-------------------------------------------------------------------------------

#---------------------------------------
#			Common libs
# requires: -wx
#           -sparsehash
#---------------------------------------
if(wxWidgets_FOUND AND SPARSEHASH_FOUND)
    set(common_libs TRUE)
else(wxWidgets_FOUND AND SPARSEHASH_FOUND)
    set(common_libs FALSE)
    message(STATUS "Skip build of common libraries: miss some dependencies")
    message(STATUS "${msg_dep_common_libs}")
endif(wxWidgets_FOUND AND SPARSEHASH_FOUND)

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
    message(STATUS "${msg_dep_pcsx2}")
endif(wxWidgets_FOUND AND ZLIB_FOUND AND common_libs)
# Linux need also gtk2
if(Linux AND NOT GTK2_FOUND)
    set(pcsx2_core FALSE)
    message(STATUS "Skip build of pcsx2 core: miss some dependencies")
    message(STATUS "${msg_dep_pcsx2}")
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
    message(STATUS "${msg_dep_cdvdiso}")
endif(BZIP2_FOUND)

#---------------------------------------
#			CDVDlinuz
#---------------------------------------
set(CDVDlinuz TRUE)

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
#			GSdx
#---------------------------------------
# requires: -OpenGL
#			-X11
#           -PCSX2 SDL
#---------------------------------------
if(OPENGL_FOUND AND X11_FOUND AND projectSDL)
    set(GSdx TRUE)
else(OPENGL_FOUND AND X11_FOUND AND projectSDL)
	set(GSdx FALSE)
    message(STATUS "Skip build of GSdx: miss some dependencies")
    message(STATUS "${msg_dep_gsdx}")
endif(OPENGL_FOUND AND X11_FOUND AND projectSDL)
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
    message(STATUS "${msg_dep_zerogs}")
endif(GLEW_FOUND AND OPENGL_FOUND AND X11_FOUND AND CG_FOUND)
#---------------------------------------

#---------------------------------------
#			zzogl-pg
#---------------------------------------
# requires:	-GLEW
#			-OpenGL
#			-X11
#			-CG
#			-JPEG
#           -common_libs
#---------------------------------------
if(GLEW_FOUND AND OPENGL_FOUND AND X11_FOUND AND CG_FOUND AND JPEG_FOUND AND common_libs)
	set(zzogl TRUE)
else(GLEW_FOUND AND OPENGL_FOUND AND X11_FOUND AND CG_FOUND AND JPEG_FOUND AND common_libs)
	set(zzogl FALSE)
    message(STATUS "Skip build of zzogl: miss some dependencies")
    message(STATUS "${msg_dep_zzogl}")
endif(GLEW_FOUND AND OPENGL_FOUND AND X11_FOUND AND CG_FOUND AND JPEG_FOUND AND common_libs)
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
# FIXME: internal sdl does not like X11 input with dynamic feature (SDL_VIDEO_DRIVER_X11_DYNAMIC_XINPUT)
if(NOT FORCE_INTERNAL_SDL)
if(SDL_FOUND)
	set(onepad TRUE)
else(SDL_FOUND)
	set(onepad FALSE)
    message(STATUS "Skip build of onepad: miss some dependencies")
    message(STATUS "${msg_dep_onepad}")
endif(SDL_FOUND)
else(NOT FORCE_INTERNAL_SDL)
    message(STATUS "Skip build of onepad not compatible with internal SDL")
endif(NOT FORCE_INTERNAL_SDL)
#---------------------------------------

#---------------------------------------
#			zeropad
#---------------------------------------
# requires: -SDL
#---------------------------------------
# FIXME: internal sdl does not like X11 input with dynamic feature (SDL_VIDEO_DRIVER_X11_DYNAMIC_XINPUT)
if(NOT FORCE_INTERNAL_SDL)
if(SDL_FOUND)
	set(zeropad TRUE)
else(SDL_FOUND)
	set(zeropad FALSE)
    message(STATUS "Skip build of zeropad: miss some dependencies")
    message(STATUS "${msg_dep_zeropad}")
endif(SDL_FOUND)
else(NOT FORCE_INTERNAL_SDL)
    message(STATUS "Skip build of zeropad not compatible with internal SDL")
endif(NOT FORCE_INTERNAL_SDL)
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
#           -common_libs
#---------------------------------------
if(ALSA_FOUND AND PORTAUDIO_FOUND AND SOUNDTOUCH_FOUND AND common_libs)
	set(spu2-x TRUE)
else(ALSA_FOUND AND PORTAUDIO_FOUND AND SOUNDTOUCH_FOUND AND common_libs)
	set(spu2-x FALSE)
    message(STATUS "Skip build of spu2-x: miss some dependencies")
    message(STATUS "${msg_dep_spu2x}")
endif(ALSA_FOUND AND PORTAUDIO_FOUND AND SOUNDTOUCH_FOUND AND common_libs)
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
    message(STATUS "${msg_dep_zerospu2}")
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
set(CDVDolio FALSE)
set(CDVDpeops FALSE)
set(LilyPad FALSE)
set(PeopsSPU2 FALSE)
set(SSSPSXPAD FALSE)
set(xpad FALSE)
#-------------------------------------------------------------------------------
