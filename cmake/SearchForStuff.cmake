#-------------------------------------------------------------------------------
#						Search all libraries on the system
#-------------------------------------------------------------------------------
# Do not search Lib in /usr/lib64. Hope it is not overwritten in find_package or others macro
SET(FIND_LIBRARY_USE_LIB64_PATHS FALSE)

## Linux only libraries
if(Linux)
    # Most plugins (if not all) and PCSX2 core need gtk2, so set the required flags
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
find_package(Git)
find_package(JPEG)
find_package(OpenGL)
# Tell cmake that we use SDL as a library and not as an application
set(SDL_BUILDING_LIBRARY TRUE)
find_package(SDL)
# The requirement of wxWidgets is checked in SelectPcsx2Plugins module
# Does not require the module (allow to compile non-wx plugins)
# Force the unicode build (the variable is only supported on cmake 2.8.3 and above)
# Warning do not put any double-quote for the argument...
# set(wxWidgets_CONFIG_OPTIONS --unicode=yes --debug=yes) # In case someone want to debug inside wx
#
# Fedora uses an extra non-standard option ... Arch must be the first option.
if(Fedora)
    set(wxWidgets_CONFIG_OPTIONS --arch i686 --unicode=yes)
else()
    set(wxWidgets_CONFIG_OPTIONS --unicode=yes)
endif()
find_package(wxWidgets COMPONENTS base core adv)
find_package(ZLIB)

## Use pcsx2 package to find module
include(FindAio)
## Include cg because of zzogl-cg and zerogs
#if(NOT GLSL_API)
	include(FindCg)
#endif(NOT GLSL_API)
include(FindEGL)
include(FindGLES2)
include(FindGlew)
include(FindLibc)
include(FindPortAudio)
include(FindSoundTouch)
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
# Empty

#----------------------------------------
#		    Use system include (if not 3rdparty one)
#----------------------------------------
if(Linux)
	if(GTK2_FOUND)
		include_directories(${GTK2_INCLUDE_DIRS})
	endif(GTK2_FOUND)

	if(X11_FOUND)
		include_directories(${X11_INCLUDE_DIR})
	endif(X11_FOUND)
endif(Linux)

if(AIO_FOUND)
    include_directories(${AIO_INCLUDE_DIR})
endif()

if(ALSA_FOUND)
	include_directories(${ALSA_INCLUDE_DIRS})
endif(ALSA_FOUND)

if(BZIP2_FOUND)
	include_directories(${BZIP2_INCLUDE_DIR})
endif(BZIP2_FOUND)

if(CG_FOUND)
	include_directories(${CG_INCLUDE_DIRS})
endif(CG_FOUND)

if (EGL_FOUND)
    include_directories(${EGL_INCLUDE_DIR})
endif()

if(JPEG_FOUND)
	include_directories(${JPEG_INCLUDE_DIR})
endif(JPEG_FOUND)

if(GLEW_FOUND)
    include_directories(${GLEW_INCLUDE_DIR})
endif(GLEW_FOUND)

if(GLESV2_FOUND)
    include_directories(${GLESV2_INCLUDE_DIR})
endif()

if(OPENGL_FOUND)
	include_directories(${OPENGL_INCLUDE_DIR})
endif(OPENGL_FOUND)

if(PORTAUDIO_FOUND)
	include_directories(${PORTAUDIO_INCLUDE_DIR})
endif(PORTAUDIO_FOUND)

if(SDL_FOUND)
	include_directories(${SDL_INCLUDE_DIR})
endif(SDL_FOUND)

if(SOUNDTOUCH_FOUND)
	include_directories(${SOUNDTOUCH_INCLUDE_DIR})
endif(SOUNDTOUCH_FOUND)

if(SPARSEHASH_FOUND)
	include_directories(${SPARSEHASH_INCLUDE_DIR})
endif(SPARSEHASH_FOUND)
if(SPARSEHASH_NEW_FOUND)
    include_directories(${SPARSEHASH_NEW_INCLUDE_DIR})
    # allow to build parts that depend on sparsehash
    set(SPARSEHASH_FOUND TRUE)
endif(SPARSEHASH_NEW_FOUND)

if(wxWidgets_FOUND)
    if(Linux)
        # Force the use of 32 bit library configuration on
        # 64 bits machine with 32 bits library in /usr/lib32
        if(CMAKE_SIZEOF_VOID_P MATCHES "8")
            ## There is no guarantee that wx-config is a link to a 32 bits library. So you need to force the destinity
            # Library can go into 3 path major paths (+ multiarch but you will see that later when implementation is done)
            # 1/ /usr/lib32 (32 bits only)
            # 2/ /usr/lib64 (64 bits only)
            # 3/ /usr/lib   (32 or 64 bits depends on distributions)
            if (EXISTS "/usr/lib32/wx")
                STRING(REGEX REPLACE "/usr/lib/wx" "/usr/lib32/wx" wxWidgets_INCLUDE_DIRS "${wxWidgets_INCLUDE_DIRS}")
                STRING(REGEX REPLACE "/usr/lib64/wx" "/usr/lib32/wx" wxWidgets_INCLUDE_DIRS "${wxWidgets_INCLUDE_DIRS}")
            endif (EXISTS "/usr/lib32/wx")
            if (EXISTS "/usr/lib/wx")
                STRING(REGEX REPLACE "/usr/lib64/wx" "/usr/lib/wx" wxWidgets_INCLUDE_DIRS "${wxWidgets_INCLUDE_DIRS}")
            endif (EXISTS "/usr/lib/wx")
            # Multiarch ubuntu/debian
            STRING(REGEX REPLACE "/usr/lib/x86_64-linux-gnu" "/usr/lib/i386-linux-gnu" wxWidgets_INCLUDE_DIRS "${wxWidgets_INCLUDE_DIRS}")
        endif(CMAKE_SIZEOF_VOID_P MATCHES "8")

		# Some people are trying to compile with wx 3.0 ...
		### 3.0
		# -I/usr/lib/i386-linux-gnu/wx/include/gtk2-unicode-3.0 -I/usr/include/wx-3.0 -D_FILE_OFFSET_BITS=64 -DWXUSINGDLL -D__WXGTK__ -pthread
		# -L/usr/lib/i386-linux-gnu -pthread   -lwx_gtk2u_xrc-3.0 -lwx_gtk2u_html-3.0 -lwx_gtk2u_qa-3.0 -lwx_gtk2u_adv-3.0 -lwx_gtk2u_core-3.0 -lwx_baseu_xml-3.0 -lwx_baseu_net-3.0 -lwx_baseu-3.0
		### 2.8
		# -I/usr/lib/i386-linux-gnu/wx/include/gtk2-unicode-release-2.8 -I/usr/include/wx-2.8 -D_FILE_OFFSET_BITS=64 -D_LARGE_FILES -D__WXGTK__ -pthread
		# -L/usr/lib/i386-linux-gnu -pthread -Wl,-z,relro  -L/usr/lib/i386-linux-gnu   -lwx_gtk2u_richtext-2.8 -lwx_gtk2u_aui-2.8 -lwx_gtk2u_xrc-2.8 -lwx_gtk2u_qa-2.8 -lwx_gtk2u_html-2.8 -lwx_gtk2u_adv-2.8 -lwx_gtk2u_core-2.8 -lwx_baseu_xml-2.8 -lwx_baseu_net-2.8 -lwx_baseu-2.8
		if ("${wxWidgets_INCLUDE_DIRS}" MATCHES "3.0")
			message(WARNING "\nWxwidget 3.0 is installed on your system whereas PCSX2 required 2.8 !!!\nPCSX2 will try to use 2.8 but if it would be better to fix your setup.\n")
			STRING(REGEX REPLACE "unicode" "unicode-release" wxWidgets_INCLUDE_DIRS "${wxWidgets_INCLUDE_DIRS}")
			STRING(REGEX REPLACE "3\\.0" "2.8" wxWidgets_INCLUDE_DIRS "${wxWidgets_INCLUDE_DIRS}")
			STRING(REGEX REPLACE "3\\.0" "2.8" wxWidgets_LIBRARIES "${wxWidgets_LIBRARIES}")
		endif()
    endif(Linux)

	include(${wxWidgets_USE_FILE})
endif(wxWidgets_FOUND)

if(ZLIB_FOUND)
	include_directories(${ZLIB_INCLUDE_DIRS})
endif(ZLIB_FOUND)
