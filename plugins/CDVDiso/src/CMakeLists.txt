# CDVDiso Plugin

# plugin name
set(CDVDisoName CDVDiso)

set(CommonFlags
    -Wall
    -m32
    -fpic
    -fpermissive
    )

set(OptimizationFlags
    -O2
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

# CDVDiso sources
set(CDVDisoSources
    CDVDisop.cpp
    libiso.cpp)

# CDVDiso headers
set(CDVDisoHeaders
    CDVDiso.h
    libiso.h)

# CDVDiso Linux sources
set(CDVDisoLinuxSources
    Linux/Config.cpp
    Linux/interface.c
    Linux/Linux.cpp
    Linux/support.c)

# CDVDiso Linux headers
set(CDVDisoLinuxHeaders
    Linux/callbacks.h
    Linux/Config.h
    Linux/interface.h
    Linux/support.h)


# add additional include directories
include_directories(.
    Linux)

# add library
add_library(${CDVDisoName} SHARED
			${CDVDisoSources}
			${CDVDisoHeaders}
			${CDVDisoLinuxSources}
			${CDVDisoLinuxHeaders})

# set output directory
set_target_properties(${CDVDisoName} PROPERTIES
					  LIBRARY_OUTPUT_DIRECTORY ${PROJECT_SOURCE_DIR}/bin/plugins)

# Link with bz2
target_link_libraries(${CDVDisoName} ${BZIP2_LIBRARIES})

# Force the linker into 32 bits mode
target_link_libraries(${CDVDisoName} -m32)

# Linker strip option
if (CMAKE_BUILD_STRIP)
    target_link_libraries(${CDVDisoName} -s)
endif (CMAKE_BUILD_STRIP)
