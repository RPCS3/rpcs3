
# Project Name
project(Pcsx2)

# need cmake version >=2.6
cmake_minimum_required(VERSION 2.6)

# include modules
include(${PROJECT_SOURCE_DIR}/cmake/Pcsx2Utils.cmake)
include(${PROJECT_SOURCE_DIR}/cmake/SearchForStuff.cmake)

#-------------------------------------------------------------------------------
# if no build type is set, use Debug as default
#-------------------------------------------------------------------------------
if(CMAKE_BUILD_TYPE STREQUAL "")
	set(CMAKE_BUILD_TYPE Debug)
	message(STATUS "BuildType set to Debug!!! by default")
endif(CMAKE_BUILD_TYPE STREQUAL "")
#-------------------------------------------------------------------------------

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
# This target is used to convert all binary Resources to c/c++-Header files.
# It is always considered as out-of-date, so it is build every time.
#-------------------------------------------------------------------------------
# add resources here
set(resourceList AppIcon16.png
				 AppIcon32.png
				 AppIcon64.png
				 BackgroundLogo.png
				 ButtonIcon_Camera.png
				 ConfigIcon_Cpu.png
				 ConfigIcon_Gamefixes.png
				 ConfigIcon_Paths.png
				 ConfigIcon_Plugins.png
				 ConfigIcon_Speedhacks.png
				 ConfigIcon_Video.png
				 Dualshock.jpg)
				 
createResourceTarget(${resourceList})
#-------------------------------------------------------------------------------

