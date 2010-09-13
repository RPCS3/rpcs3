### Select the build type
# Use Release/Devel/Debug     : -DCMAKE_BUILD_TYPE=Release|Devel|Debug
# Enable/disable the stipping : -DCMAKE_BUILD_STRIP=TRUE|FALSE
### Force the choice of 3rd party library in pcsx2 over system libraries
# Use all         internal lib: -DFORCE_INTERNAL_ALL=TRUE
# Use soundtouch  internal lib: -DFORCE_INTERNAL_SOUNDTOUCH=TRUE
# Use zlib        internal lib: -DFORCE_INTERNAL_ZLIB=TRUE
### Add some flags to the build process
# control C flags             : -DUSER_CMAKE_C_FLAGS="cflags"
# control C++ flags           : -DUSER_CMAKE_CXX_FLAGS="cxxflags"
# control link flags          : -DUSER_CMAKE_LD_FLAGS="ldflags"
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
#-------------------------------------------------------------------------------

#-------------------------------------------------------------------------------
# Set default strip option. Can be set with -DCMAKE_BUILD_STRIP=TRUE/FALSE
#-------------------------------------------------------------------------------
if(NOT DEFINED CMAKE_BUILD_STRIP)
    if(CMAKE_BUILD_TYPE STREQUAL "Release")
        set(CMAKE_BUILD_STRIP TRUE)
        message(STATUS "Enable the stripping by default in ${CMAKE_BUILD_TYPE} build !!!")
    else(CMAKE_BUILD_TYPE STREQUAL "Release")
        set(CMAKE_BUILD_STRIP FALSE)
        message(STATUS "Disable the stripping by default in ${CMAKE_BUILD_TYPE} build !!!")
    endif(CMAKE_BUILD_TYPE STREQUAL "Release")
endif(NOT DEFINED CMAKE_BUILD_STRIP)

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
string(STRIP "${CMAKE_C_FLAGS} -m32 -msse -msse2 -march=i686 -pthread" CMAKE_C_FLAGS)


### C++ flags
# Note CMAKE_CXX_FLAGS is also send to the linker.
# By default allow build on amd64 machine
if(DEFINED USER_CMAKE_CXX_FLAGS)
    message(STATUS "Pcsx2 is very sensible with gcc flags, so use USER_CMAKE_CXX_FLAGS at your own risk !!!")
    string(STRIP "${USER_CMAKE_CXX_FLAGS}" CMAKE_CXX_FLAGS)
endif(DEFINED USER_CMAKE_CXX_FLAGS)
# Use some default machine flags
string(STRIP "${CMAKE_CXX_FLAGS} -m32 -msse -msse2 -march=i686 -pthread" CMAKE_CXX_FLAGS)

#-------------------------------------------------------------------------------
# Select library system vs 3rdparty
#-------------------------------------------------------------------------------
if(FORCE_INTERNAL_ALL)
    set(FORCE_INTERNAL_SOUNDTOUCH TRUE)
    set(FORCE_INTERNAL_ZLIB TRUE)
endif(FORCE_INTERNAL_ALL)

if(NOT DEFINED FORCE_INTERNAL_SOUNDTOUCH)
    set(FORCE_INTERNAL_SOUNDTOUCH TRUE)
    message(STATUS "Use internal version of Soundtouch by default.
    Note: There have been issues in the past with sound quality depending on the version of Soundtouch
    Use -DFORCE_INTERNAL_SOUNDTOUCH=FALSE at your own risk")
    # set(FORCE_INTERNAL_SOUNDTOUCH FALSE)
endif(NOT DEFINED FORCE_INTERNAL_SOUNDTOUCH)

if(NOT DEFINED FORCE_INTERNAL_ZLIB)
    set(FORCE_INTERNAL_ZLIB FALSE)
endif(NOT DEFINED FORCE_INTERNAL_ZLIB)
