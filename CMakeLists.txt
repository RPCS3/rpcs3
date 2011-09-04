# Project Name
project(Pcsx2)

# There is some incompatible change with version 2.6 and below !
cmake_minimum_required(VERSION 2.8)

# Variable to check that people use the good file
set(TOP_CMAKE_WAS_SOURCED TRUE)

# Print a clear message that 64bits is not supported
# It would avoid compilation failure later.
# Note: disable the failure in package mode
if(NOT PACKAGE_MODE)
    if(CMAKE_SIZEOF_VOID_P MATCHES "8")
        if (EXISTS /etc/fedora-release)
            # Fedora users can install all .i686
            message(STATUS "
            Please ensure that you have all 32bits dependency installed (.i686 packages).
            See http://code.google.com/p/pcsx2/wiki/ChrootAnd64bStatusLinux for more details.")
        else (EXISTS /etc/fedora-release)
        message(FATAL_ERROR "
        PCSX2 does not support 64bits environment. Please install a 32bits chroot or a 32bits OS.
        PCSX2 have neither no plan to support the 64bits architecture in the future.
        It will need a complete rewrite of the core emulator and a lots of time

        However when linux distribution will support properly multi-arch package, it will
        be at least possible to easily compile and install PCSX2 witout too much hassle (the chroot environment)")
        endif (EXISTS /etc/fedora-release)
    endif(CMAKE_SIZEOF_VOID_P MATCHES "8")
endif(NOT PACKAGE_MODE)

# 64 bits specific configuration
if(CMAKE_SIZEOF_VOID_P MATCHES "8")
    # Do not search library in /usr/lib64
    SET_PROPERTY(GLOBAL PROPERTY FIND_LIBRARY_USE_LIB64_PATHS OFF)
    # Probably useless but it will not harm
    SET_PROPERTY(GLOBAL PROPERTY COMPILE_DEFINITIONS "-m32")
endif(CMAKE_SIZEOF_VOID_P MATCHES "8")

# set module path
set(CMAKE_MODULE_PATH ${PROJECT_SOURCE_DIR}/cmake)

# include some generic functions
include(Pcsx2Utils)

# Detect current OS
detectOperatingSystem()

#-------------------------------------------------------------------------------
# Include specific module
# BuildParameters Must be done before SearchForStuff
include(BuildParameters)
# SearchForStuff be done before SelectPcsx2Plugins
include(SearchForStuff)
include(SelectPcsx2Plugins)

# add additional project-wide include directories
include_directories(${PROJECT_SOURCE_DIR}/common/include
					${PROJECT_SOURCE_DIR}/common/include/Utilities
					${PROJECT_SOURCE_DIR}/common/include/x86emitter)

# make the translation
if(EXISTS "${PROJECT_SOURCE_DIR}/locales")
    add_subdirectory(locales)
endif(EXISTS "${PROJECT_SOURCE_DIR}/locales")

# make 3rdParty
if(EXISTS "${PROJECT_SOURCE_DIR}/3rdparty")
    add_subdirectory(3rdparty)
endif(EXISTS "${PROJECT_SOURCE_DIR}/3rdparty")

# make common
if(common_libs)
    add_subdirectory(common/src/Utilities)
    add_subdirectory(common/src/x86emitter)
endif(common_libs)

# make tools
add_subdirectory(tools)

# make pcsx2
if(EXISTS "${PROJECT_SOURCE_DIR}/pcsx2" AND pcsx2_core)
    add_subdirectory(pcsx2)
endif(EXISTS "${PROJECT_SOURCE_DIR}/pcsx2" AND pcsx2_core)

# make plugins
if(EXISTS "${PROJECT_SOURCE_DIR}/plugins")
    add_subdirectory(plugins)
endif(EXISTS "${PROJECT_SOURCE_DIR}/plugins")

#-------------------------------------------------------------------------------
# Install some files to ease package creation
if(PACKAGE_MODE)
    INSTALL(FILES "${PROJECT_SOURCE_DIR}/bin/GameIndex.dbf" DESTINATION "${GAMEINDEX_DIR}")
    INSTALL(FILES "${PROJECT_SOURCE_DIR}/linux_various/pcsx2.desktop" DESTINATION "${CMAKE_INSTALL_PREFIX}/share/applications")
    INSTALL(FILES "${PROJECT_SOURCE_DIR}/linux_various/pcsx2.xpm" DESTINATION "${CMAKE_INSTALL_PREFIX}/share/pixmaps")
    INSTALL(FILES "${PROJECT_SOURCE_DIR}/bin/docs/PCSX2_FAQ_0.9.8.pdf" DESTINATION "${CMAKE_INSTALL_PREFIX}/share/doc/pcsx2")
    INSTALL(FILES "${PROJECT_SOURCE_DIR}/bin/docs/PCSX2_Readme_0.9.8.pdf" DESTINATION "${CMAKE_INSTALL_PREFIX}/share/doc/pcsx2")
endif(PACKAGE_MODE)
