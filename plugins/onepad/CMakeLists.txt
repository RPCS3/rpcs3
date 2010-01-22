# onepad Plugin

# plugin name
set(onepadName onepad)

# Debug - Build
if(CMAKE_BUILD_TYPE STREQUAL Debug)
	# add defines
	add_definitions(-Wall -fPIC -m32 -g)
endif(CMAKE_BUILD_TYPE STREQUAL Debug)

# Devel - Build
if(CMAKE_BUILD_TYPE STREQUAL Devel)
	# add defines
	add_definitions(-Wall -fPIC -m32 -O2)
endif(CMAKE_BUILD_TYPE STREQUAL Devel)

# Release - Build
if(CMAKE_BUILD_TYPE STREQUAL Release)
	# add defines
	add_definitions(-Wall -fPIC -m32 -O2 -s)
endif(CMAKE_BUILD_TYPE STREQUAL Release)

# onepad sources
set(onepadSources
	analog.cpp
	controller.cpp
	joystick.cpp
	keyboard.cpp
	onepad.cpp)
	
# onepad headers
set(onepadHeaders
	analog.h
	bitwise.h
	controller.h
	joystick.h
	keyboard.h
	onepad.h)

# onepad Linux sources
set(onepadLinuxSources
#	Linux/callbacks.c
	Linux/gui.cpp
	Linux/ini.cpp
	Linux/interface.c
	Linux/linux.cpp
	Linux/support.c)

# onepad Linux headers
set(onepadLinuxHeaders
	Linux/callbacks.h
	Linux/interface.h
	Linux/support.h)

# onepad Windows sources
set(onepadWindowsSources
	)

# onepad Windows headers
set(onepadWindowsHeaders
	)

# add additional include directories
include_directories(.)

# add library
add_library(${onepadName} SHARED 
			${onepadSources}
			${onepadHeaders}
			${onepadLinuxSources}
			${onepadLinuxHeaders})

# set output directory
set_target_properties(${onepadName} PROPERTIES
					  LIBRARY_OUTPUT_DIRECTORY ${PROJECT_SOURCE_DIR}/bin/plugins)

# link target with SDL
target_link_libraries(${onepadName} ${SDL_LIBRARY})

