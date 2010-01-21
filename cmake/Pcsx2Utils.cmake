# additonal cmake macros and functions

#-------------------------------------------------------------------------------
#						createResourceTarget
#-------------------------------------------------------------------------------
# This function is used to generate header files, used as resources.
# A list of Resources taken as input parameter.
#-------------------------------------------------------------------------------
function(createResourceTarget)
	# working directory
	set(workdir ${PROJECT_SOURCE_DIR}/pcsx2/gui/Resources)

	# create dummy target depending on bin2cpp
	add_custom_target(Resources
					  DEPENDS bin2cpp)

	# create a custom command for every resource file
	foreach(entry IN LISTS ARGV)
		# create custom command and assign to target Resources
		add_custom_command(TARGET Resources POST_BUILD
						   COMMAND bin2cpp ${entry}
	   				       WORKING_DIRECTORY ${workdir})
	endforeach(entry)
endfunction(createResourceTarget)
#-------------------------------------------------------------------------------


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

