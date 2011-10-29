# Check that people use the good file
if(NOT TOP_CMAKE_WAS_SOURCED)
    message(FATAL_ERROR "
    You did not 'cmake' the good CMakeLists.txt file. Use the one in the top dir.
    It is advice to delete all wrongly generated cmake stuff => CMakeFiles & CMakeCache.txt")
endif(NOT TOP_CMAKE_WAS_SOURCED)


# plugin name
set(Output GSnull)

set(CommonFlags
    -fvisibility=hidden
    -Wall
    -Wno-write-strings
    )

set(OptimizationFlags
    -O2
    -DNDEBUG
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

# GSnull sources
set(GSnullSources
	GifTransfer.cpp
	GS.cpp
	Registers.cpp)

# GSnull headers
set(GSnullHeaders
	GifTransfer.h
	GS.h
	Registers.h)

# GSnull Linux sources
set(GSnullLinuxSources
	Linux/Config.cpp
	Linux/GSLinux.cpp
	Linux/Linux.cpp)

# GSnull Linux headers
set(GSnullLinuxHeaders
	Linux/Config.h
	Linux/GSLinux.h
	Linux/Linux.h)

# GSnull null sources
set(GSnullnullSources
	null/GSnull.cpp)

# GSnull null headers
set(GSnullnullHeaders
	null/GSnull.h)

# GSnull Windows sources
set(GSnullWindowsSources
	Windows/Config.cpp
	Windows/GS.def
	Windows/GS.rc
	Windows/GSwin.cpp
	Windows/Win32.cpp)

# GSnull Windows headers
set(GSnullWindowsHeaders
	)

# add additional include directories
include_directories(.)

# add library
add_library(${Output} SHARED
			${GSnullSources}
			${GSnullHeaders}
			${GSnullnullSources}
			${GSnullnullHeaders}
			${GSnullLinuxSources}
			${GSnullLinuxHeaders})

# User flags options
if(NOT USER_CMAKE_LD_FLAGS STREQUAL "")
    target_link_libraries(${Output} "${USER_CMAKE_LD_FLAGS}")
endif(NOT USER_CMAKE_LD_FLAGS STREQUAL "")

if(PACKAGE_MODE)
    install(TARGETS ${Output} DESTINATION ${PLUGIN_DIR})
else(PACKAGE_MODE)
    install(TARGETS ${Output} DESTINATION ${CMAKE_SOURCE_DIR}/bin/plugins)
endif(PACKAGE_MODE)
