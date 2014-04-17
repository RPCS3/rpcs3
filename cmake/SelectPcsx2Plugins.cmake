#-------------------------------------------------------------------------------
#                              Dependency message print
#-------------------------------------------------------------------------------
set(msg_dep_common_libs "check these libraries -> wxWidgets (>=2.8.10), sparsehash (>=1.5), aio")
set(msg_dep_pcsx2       "check these libraries -> wxWidgets (>=2.8.10), gtk2 (>=2.16), zlib (>=1.2.4), pcsx2 common libs")
set(msg_dep_cdvdiso     "check these libraries -> bzip2 (>=1.0.5), gtk2 (>=2.16)")
set(msg_dep_zerogs      "check these libraries -> glew (>=1.6), opengl, X11, nvidia-cg-toolkit (>=2.1)")
set(msg_dep_gsdx        "check these libraries -> opengl, egl, X11")
set(msg_dep_onepad      "check these libraries -> sdl (==1.2)")
set(msg_dep_spu2x       "check these libraries -> soundtouch (>=1.5), alsa, portaudio (>=1.9), sdl (==1.2) pcsx2 common libs")
set(msg_dep_zerospu2    "check these libraries -> soundtouch (>=1.5), alsa")
if(GLSL_API)
	set(msg_dep_zzogl       "check these libraries -> glew (>=1.6), jpeg (>=6.2), opengl, X11, pcsx2 common libs")
else(GLSL_API)
	set(msg_dep_zzogl       "check these libraries -> glew (>=1.6), jpeg (>=6.2), opengl, X11, nvidia-cg-toolkit (>=2.1), pcsx2 common libs")
endif()

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
elseif(NOT EXISTS "${CMAKE_SOURCE_DIR}/common/src")
    set(common_libs FALSE)
else()
    set(common_libs FALSE)
    message(STATUS "Skip build of common libraries: miss some dependencies")
    message(STATUS "${msg_dep_common_libs}")
endif()

#---------------------------------------
#			Pcsx2 core
# requires: -wx
#           -gtk2 (linux)
#           -zlib
#           -common_libs
#           -aio
#---------------------------------------
# Common dependancy
if(wxWidgets_FOUND AND ZLIB_FOUND AND common_libs AND AIO_FOUND)
    set(pcsx2_core TRUE)
elseif(NOT EXISTS "${CMAKE_SOURCE_DIR}/pcsx2")
    set(pcsx2_core FALSE)
else()
    set(pcsx2_core FALSE)
    message(STATUS "Skip build of pcsx2 core: miss some dependencies")
    message(STATUS "${msg_dep_pcsx2}")
endif()
# Linux need also gtk2
if(Linux AND pcsx2_core AND NOT GTK2_FOUND)
    set(pcsx2_core FALSE)
    message(STATUS "Skip build of pcsx2 core: miss some dependencies")
    message(STATUS "${msg_dep_pcsx2}")
endif()


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
if(GTK2_FOUND)
    set(CDVDnull TRUE)
endif()
#---------------------------------------

#---------------------------------------
#			CDVDiso
#---------------------------------------
# requires: -BZip2
#           -gtk2 (linux)
#---------------------------------------
if(EXTRA_PLUGINS)
    if(BZIP2_FOUND AND GTK2_FOUND)
        set(CDVDiso TRUE)
    elseif(NOT EXISTS "${CMAKE_SOURCE_DIR}/plugins/CDVDiso")
        set(CDVDiso FALSE)
    else()
        set(CDVDiso FALSE)
        message(STATUS "Skip build of CDVDiso: miss some dependencies")
        message(STATUS "${msg_dep_cdvdiso}")
    endif()
endif()

#---------------------------------------
#			CDVDlinuz
#---------------------------------------
if(EXTRA_PLUGINS)
    set(CDVDlinuz TRUE)
endif()

#---------------------------------------
#			dev9null
#---------------------------------------
if(GTK2_FOUND)
    set(dev9null TRUE)
endif()
#---------------------------------------

#---------------------------------------
#			FWnull
#---------------------------------------
if(GTK2_FOUND)
    set(FWnull TRUE)
endif()
#---------------------------------------

#---------------------------------------
#			GSnull
#---------------------------------------
if(GTK2_FOUND AND EXTRA_PLUGINS)
    set(GSnull TRUE)
endif()
#---------------------------------------

#---------------------------------------
#			GSdx
#---------------------------------------
# requires: -OpenGL
#			-X11
#---------------------------------------
if(OPENGL_FOUND AND X11_FOUND AND EGL_FOUND)
    set(GSdx TRUE)
elseif(NOT EXISTS "${CMAKE_SOURCE_DIR}/plugins/GSdx")
	set(GSdx FALSE)
else()
	set(GSdx FALSE)
    message(STATUS "Skip build of GSdx: miss some dependencies")
    message(STATUS "${msg_dep_gsdx}")
endif()
#---------------------------------------

#---------------------------------------
#			zerogs
#---------------------------------------
# requires:	-GLEW
#			-OpenGL
#			-X11
#			-CG
#---------------------------------------
if(EXTRA_PLUGINS)
    if(GLEW_FOUND AND OPENGL_FOUND AND X11_FOUND AND CG_FOUND)
        set(zerogs TRUE)
    elseif(NOT EXISTS "${CMAKE_SOURCE_DIR}/plugins/zerogs")
        set(zerogs FALSE)
    else()
        set(zerogs FALSE)
        message(STATUS "Skip build of zerogs: miss some dependencies")
        message(STATUS "${msg_dep_zerogs}")
    endif()
endif()
#---------------------------------------

#---------------------------------------
#			zzogl-pg
#---------------------------------------
# requires:	-GLEW
#			-OpenGL
#			-X11
#			-CG (only with cg build
#			-JPEG
#           -common_libs
#---------------------------------------
if((GLEW_FOUND AND OPENGL_FOUND AND X11_FOUND AND JPEG_FOUND AND common_libs) AND (CG_FOUND OR GLSL_API))
	set(zzogl TRUE)
    if(CG_FOUND AND NOT GLSL_API AND EXTRA_PLUGINS)
		set(zzoglcg TRUE)
	else()
		set(zzoglcg FALSE)
	endif()
elseif(NOT EXISTS "${CMAKE_SOURCE_DIR}/plugins/zzogl-pg")
	set(zzogl FALSE)
	set(zzoglcg FALSE)
	set(REBUILD_SHADER FALSE)
else()
	set(zzogl FALSE)
	set(zzoglcg FALSE)
	set(REBUILD_SHADER FALSE)
    message(STATUS "Skip build of zzogl: miss some dependencies")
    message(STATUS "${msg_dep_zzogl}")
endif()
#---------------------------------------

#---------------------------------------
#			PadNull
#---------------------------------------
if(GTK2_FOUND AND EXTRA_PLUGINS)
    set(PadNull TRUE)
endif()
#---------------------------------------

#---------------------------------------
#			onepad
#---------------------------------------
# requires: -SDL
#---------------------------------------
if(SDL_FOUND)
	set(onepad TRUE)
elseif(NOT EXISTS "${CMAKE_SOURCE_DIR}/plugins/onepad")
	set(onepad FALSE)
else()
	set(onepad FALSE)
    message(STATUS "Skip build of onepad: miss some dependencies")
    message(STATUS "${msg_dep_onepad}")
endif()
#---------------------------------------

#---------------------------------------
#			SPU2null
#---------------------------------------
if(GTK2_FOUND AND EXTRA_PLUGINS)
    set(SPU2null TRUE)
endif()
#---------------------------------------

#---------------------------------------
#			spu2-x
#---------------------------------------
# requires: -SoundTouch
#			-ALSA
#           -Portaudio
#           -SDL
#           -common_libs
#---------------------------------------
if(ALSA_FOUND AND PORTAUDIO_FOUND AND SOUNDTOUCH_FOUND AND SDL_FOUND AND common_libs)
	set(spu2-x TRUE)
elseif(NOT EXISTS "${CMAKE_SOURCE_DIR}/plugins/spu2-x")
	set(spu2-x FALSE)
else()
	set(spu2-x FALSE)
    message(STATUS "Skip build of spu2-x: miss some dependencies")
    message(STATUS "${msg_dep_spu2x}")
endif()
#---------------------------------------

#---------------------------------------
#			zerospu2
#---------------------------------------
# requires: -SoundTouch
#			-ALSA
#			-PortAudio
#---------------------------------------
if(EXTRA_PLUGINS)
    if(EXISTS "${CMAKE_SOURCE_DIR}/plugins/zerospu2" AND SOUNDTOUCH_FOUND AND ALSA_FOUND)
        set(zerospu2 TRUE)
        # Comment the next line, if you want to compile zerospu2
        set(zerospu2 FALSE)
        message(STATUS "Don't build zerospu2. It is super-seeded by spu2x")
    elseif(NOT EXISTS "${CMAKE_SOURCE_DIR}/plugins/zerospu2")
        set(zerospu2 FALSE)
    else()
        set(zerospu2 FALSE)
        message(STATUS "Skip build of zerospu2: miss some dependencies")
        message(STATUS "${msg_dep_zerospu2}")
    endif()
endif()
#---------------------------------------

#---------------------------------------
#			USBnull
#---------------------------------------
if(GTK2_FOUND)
    set(USBnull TRUE)
endif()
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
