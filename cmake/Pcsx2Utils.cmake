# additonal cmake macros and functions

#-------------------------------------------------------------------------------
#						detectOperatingSystem
#-------------------------------------------------------------------------------
# This function detects on which OS cmake is run and set a flag to control the
# build process. Supported OS: Linux, MacOSX, Windows
#-------------------------------------------------------------------------------
function(detectOperatingSystem)
	# nothing detected yet
	set(Linux FALSE PARENT_SCOPE)
	set(MacOSX FALSE PARENT_SCOPE)
	set(Windows FALSE PARENT_SCOPE)
	
	# check if we are on Linux
	if(${CMAKE_SYSTEM_NAME} STREQUAL "Linux")
		set(Linux TRUE PARENT_SCOPE)
	endif(${CMAKE_SYSTEM_NAME} STREQUAL "Linux")

	# check if we are on MacOSX
	if(APPLE)
		set(MacOSX TRUE PARENT_SCOPE)
	endif(APPLE)

	# check if we are on Windows
	if(${CMAKE_SYSTEM_NAME} STREQUAL "Windows")
		set(Windows TRUE PARENT_SCOPE)
	endif(${CMAKE_SYSTEM_NAME} STREQUAL "Windows")
endfunction(detectOperatingSystem)
#-------------------------------------------------------------------------------

