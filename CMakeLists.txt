# Project Name
project(Pcsx2)

# There is some incompatible change with version 2.6 and below !
cmake_minimum_required(VERSION 2.8)

# Variable to check that people use the good file
set(TOP_CMAKE_WAS_SOURCED TRUE)

# Print a clear message that 64bits is not supported
# It would avoid compilation failure later.
if(CMAKE_SIZEOF_VOID_P MATCHES "8")
    message(FATAL_ERROR "
    PCSX2 does not support 64bits environment. Please install a 32bits chroot or a 32bits OS.
    PCSX2 have neither no plan to support the 64bits architecture in the future.
    It will need a complete rewrite of the core emulator and a lots of time
    
    However when linux distribution will support properly multi-arch package, it will 
    be at least possible to easily compile and install PCSX2 witout too much hassle (the chroot environment)")
endif(CMAKE_SIZEOF_VOID_P MATCHES "8")


# set module path
set(CMAKE_MODULE_PATH ${PROJECT_SOURCE_DIR}/cmake)

# include some generic functions
include(Pcsx2Utils)

# Detect current OS
detectOperatingSystem()

#-------------------------------------------------------------------------------
# Include specific module
# BuildParameters Must be done before SearchForStuff
include(BuildParameters)
# SearchForStuff be done before SelectPcsx2Plugins
include(SearchForStuff)
include(SelectPcsx2Plugins)

# add additional project-wide include directories
include_directories(${PROJECT_SOURCE_DIR}/common/include
					${PROJECT_SOURCE_DIR}/common/include/Utilities
					${PROJECT_SOURCE_DIR}/common/include/x86emitter)

# make 3rdParty
if(EXISTS "${PROJECT_SOURCE_DIR}/3rdparty")
    add_subdirectory(3rdparty)
endif(EXISTS "${PROJECT_SOURCE_DIR}/3rdparty")

# make common
if(common_libs)
    add_subdirectory(common/src/Utilities)
    add_subdirectory(common/src/x86emitter)
endif(common_libs)

# make tools
add_subdirectory(tools)

# make pcsx2
if(EXISTS "${PROJECT_SOURCE_DIR}/pcsx2" AND pcsx2_core)
    add_subdirectory(pcsx2)
endif(EXISTS "${PROJECT_SOURCE_DIR}/pcsx2" AND pcsx2_core)

# make plugins
if(EXISTS "${PROJECT_SOURCE_DIR}/plugins")
    add_subdirectory(plugins)
endif(EXISTS "${PROJECT_SOURCE_DIR}/plugins")

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

