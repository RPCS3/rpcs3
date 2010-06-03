
# Project Name
project(Pcsx2)

# need cmake version >=2.6
cmake_minimum_required(VERSION 2.6)

#-------------------------------------------------------------------------------
# Detect the 32bit/64bit and set the 32bit library path 32_LD_LIBRARY_PATH
# Note: Must be done before SearchForStuff
#-------------------------------------------------------------------------------
if (UNIX)
    if (CMAKE_SIZEOF_VOID_P MATCHES "8" AND EXISTS "/usr/lib32")
        # 64 bits machine with 32 bits library in /usr/lib32
            set(32_LD_LIBRARY_PATH /usr/lib32)
    else (CMAKE_SIZEOF_VOID_P MATCHES "8" AND EXISTS "/usr/lib32")
        # default case
        set(32_LD_LIBRARY_PATH /usr/lib)
    endif (CMAKE_SIZEOF_VOID_P MATCHES "8" AND EXISTS "/usr/lib32")
endif (UNIX)

# set module path
set(CMAKE_MODULE_PATH ${PROJECT_SOURCE_DIR}/cmake)

#-------------------------------------------------------------------------------
# include modules
include(Pcsx2Utils)
# BuildParameters Must be done before SearchForStuff
include(BuildParameters)
# SearchForStuff be done before SelectPcsx2Plugins
include(SearchForStuff)
include(SelectPcsx2Plugins)

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

