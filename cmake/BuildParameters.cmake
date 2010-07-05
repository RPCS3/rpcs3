### Select the build type
# Use Release/Devel/Debug     : -DCMAKE_BUILD_TYPE=Release|Devel|Debug
# Enable/disable the stipping : -DCMAKE_BUILD_STRIP=TRUE|FALSE
### Force the choice of 3rd party library in pcsx2 over system libraries
# Use all         internal lib: -DFORCE_INTERNAL_ALL=TRUE
# Use soundtouch  internal lib: -DFORCE_INTERNAL_SOUNDTOUCH=TRUE
# Use zlib        internal lib: -DFORCE_INTERNAL_ZLIB=TRUE
#-------------------------------------------------------------------------------

### Cmake set default value for various compilation variable
### Here the list of default value for documentation purpose
# ${CMAKE_SHARED_LIBRARY_CXX_FLAGS} = "-fPIC"
# ${CMAKE_SHARED_LIBRARY_LINK_CXX_FLAGS} = "-rdynamic"
# 
# Use in debug mode
# ${CMAKE_CXX_FLAGS_DEBUG} = "-g"
# Use in release mode
# ${CMAKE_CXX_FLAGS_RELEASE} = "-O3 -DNDEBUG"

#-------------------------------------------------------------------------------
# Remove bad default option
#-------------------------------------------------------------------------------
# Remove -rdynamic option that can some segmentation fault when openining pcsx2 plugins
SET(CMAKE_SHARED_LIBRARY_LINK_C_FLAGS " ")
SET(CMAKE_SHARED_LIBRARY_LINK_CXX_FLAGS " ")
# Remove -fPIC option. No good reason to use it for plugins. Moreover we
# only support x86 architecture. And last but not least it impact the performance.
set(CMAKE_SHARED_LIBRARY_C_FLAGS "")
set(CMAKE_SHARED_LIBRARY_CXX_FLAGS "")

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
