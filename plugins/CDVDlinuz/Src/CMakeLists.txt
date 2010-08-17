# CDVDlinuz Plugin

# plugin name
set(Output CDVDlinuz)

set(CommonFlags
    -Wall
    -D_LARGEFILE64_SOURCE
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

# CDVDlinuz sources
set(CDVDlinuzSources
    buffer.c
    convert.c
    ini.c
    version.c)

# CDVDlinuz headers
set(CDVDlinuzHeaders
    buffer.h
    convert.h
    ini.h
    PS2Edefs.h
    PS2Etypes.h
    # ERROR It takes /usr/include/alsa/version.h
    # The content was move to Linux/CDVDlinuz.h
    # version.h
)

# CDVDlinuz Linux sources
set(CDVDlinuzLinuxSources
    Linux/aboutbox.c
    Linux/actualfile.c
    Linux/CD.c
    Linux/CDVDlinuz.c
    Linux/conf.c
    Linux/device.c
    Linux/DVD.c
    Linux/interface.c
    Linux/logfile.c
    Linux/mainbox.c)

# CDVDlinuz Linux headers
set(CDVDlinuzLinuxHeaders
    Linux/aboutbox.h
    Linux/actualfile.h
    Linux/CD.h
    Linux/CDVDlinuz.h
    Linux/conf.h
    Linux/device.h
    Linux/DVD.h
    Linux/logfile.h
    Linux/mainbox.h)

# add additional include directories
include_directories(.
    Linux)

# add library
add_library(${Output} SHARED
    ${CDVDlinuzSources}
    ${CDVDlinuzHeaders}
    ${CDVDlinuzLinuxSources}
    ${CDVDlinuzLinuxHeaders})

# set output directory
set_target_properties(${Output} PROPERTIES
    LIBRARY_OUTPUT_DIRECTORY ${PROJECT_SOURCE_DIR}/bin/plugins)

# User flags options
if(NOT USER_CMAKE_LD_FLAGS STREQUAL "")
    target_link_libraries(${Output} "${USER_CMAKE_LD_FLAGS}")
endif(NOT USER_CMAKE_LD_FLAGS STREQUAL "")
