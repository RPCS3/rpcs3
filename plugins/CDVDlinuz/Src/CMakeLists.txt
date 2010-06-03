# CDVDlinuz Plugin

# plugin name
set(CDVDlinuzName CDVDlinuz)

set(CommonFlags
    -Wall
    -m32
    -fPIC
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
    # Either fix include dir. Or rename the file. Or mv stuff into in another include file.
    version.h
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
add_library(${CDVDlinuzName} SHARED
    ${CDVDlinuzSources}
    ${CDVDlinuzHeaders}
    ${CDVDlinuzLinuxSources}
    ${CDVDlinuzLinuxHeaders})

# set output directory
set_target_properties(${CDVDlinuzName} PROPERTIES
    LIBRARY_OUTPUT_DIRECTORY ${PROJECT_SOURCE_DIR}/bin/plugins)

# Set link flags
target_link_libraries(${CDVDlinuzName} -m32)

# Linker strip option
if (CMAKE_BUILD_STRIP)
    target_link_libraries(${CDVDlinuzName} -s)
endif (CMAKE_BUILD_STRIP)
