# onepad Plugin

# plugin name
set(onepadName onepad)

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

# add library
add_library(${onepadName} SHARED 
			${onepadSources}
			${onepadHeaders}
			${onepadLinuxSources}
			${onepadLinuxHeaders})

# set output directory
set_target_properties(${onepadName} PROPERTIES
					  LIBRARY_OUTPUT_DIRECTORY ${PROJECT_SOURCE_DIR}/bin/plugins)

# Debug - Build
if(CMAKE_BUILD_TYPE STREQUAL Debug)
	set(CMAKE_C_FLAGS_DEBUG "-Wall -fPIC -m32 -g")
	set(CMAKE_CXX_FLAGS_DEBUG "-Wall -fPIC -m32 -g")
	
	# add defines
	add_definitions()
endif(CMAKE_BUILD_TYPE STREQUAL Debug)

# Devel - Build
if(CMAKE_BUILD_TYPE STREQUAL Devel)
	set(CMAKE_C_FLAGS_DEVEL "-Wall -fPIC -m32 -O2")
	set(CMAKE_CXX_FLAGS_DEVEL "-Wall -fPIC -m32 -O2")
	
	# add defines
	add_definitions()
endif(CMAKE_BUILD_TYPE STREQUAL Devel)

# Release - Build
if(CMAKE_BUILD_TYPE STREQUAL Release)
	set(CMAKE_C_FLAGS_RELEASE "-Wall -fPIC -m32 -O2 -s")
	set(CMAKE_CXX_FLAGS_RELEASE "-Wall -fPIC -m32 -O2 -s")
	
	# add defines
	add_definitions()
endif(CMAKE_BUILD_TYPE STREQUAL Release)

# link target with wx
target_link_libraries(${onepadName} ${SDL_LIBRARY})

# add additional include directories
include_directories(.
					../../common/include)

