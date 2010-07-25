# Check that people use the good file
if(NOT TOP_CMAKE_WAS_SOURCED)
    message(FATAL_ERROR "
    You did not 'cmake' the good CMakeLists.txt file. Use the one in the top dir.
    It is advice to delete all wrongly generated cmake stuff => CMakeFiles & CMakeCache.txt")
endif(NOT TOP_CMAKE_WAS_SOURCED)


# plugin name
set(Output zeropad)

set(CommonFlags
    -Wall
    -fpic
    )

set(OptimizationFlags
    -O2
    -DNDEBUG
    -fomit-frame-pointer
    )


# Debug - Build
if(CMAKE_BUILD_TYPE STREQUAL Debug)
	# add defines
	add_definitions(${CommonFlags} -g)
endif(CMAKE_BUILD_TYPE STREQUAL Debug)

# Devel - Build
if(CMAKE_BUILD_TYPE STREQUAL Devel)
	# add defines
	add_definitions(${CommonFlags} ${OptimizationFlags})
endif(CMAKE_BUILD_TYPE STREQUAL Devel)

# Release - Build
if(CMAKE_BUILD_TYPE STREQUAL Release)
	# add defines
	add_definitions(${CommonFlags} ${OptimizationFlags})
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
add_library(${Output} SHARED
			${zeropadSources}
			${zeropadHeaders}
			${zeropadLinuxSources}
			${zeropadLinuxHeaders})

# set output directory
set_target_properties(${Output} PROPERTIES
					  LIBRARY_OUTPUT_DIRECTORY ${PROJECT_SOURCE_DIR}/bin/plugins)

# link target with SDL
target_link_libraries(${Output} ${SDL_LIBRARY})

# User flags options
if(NOT USER_CMAKE_LD_FLAGS STREQUAL "")
    target_link_libraries(${Output} "${USER_CMAKE_LD_FLAGS}")
endif(NOT USER_CMAKE_LD_FLAGS STREQUAL "")
