### TODO 
# Hardcode GAMEINDEX_DIR, if default is fine for everybody

### Select the build type
# Use Release/Devel/Debug      : -DCMAKE_BUILD_TYPE=Release|Devel|Debug
# Enable/disable the stripping : -DCMAKE_BUILD_STRIP=TRUE|FALSE
# generation .po based on src  : -DCMAKE_BUILD_PO=TRUE|FALSE
# Rebuild the ps2hw.dat file   : -DREBUILD_SHADER=TRUE
# Build the Replay Loaders     : -DBUILD_REPLAY_LOADERS=TRUE|FALSE
# Use GLSL API(else NVIDIA_CG): -DGLSL_API=TRUE|FALSE
# Use EGL (vs GLX)            : -DEGL_API=TRUE|FALSE
# Use SDL2                    : -DSDL2_API=TRUE|FALSE
# Build all plugins           : -DEXTRA_PLUGINS=TRUE|FALSE

### GCC optimization options
# control C flags             : -DUSER_CMAKE_C_FLAGS="cflags"
# control C++ flags           : -DUSER_CMAKE_CXX_FLAGS="cxxflags"
# control link flags          : -DUSER_CMAKE_LD_FLAGS="ldflags"

### Packaging options
# Installation path           : -DPACKAGE_MODE=TRUE(follow FHS)|FALSE(local bin/)
# Plugin installation path    : -DPLUGIN_DIR="/usr/lib/pcsx2"
# GL Shader installation path : -DGLSL_SHADER_DIR="/usr/share/games/pcsx2"
# Game DB installation path   : -DGAMEINDEX_DIR="/usr/share/games/pcsx2"
# Follow XDG standard         : -DXDG_STD=TRUE|FALSE
#-------------------------------------------------------------------------------

#-------------------------------------------------------------------------------
# if no build type is set, use Devel as default
# Note without the CMAKE_BUILD_TYPE options the value is still defined to ""
# Ensure that the value set by the User is correct to avoid some bad behavior later
#-------------------------------------------------------------------------------
if(NOT CMAKE_BUILD_TYPE MATCHES "Debug|Devel|Release")
	set(CMAKE_BUILD_TYPE Devel)
	message(STATUS "BuildType set to ${CMAKE_BUILD_TYPE} by default")
endif(NOT CMAKE_BUILD_TYPE MATCHES "Debug|Devel|Release")

# Initially stip was disabled on release build but it is not stackstrace friendly!
# It only cost several MB so disbable it by default
if(NOT DEFINED CMAKE_BUILD_STRIP)
    set(CMAKE_BUILD_STRIP FALSE)
endif(NOT DEFINED CMAKE_BUILD_STRIP)

if(NOT DEFINED CMAKE_BUILD_PO)
    if(CMAKE_BUILD_TYPE STREQUAL "Release")
        set(CMAKE_BUILD_PO TRUE)
        message(STATUS "Enable the building of po files by default in ${CMAKE_BUILD_TYPE} build !!!")
    else(CMAKE_BUILD_TYPE STREQUAL "Release")
        set(CMAKE_BUILD_PO FALSE)
        message(STATUS "Disable the building of po files by default in ${CMAKE_BUILD_TYPE} build !!!")
    endif(CMAKE_BUILD_TYPE STREQUAL "Release")
endif(NOT DEFINED CMAKE_BUILD_PO)

#-------------------------------------------------------------------------------
# Control GCC flags
#-------------------------------------------------------------------------------
### Cmake set default value for various compilation variable
### Here the list of default value for documentation purpose
# ${CMAKE_SHARED_LIBRARY_CXX_FLAGS} = "-fPIC"
# ${CMAKE_SHARED_LIBRARY_LINK_CXX_FLAGS} = "-rdynamic"
# 
# ${CMAKE_C_FLAGS} = "-g -O2"
# ${CMAKE_CXX_FLAGS} = "-g -O2"
# Use in debug mode
# ${CMAKE_CXX_FLAGS_DEBUG} = "-g"
# Use in release mode
# ${CMAKE_CXX_FLAGS_RELEASE} = "-O3 -DNDEBUG"

#-------------------------------------------------------------------------------
# Do not use default cmake flags
#-------------------------------------------------------------------------------
set(CMAKE_C_FLAGS_DEBUG "")
set(CMAKE_CXX_FLAGS_DEBUG "")
set(CMAKE_C_FLAGS_DEVEL "")
set(CMAKE_CXX_FLAGS_DEVEL "")
set(CMAKE_C_FLAGS_RELEASE "")
set(CMAKE_CXX_FLAGS_RELEASE "")

#-------------------------------------------------------------------------------
# Remove bad default option
#-------------------------------------------------------------------------------
# Remove -rdynamic option that can some segmentation fault when openining pcsx2 plugins
set(CMAKE_SHARED_LIBRARY_LINK_C_FLAGS "")
set(CMAKE_SHARED_LIBRARY_LINK_CXX_FLAGS "")
# Remove -fPIC option. No good reason to use it for plugins. Moreover we
# only support x86 architecture. And last but not least it impact the performance.
# Long term future note :), amd64 build will need the -fPIC flags
set(CMAKE_SHARED_LIBRARY_C_FLAGS "")
set(CMAKE_SHARED_LIBRARY_CXX_FLAGS "")

#-------------------------------------------------------------------------------
# Set some default compiler flags
#-------------------------------------------------------------------------------
set(DEFAULT_WARNINGS "-Wno-write-strings -Wno-format -Wno-unused-parameter -Wno-unused-value -Wstrict-aliasing -Wno-unused-function -Wno-attributes -Wno-unused-result -Wno-missing-field-initializers -Wno-unused-local-typedefs -Wno-parentheses")
set(HARDEING_OPT "-D_FORTIFY_SOURCE=2  -Wformat -Wformat-security")
set(DEFAULT_GCC_FLAG "-m32 -msse -msse2 -march=i686 -pthread ${DEFAULT_WARNINGS} ${HARDEING_OPT}")
if(CMAKE_BUILD_TYPE MATCHES "Debug|Devel")
    set(DEFAULT_GCC_FLAG "-g ${DEFAULT_GCC_FLAG}")
endif()
set(DEFAULT_CPP_FLAG "${DEFAULT_GCC_FLAG} -Wno-invalid-offsetof")

#-------------------------------------------------------------------------------
# Allow user to set some default flags
# Note: string STRIP must be used to remove trailing and leading spaces.
#       See policy CMP0004
#-------------------------------------------------------------------------------
### linker flags
if(DEFINED USER_CMAKE_LD_FLAGS)
    message(STATUS "Pcsx2 is very sensible with gcc flags, so use USER_CMAKE_LD_FLAGS at your own risk !!!")
    string(STRIP "${USER_CMAKE_LD_FLAGS}" USER_CMAKE_LD_FLAGS)
else(DEFINED USER_CMAKE_LD_FLAGS)
    set(USER_CMAKE_LD_FLAGS "")
endif(DEFINED USER_CMAKE_LD_FLAGS)

# ask the linker to strip the binary
if(CMAKE_BUILD_STRIP) 
    string(STRIP "${USER_CMAKE_LD_FLAGS} -s" USER_CMAKE_LD_FLAGS)
endif(CMAKE_BUILD_STRIP) 


### c flags
# Note CMAKE_C_FLAGS is also send to the linker.
# By default allow build on amd64 machine
if(DEFINED USER_CMAKE_C_FLAGS)
    message(STATUS "Pcsx2 is very sensible with gcc flags, so use USER_CMAKE_C_FLAGS at your own risk !!!")
    string(STRIP "${USER_CMAKE_C_FLAGS}" CMAKE_C_FLAGS)
endif(DEFINED USER_CMAKE_C_FLAGS)
# Use some default machine flags
string(STRIP "${CMAKE_C_FLAGS} ${DEFAULT_GCC_FLAG}" CMAKE_C_FLAGS)


### C++ flags
# Note CMAKE_CXX_FLAGS is also send to the linker.
# By default allow build on amd64 machine
if(DEFINED USER_CMAKE_CXX_FLAGS)
    message(STATUS "Pcsx2 is very sensible with gcc flags, so use USER_CMAKE_CXX_FLAGS at your own risk !!!")
    string(STRIP "${USER_CMAKE_CXX_FLAGS}" CMAKE_CXX_FLAGS)
endif(DEFINED USER_CMAKE_CXX_FLAGS)
# Use some default machine flags
string(STRIP "${CMAKE_CXX_FLAGS} ${DEFAULT_CPP_FLAG}" CMAKE_CXX_FLAGS)

#-------------------------------------------------------------------------------
# Default package option
#-------------------------------------------------------------------------------
if(NOT DEFINED PACKAGE_MODE)
    set(PACKAGE_MODE FALSE)
endif(NOT DEFINED PACKAGE_MODE)

if(PACKAGE_MODE)
    if(NOT DEFINED PLUGIN_DIR)
        set(PLUGIN_DIR "${CMAKE_INSTALL_PREFIX}/lib/games/pcsx2")
    endif(NOT DEFINED PLUGIN_DIR)

    if(NOT DEFINED GAMEINDEX_DIR)
        set(GAMEINDEX_DIR "${CMAKE_INSTALL_PREFIX}/share/games/pcsx2")
    endif(NOT DEFINED GAMEINDEX_DIR)

    # Compile all source codes with these 3 defines
    add_definitions(-DPLUGIN_DIR_COMPILATION=${PLUGIN_DIR} -DGAMEINDEX_DIR_COMPILATION=${GAMEINDEX_DIR})
endif(PACKAGE_MODE)

#-------------------------------------------------------------------------------
# Select nvidia cg shader api by default (zzogl only)
#-------------------------------------------------------------------------------
if(NOT DEFINED GLSL_API)
	set(GLSL_API FALSE)
endif(NOT DEFINED GLSL_API)

#-------------------------------------------------------------------------------
# Select GLX API by default (zzogl only)
#-------------------------------------------------------------------------------
if(NOT DEFINED EGL_API)
    set(EGL_API FALSE)
else()
    message(STATUS "EGL is experimental and not expected to work yet!!!")
endif()

#-------------------------------------------------------------------------------
# Select opengl api by default (gsdx)
#-------------------------------------------------------------------------------
if(NOT DEFINED GLES_API)
    set(GLES_API FALSE)
endif()

#-------------------------------------------------------------------------------
# Select SDL1 by default (spu2x and onepad)
#-------------------------------------------------------------------------------
# FIXME do a proper detection
set(SDL2_LIBRARY "-lSDL2")
if(NOT DEFINED SDL2_API)
    set(SDL2_API FALSE)
endif()

#-------------------------------------------------------------------------------
# Use the precompiled shader file by default (both zzogl&gsdx)
#-------------------------------------------------------------------------------
if(NOT DEFINED REBUILD_SHADER)
	set(REBUILD_SHADER FALSE)
endif(NOT DEFINED REBUILD_SHADER)

#-------------------------------------------------------------------------------
# Build the replay loaders by default
#-------------------------------------------------------------------------------
if(NOT DEFINED BUILD_REPLAY_LOADERS)
	set(BUILD_REPLAY_LOADERS TRUE)
endif(NOT DEFINED BUILD_REPLAY_LOADERS)

#-------------------------------------------------------------------------------
# Use PCSX2 default path (not XDG)
#-------------------------------------------------------------------------------
if (NOT DEFINED XDG_STD)
    set(XDG_STD FALSE)
endif (NOT DEFINED XDG_STD)

#-------------------------------------------------------------------------------
# Use only main plugin (faster compilation time)
#-------------------------------------------------------------------------------
if (NOT DEFINED EXTRA_PLUGINS)
    set(EXTRA_PLUGINS FALSE)
endif()
