#-------------------------------------------------------------------------------
#						Search all libraries on the system
#-------------------------------------------------------------------------------
# Do not search Lib in /usr/lib64. Hope it is not overwritten in find_package or others macro
SET(FIND_LIBRARY_USE_LIB64_PATHS FALSE)

## Linux only libraries
if(Linux)
    # Most plugins (if not all) and PCSX2 core need gtk2, so 
    # set the required flags
    find_package(GTK2 REQUIRED gtk)
    if(GTK2_FOUND)
        # From version 2.21.3 gtk moved gdk-pixbuf into a separate module
        # Cmake need to be fixed. For the moment uses a manual detection.
        find_path(GDK_PIXBUF_INCLUDE_DIRS NAMES gdk-pixbuf/gdk-pixbuf.h PATHS
            /usr/include/gdk-pixbuf-2.0
            /usr/include)
        if (GDK_PIXBUF_INCLUDE_DIRS)
            set(GTK2_INCLUDE_DIRS "${GDK_PIXBUF_INCLUDE_DIRS}" "${GTK2_INCLUDE_DIRS}")
            # Remove duplicates when cmake will be fixed
            list(REMOVE_DUPLICATES GTK2_INCLUDE_DIRS)
        endif (GDK_PIXBUF_INCLUDE_DIRS)
    endif(GTK2_FOUND)

    find_package(X11)
endif(Linux)

## Use cmake package to find module
find_package(ALSA)
find_package(BZip2)
find_package(Gettext) # translation tool
find_package(JPEG)
find_package(OpenGL)
# Tell cmake that we use SDL as a library and not as an application
if(NOT FORCE_INTERNAL_SDL)
    set(SDL_BUILDING_LIBRARY TRUE)
    find_package(SDL)
endif(NOT FORCE_INTERNAL_SDL)
find_package(Subversion)
# The requierement of wxWidgets is checked in SelectPcsx2Plugins module
# Does not requiere the module (allow to compile non-wx plugins)
# Force the unicode build (the variable is only supported on cmake 2.8.3 and above)
# Warning do not put any double-quote for the argument...
# set(wxWidgets_CONFIG_OPTIONS --unicode=yes --debug=yes) # In case someone want to debug inside wx
set(wxWidgets_CONFIG_OPTIONS --unicode=yes)
find_package(wxWidgets COMPONENTS base core adv)
if(NOT FORCE_INTERNAL_ZLIB)
    find_package(ZLIB)
endif(NOT FORCE_INTERNAL_ZLIB)

## Use pcsx2 package to find module
include(FindCg)
include(FindGlew)
include(FindPortAudio)
if(NOT FORCE_INTERNAL_SOUNDTOUCH)
    include(FindSoundTouch)
endif(NOT FORCE_INTERNAL_SOUNDTOUCH)
include(FindSparseHash)

# Note for include_directory: The order is important to avoid a mess between include file from your system and the one of pcsx2
# If you include first 3rdparty, all 3rdpary include will have a higer priority...
# If you include first /usr/include, all system include will have a higer priority over the pcsx2 one...
# Current implementation: 
# 1/ include 3rdparty sub-directory that we will used (either request or fallback)
# 2/ include system one
#----------------------------------------
#         Fallback on 3rdparty libraries
#----------------------------------------
# Note to avoid some conflict with system include, we must include 3rdparty first
if(NOT ZLIB_FOUND OR FORCE_INTERNAL_ZLIB)
	# use project one
	set(projectZLIB TRUE)
    set(ZLIB_FOUND TRUE)
    # Set path
    set(ZLIB_LIBRARIES pcsx2_zlib)
    include_directories(${PROJECT_SOURCE_DIR}/3rdparty/zlib)
    message(STATUS "Use internal pcsx2 zlib library")
endif(NOT ZLIB_FOUND OR FORCE_INTERNAL_ZLIB)

if(NOT SOUNDTOUCH_FOUND OR FORCE_INTERNAL_SOUNDTOUCH)
	# use project one
	set(projectSoundTouch TRUE)
	set(SOUNDTOUCH_FOUND TRUE)
    # Set path
	set(SOUNDTOUCH_LIBRARIES pcsx2_SoundTouch)
    include_directories(${PROJECT_SOURCE_DIR}/3rdparty/soundtouch_linux_include)
    message(STATUS "Use internal pcsx2 SoundTouch library")
endif(NOT SOUNDTOUCH_FOUND OR FORCE_INTERNAL_SOUNDTOUCH)

if(NOT SDL_FOUND OR FORCE_INTERNAL_SDL)
	# use project one
    set(projectSDL TRUE)
	set(SDL_FOUND TRUE)
    # Set path
    set(SDL_LIBRARY pcsx2_SDL)
    include_directories(${PROJECT_SOURCE_DIR}/3rdparty/SDL-1.3.0-5387/include)
    message(STATUS "Use internal pcsx2 SDL library")
endif(NOT SDL_FOUND OR FORCE_INTERNAL_SDL)

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

# ALSA
if(ALSA_FOUND)
	include_directories(${ALSA_INCLUDE_DIRS})
endif(ALSA_FOUND)

# bzip2
if(BZIP2_FOUND)
	include_directories(${BZIP2_INCLUDE_DIR})
endif(BZIP2_FOUND)

# Cg
if(CG_FOUND)
	include_directories(${CG_INCLUDE_DIRS})
endif(CG_FOUND)

# Jpeg
if(JPEG_FOUND)
	include_directories(${JPEG_INCLUDE_DIR})
endif(JPEG_FOUND)

# GLEW
if(GLEW_FOUND)
    include_directories(${GLEW_INCLUDE_DIR})
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
if(SDL_FOUND AND NOT projectSDL)
	include_directories(${SDL_INCLUDE_DIR})
endif(SDL_FOUND AND NOT projectSDL)

# SoundTouch
if(SOUNDTOUCH_FOUND AND NOT projectSoundTouch)
	include_directories(${SOUNDTOUCH_INCLUDE_DIR})
endif(SOUNDTOUCH_FOUND AND NOT projectSoundTouch)

# SPARSEHASH
if(SPARSEHASH_FOUND)
	include_directories(${SPARSEHASH_INCLUDE_DIR})
endif(SPARSEHASH_FOUND)

# Wx
if(wxWidgets_FOUND)
    if(Linux)
        # Force the use of 32 bit library configuration on
        # 64 bits machine with 32 bits library in /usr/lib32
        if(CMAKE_SIZEOF_VOID_P MATCHES "8" AND EXISTS "/usr/lib32")
            if (EXISTS "/usr/lib32")
                # Debian/ubuntu. 64b in /usr/lib and 32b in /usr/lib32
                STRING(REGEX REPLACE "/usr/lib/wx" "/usr/lib32/wx" wxWidgets_INCLUDE_DIRS "${wxWidgets_INCLUDE_DIRS}")
                # I'm sure someone did it! 64b in /usr/lib64 and 32b in /usr/lib32
                STRING(REGEX REPLACE "/usr/lib64/wx" "/usr/lib32/wx" wxWidgets_INCLUDE_DIRS "${wxWidgets_INCLUDE_DIRS}")
            endif (EXISTS "/usr/lib32")
            if (EXISTS "/usr/lib")
                # Fedora/Open suse. 64b in /usr/lib64 and 32b in /usr/lib
                STRING(REGEX REPLACE "/usr/lib64/wx" "/usr/lib/wx" wxWidgets_INCLUDE_DIRS "${wxWidgets_INCLUDE_DIRS}")
            endif (EXISTS "/usr/lib")
        endif(CMAKE_SIZEOF_VOID_P MATCHES "8" AND EXISTS "/usr/lib32")
    endif(Linux)

	include(${wxWidgets_USE_FILE})
endif(wxWidgets_FOUND)

# Zlib
if(ZLIB_FOUND AND NOT projectZLIB)
	include_directories(${ZLIB_INCLUDE_DIRS})
endif(ZLIB_FOUND AND NOT projectZLIB)
