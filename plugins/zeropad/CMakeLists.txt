# zeropad Plugin

# plugin name
set(zeropadName zeropad)

# Debug - Build
if(CMAKE_BUILD_TYPE STREQUAL Debug)
	# add defines
	add_definitions(-Wall -m32 -g -fpic)
endif(CMAKE_BUILD_TYPE STREQUAL Debug)

# Devel - Build
if(CMAKE_BUILD_TYPE STREQUAL Devel)
	# add defines
	add_definitions(-Wall -m32 -O2 -fpic -fomit-frame-pointer)
endif(CMAKE_BUILD_TYPE STREQUAL Devel)

# Release - Build
if(CMAKE_BUILD_TYPE STREQUAL Release)
	# add defines
	add_definitions(-Wall -m32 -O2 -fpic -fomit-frame-pointer)
endif(CMAKE_BUILD_TYPE STREQUAL Release)

# zeropad sources
set(zeropadSources
    analog.cpp
    joystick.cpp
    keyboard.cpp
    zeropad.cpp)

# zeropad headers
set(zeropadHeaders
    analog.h
    bitwise.h
    joystick.h
    keyboard.h
    zeropad.h)

# zeropad Linux sources
set(zeropadLinuxSources
    Linux/gui.cpp
    Linux/linux.cpp
    # Linux/callbacks.c
    Linux/interface.c
    Linux/support.c)

# zeropad Linux headers
set(zeropadLinuxHeaders
    Linux/callbacks.h
    Linux/interface.h
    Linux/linux.h
    Linux/support.h)

# zeropad Windows sources
set(zeropadWindowsSources
    Windows/gui.cpp
    Windows/win.cpp)

# zeropad Windows headers
set(zeropadWindowsHeaders
    Windows/resource.h
    Windows/win.h)

# add additional include directories
include_directories(.
    Linux)

# add library
add_library(${zeropadName} SHARED
			${zeropadSources}
			${zeropadHeaders}
			${zeropadLinuxSources}
			${zeropadLinuxHeaders})

# set output directory
set_target_properties(${zeropadName} PROPERTIES
					  LIBRARY_OUTPUT_DIRECTORY ${PROJECT_SOURCE_DIR}/bin/plugins)

# link target with SDL
target_link_libraries(${zeropadName} ${SDL_LIBRARY})

# Force the linker into 32 bits mode
target_link_libraries(${zeropadName} -m32)
 
# Linker strip option
if (CMAKE_BUILD_STRIP)
    target_link_libraries(${zeropadName} -s)
endif (CMAKE_BUILD_STRIP)
