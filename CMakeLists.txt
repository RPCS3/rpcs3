
# Project Name
project(Pcsx2)

# need cmake version >=2.6
cmake_minimum_required(VERSION 2.6)

#-------------------------------------------------------------------------------
# Detect the 32bit/64bit and set the 32bit library path 32_LD_LIBRARY_PATH
#-------------------------------------------------------------------------------
if (UNIX)
    if (CMAKE_SIZEOF_VOID_P MATCHES "8")
        # 64 bits machine
        if (EXISTS /usr/lib32)
            # 32 bits library in /usr/lib32
            set(32_LD_LIBRARY_PATH /usr/lib32)
        else (EXISTS /usr/lib32)
            set(32_LD_LIBRARY_PATH /usr/lib)
        endif (EXISTS /usr/lib32)
    else (CMAKE_SIZEOF_VOID_P MATCHES "8")
        # 32 bits machine
        set(32_LD_LIBRARY_PATH /usr/lib)
    endif (CMAKE_SIZEOF_VOID_P MATCHES "8")
endif (UNIX)

# set module path
set(CMAKE_MODULE_PATH ${PROJECT_SOURCE_DIR}/cmake)

#-------------------------------------------------------------------------------
# if no build type is set, use Devel as default
#-------------------------------------------------------------------------------
if(CMAKE_BUILD_TYPE STREQUAL "")
	set(CMAKE_BUILD_TYPE Devel)
	message(STATUS "BuildType set to ${CMAKE_BUILD_TYPE} by default")
endif(CMAKE_BUILD_TYPE STREQUAL "")
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
 
# Force the choice of 3rd party library either system one or pcsx2 one
# Use all         internal lib: -DFORCE_INTERNAL_ALL=TRUE
# Use a52         internal lib: -DFORCE_INTERNAL_A52=TRUE
# Use bzip        internal lib: -DFORCE_INTERNAL_BZIP2=TRUE
# Use soundtouch  internal lib: -DFORCE_INTERNAL_SOUNDTOUCH=TRUE
# Use zlib        internal lib: -DFORCE_INTERNAL_ZLIB=TRUE
#-------------------------------------------------------------------------------

# There have been issues in the past with sound quality depending on the version
# of Soundtouch, so for the moment, we'll use the internal version.
set(FORCE_INTERNAL_SOUNDTOUCH TRUE)

if(FORCE_INTERNAL_ALL)
    set(FORCE_INTERNAL_A52 TRUE)
    set(FORCE_INTERNAL_BZIP2 TRUE)
    set(FORCE_INTERNAL_SOUNDTOUCH TRUE)
    set(FORCE_INTERNAL_ZLIB TRUE)
endif(FORCE_INTERNAL_ALL)

if(NOT DEFINED FORCE_INTERNAL_A52)
    set(FORCE_INTERNAL_A52 FALSE)
endif(NOT DEFINED FORCE_INTERNAL_A52)

if(NOT DEFINED FORCE_INTERNAL_BZIP2)
    set(FORCE_INTERNAL_BZIP2 FALSE)
endif(NOT DEFINED FORCE_INTERNAL_BZIP2)

if(NOT DEFINED FORCE_INTERNAL_SOUNDTOUCH)
    set(FORCE_INTERNAL_SOUNDTOUCH FALSE)
endif(NOT DEFINED FORCE_INTERNAL_SOUNDTOUCH)

if(NOT DEFINED FORCE_INTERNAL_ZLIB)
    set(FORCE_INTERNAL_ZLIB FALSE)
endif(NOT DEFINED FORCE_INTERNAL_ZLIB)

#-------------------------------------------------------------------------------
# include modules
include(Pcsx2Utils)
include(SearchForStuff)

# add additional project-wide include directories
include_directories(${PROJECT_SOURCE_DIR}/3rdparty
					${PROJECT_SOURCE_DIR}/common/include
					${PROJECT_SOURCE_DIR}/common/include/Utilities
					${PROJECT_SOURCE_DIR}/common/include/x86emitter)

# make 3rdParty
add_subdirectory(3rdparty)

# make common
add_subdirectory(common)

# make tools
add_subdirectory(tools)

# make pcsx2
add_subdirectory(pcsx2)

# make plugins
add_subdirectory(plugins)

#-------------------------------------------------------------------------------
#								Resources
#-------------------------------------------------------------------------------
# Specify all binary images to convert them into c/c++ header files.
#
#-------------------------------------------------------------------------------
# add resources here
set(resourceList AppIcon16.png
				 AppIcon32.png
				 AppIcon64.png
				 BackgroundLogo.png
				 ButtonIcon_Camera.png
				 ConfigIcon_Cpu.png
				 ConfigIcon_Gamefixes.png
				 ConfigIcon_MemoryCard.png
				 ConfigIcon_Paths.png
				 ConfigIcon_Plugins.png
				 ConfigIcon_Speedhacks.png
				 ConfigIcon_Video.png
				 Dualshock.jpg)
				 
createResourceTarget(${resourceList})
#-------------------------------------------------------------------------------

