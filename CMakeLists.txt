# Project Name
project(Pcsx2)

# Debian-based distributions require at least 2.8.5 due to multiarch.
cmake_minimum_required(VERSION 2.8.5)

# Keep OLD policy. Code was updated to support the new one but wasn't tested
#
# cmake version 2.8.9
#   CMP0018
#        Ignore CMAKE_SHARED_LIBRARY_<Lang>_FLAGS variable.
# 
#        CMake 2.8.8 and lower compiled sources in SHARED and MODULE libraries
#        using the value of the undocumented CMAKE_SHARED_LIBRARY_<Lang>_FLAGS
#        platform variable.  The variable contained platform-specific flags
#        needed to compile objects for shared libraries.  Typically it included
#        a flag such as -fPIC for position independent code but also included
#        other flags needed on certain platforms.  CMake 2.8.9 and higher
#        prefer instead to use the POSITION_INDEPENDENT_CODE target property to
#        determine what targets should be position independent, and new
#        undocumented platform variables to select flags while ignoring
#        CMAKE_SHARED_LIBRARY_<Lang>_FLAGS completely.
# 
#        The default for either approach produces identical compilation flags,
#        but if a project modifies CMAKE_SHARED_LIBRARY_<Lang>_FLAGS from its
#        original value this policy determines which approach to use.
# 
#        The OLD behavior for this policy is to ignore the
#        POSITION_INDEPENDENT_CODE property for all targets and use the
#        modified value of CMAKE_SHARED_LIBRARY_<Lang>_FLAGS for SHARED and
#        MODULE libraries.
# 
#        The NEW behavior for this policy is to ignore
#        CMAKE_SHARED_LIBRARY_<Lang>_FLAGS whether it is modified or not and
#        honor the POSITION_INDEPENDENT_CODE target property.
if(POLICY CMP0018)
    cmake_policy(SET CMP0018 OLD)
endif()

# Variable to check that people use the good file
set(TOP_CMAKE_WAS_SOURCED TRUE)

# Print a clear message that 64bits is not supported
# It would avoid compilation failure later.
# Note: disable the failure in package mode
if(NOT PACKAGE_MODE)
    if(CMAKE_SIZEOF_VOID_P MATCHES "8")
        message(WARNING "
        PCSX2 does not support a 64-bits environment and has no plan to support a 64-bits architecture in the future.
        It would need a complete rewrite of the core emulator and a lot of time.

        You can still run a 32-bits binary if you install all 32-bits libraries (runtime and dev).

        Ubuntu users, please upgrade to 12.04.")
    endif(CMAKE_SIZEOF_VOID_P MATCHES "8")
endif(NOT PACKAGE_MODE)

# 64 bits specific configuration
if(CMAKE_SIZEOF_VOID_P MATCHES "8")
    # Do not search library in /usr/lib64
    SET_PROPERTY(GLOBAL PROPERTY FIND_LIBRARY_USE_LIB64_PATHS OFF)
    # Probably useless but it will not harm
    SET_PROPERTY(GLOBAL PROPERTY COMPILE_DEFINITIONS "-m32")

    # Force the search on 32-bits path.
    if(EXISTS "/usr/lib32")
        set(CMAKE_LIBRARY_ARCHITECTURE "../lib32")
    endif()
endif(CMAKE_SIZEOF_VOID_P MATCHES "8")

# Debian/ubuntu drop /usr/lib32 and move /usr/lib to /usr/lib/i386-linux-gnu
if(EXISTS "/usr/lib/i386-linux-gnu")
    set(CMAKE_LIBRARY_ARCHITECTURE "i386-linux-gnu")
endif()


# * -fPIC option was removed for multiple reasons.
#     - Code only supports the x86 architecture.
#     - code uses the ebx register so it's not compliant with PIC.
#     - Impacts the performance too much.
#     - Only plugins. No package will link to them.
set(CMAKE_POSITION_INDEPENDENT_CODE OFF) 

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
					${PROJECT_SOURCE_DIR}/common/include/x86emitter
                    # WORKAROUND Some issue with multiarch on Debian/Ubuntu
                    /usr/include/i386-linux-gnu
                    )

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
    INSTALL(FILES "${PROJECT_SOURCE_DIR}/bin/docs/PCSX2_FAQ.pdf" DESTINATION "${CMAKE_INSTALL_PREFIX}/share/doc/pcsx2")
    INSTALL(FILES "${PROJECT_SOURCE_DIR}/bin/docs/PCSX2_Readme.pdf" DESTINATION "${CMAKE_INSTALL_PREFIX}/share/doc/pcsx2")
    INSTALL(FILES "${PROJECT_SOURCE_DIR}/bin/docs/pcsx2.1" DESTINATION "${CMAKE_INSTALL_PREFIX}/share/man/man1/")
endif(PACKAGE_MODE)
