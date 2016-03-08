# - Find LLVM headers and libraries.
# This module locates LLVM and adapts the llvm-config output for use with
# CMake.
#
# A given list of COMPONENTS is passed to llvm-config.
#
# The following variables are defined:
#  LLVM_FOUND          - true if LLVM was found
#  LLVM_CXXFLAGS       - C++ compiler flags for files that include LLVM headers.
#  LLVM_HOST_TARGET    - Target triple used to configure LLVM.
#  LLVM_INCLUDE_DIRS   - Directory containing LLVM include files.
#  LLVM_LDFLAGS        - Linker flags to add when linking against LLVM
#                        (includes -LLLVM_LIBRARY_DIRS).
#  LLVM_LIBRARIES      - Full paths to the library files to link against.
#  LLVM_LIBRARY_DIRS   - Directory containing LLVM libraries.
#  LLVM_ROOT_DIR       - The root directory of the LLVM installation.
#                        llvm-config is searched for in ${LLVM_ROOT_DIR}/bin.
#  LLVM_VERSION_MAJOR  - Major version of LLVM.
#  LLVM_VERSION_MINOR  - Minor version of LLVM.
#  LLVM_VERSION_STRING - Full LLVM version string (e.g. 2.9).
#
# Note: The variable names were chosen in conformance with the offical CMake
# guidelines, see ${CMAKE_ROOT}/Modules/readme.txt.

# Try suffixed versions to pick up the newest LLVM install available on Debian
# derivatives.
# We also want an user-specified LLVM_ROOT_DIR to take precedence over the
# system default locations such as /usr/local/bin. Executing find_program()
# multiples times is the approach recommended in the docs.
set(llvm_config_names llvm-config-3.9 llvm-config39
                      llvm-config-3.8 llvm-config38
                      llvm-config-3.7 llvm-config37
                      llvm-config-3.6 llvm-config36
                      llvm-config-3.5 llvm-config35
                      llvm-config-3.4 llvm-config34
                      llvm-config-3.3 llvm-config33
                      llvm-config-3.2 llvm-config32
                      llvm-config-3.1 llvm-config31 llvm-config)
find_program(LLVM_CONFIG
    NAMES ${llvm_config_names}
    PATHS ${LLVM_ROOT_DIR}/bin NO_DEFAULT_PATH
    DOC "Path to llvm-config tool.")
find_program(LLVM_CONFIG NAMES ${llvm_config_names})

if ((WIN32 AND NOT(MINGW OR CYGWIN)) OR NOT LLVM_CONFIG)
    if (WIN32)
        # A bit of a sanity check:
        if( NOT EXISTS ${LLVM_ROOT_DIR}/include/llvm )
            message(FATAL_ERROR "LLVM_ROOT_DIR (${LLVM_ROOT_DIR}) is not a valid LLVM install")
        endif()
        # We incorporate the CMake features provided by LLVM:
        set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${LLVM_ROOT_DIR}/share/llvm/cmake;${LLVM_ROOT_DIR}/lib/cmake/llvm")
        include(LLVMConfig)
        # Set properties
        set(LLVM_HOST_TARGET ${TARGET_TRIPLE})
        set(LLVM_VERSION_STRING ${LLVM_PACKAGE_VERSION})
        set(LLVM_CXXFLAGS ${LLVM_DEFINITIONS})
        set(LLVM_LDFLAGS "")
        list(REMOVE_ITEM LLVM_FIND_COMPONENTS "all-targets" index)
        list(APPEND LLVM_FIND_COMPONENTS ${LLVM_TARGETS_TO_BUILD})
        # Work around LLVM bug 21016
        list(FIND LLVM_TARGETS_TO_BUILD "X86" TARGET_X86)
        if(TARGET_X86 GREATER -1)
            list(APPEND LLVM_FIND_COMPONENTS x86utils)
        endif()
        # Similar to the work around above, but for AArch64
        list(FIND LLVM_TARGETS_TO_BUILD "AArch64" TARGET_AArch64)
        if(TARGET_AArch64 GREATER -1)
            list(APPEND LLVM_FIND_COMPONENTS AArch64Utils)
        endif()
        if(${LLVM_VERSION_STRING} MATCHES "^3\\.[0-6][\\.0-9A-Za-z]*")
            # Versions below 3.7 do not support components debuginfo[dwarf|pdb]
            # Only debuginfo is available
            list(REMOVE_ITEM LLVM_FIND_COMPONENTS "debuginfodwarf" index)
            list(REMOVE_ITEM LLVM_FIND_COMPONENTS "debuginfopdb" index)
            list(APPEND LLVM_FIND_COMPONENTS "debuginfo")
        endif()
        if(${LLVM_VERSION_STRING} MATCHES "^3\\.[0-8][\\.0-9A-Za-z]*")
            # Versions below 3.9 do not support components debuginfocodeview
            list(REMOVE_ITEM LLVM_FIND_COMPONENTS "debuginfocodeview" index)
        endif()
        if(${LLVM_VERSION_STRING} MATCHES "^3\\.[8-9][\\.0-9A-Za-z]*")
            # Versions beginning with 3.8 do not support component ipa
            list(REMOVE_ITEM LLVM_FIND_COMPONENTS "ipa" index)
        endif()

        if(${LLVM_VERSION_STRING} MATCHES "^3\\.[0-4][\\.0-9A-Za-z]*")
            llvm_map_components_to_libraries(tmplibs ${LLVM_FIND_COMPONENTS})
        else()
            llvm_map_components_to_libnames(tmplibs ${LLVM_FIND_COMPONENTS})
        endif()
        if(MSVC)
            foreach(lib ${tmplibs})
                list(APPEND LLVM_LIBRARIES "${LLVM_LIBRARY_DIRS}/${CMAKE_STATIC_LIBRARY_PREFIX}${lib}${CMAKE_STATIC_LIBRARY_SUFFIX}")
            endforeach()
        else()
            # Rely on the library search path being set correctly via -L on
            # MinGW and others, as the library list returned by
            # llvm_map_components_to_libraries also includes imagehlp and psapi.
            set(LLVM_LDFLAGS "-L${LLVM_LIBRARY_DIRS}")
            set(LLVM_LIBRARIES ${tmplibs})
        endif()

        # When using the CMake LLVM module, LLVM_DEFINITIONS is a list
        # instead of a string. Later, the list seperators would entirely
        # disappear, replace them by spaces instead. A better fix would be
        # to switch to add_definitions() instead of throwing strings around.
        string(REPLACE ";" " " LLVM_CXXFLAGS "${LLVM_CXXFLAGS}")
    else()
        if (NOT FIND_LLVM_QUIETLY)
            message(WARNING "Could not find llvm-config. Try manually setting LLVM_CONFIG to the llvm-config executable of the installation to use.")
        endif()
    endif()
else()
    macro(llvm_set var flag)
       if(LLVM_FIND_QUIETLY)
            set(_quiet_arg ERROR_QUIET)
        endif()
        execute_process(
            COMMAND ${LLVM_CONFIG} --${flag}
            OUTPUT_VARIABLE LLVM_${var}
            OUTPUT_STRIP_TRAILING_WHITESPACE
            ${_quiet_arg}
        )
        if(${ARGV2})
            file(TO_CMAKE_PATH "${LLVM_${var}}" LLVM_${var})
        endif()
    endmacro()
    macro(llvm_set_libs var flag)
       if(LLVM_FIND_QUIETLY)
            set(_quiet_arg ERROR_QUIET)
        endif()
        execute_process(
            COMMAND ${LLVM_CONFIG} --${flag} ${LLVM_FIND_COMPONENTS}
            OUTPUT_VARIABLE tmplibs
            OUTPUT_STRIP_TRAILING_WHITESPACE
            ${_quiet_arg}
        )
        file(TO_CMAKE_PATH "${tmplibs}" tmplibs)
        string(REGEX MATCHALL "${pattern}[^ ]+" LLVM_${var} ${tmplibs})
    endmacro()

    llvm_set(VERSION_STRING version)
    llvm_set(CXXFLAGS cxxflags)
    llvm_set(HOST_TARGET host-target)
    llvm_set(INCLUDE_DIRS includedir true)
    llvm_set(ROOT_DIR prefix true)
    llvm_set(ENABLE_ASSERTIONS assertion-mode)

    if(${LLVM_VERSION_STRING} MATCHES "^3\\.[0-6][\\.0-9A-Za-z]*")
        # Versions below 3.7 do not support components debuginfo[dwarf|pdb]
        # Only debuginfo is available
        list(REMOVE_ITEM LLVM_FIND_COMPONENTS "debuginfodwarf" index)
        list(REMOVE_ITEM LLVM_FIND_COMPONENTS "debuginfopdb" index)
        list(APPEND LLVM_FIND_COMPONENTS "debuginfo")
    endif()
    if(${LLVM_VERSION_STRING} MATCHES "^3\\.[0-8][\\.0-9A-Za-z]*")
        # Versions below 3.9 do not support components debuginfocodeview
        list(REMOVE_ITEM LLVM_FIND_COMPONENTS "debuginfocodeview" index)
    endif()
    if(${LLVM_VERSION_STRING} MATCHES "^3\\.[8-9][\\.0-9A-Za-z]*")
        # Versions beginning with 3.8 do not support component ipa
        list(REMOVE_ITEM LLVM_FIND_COMPONENTS "ipa" index)
    endif()

    llvm_set(LDFLAGS ldflags)
    if(NOT ${LLVM_VERSION_STRING} MATCHES "^3\\.[0-4][\\.0-9A-Za-z]*")
        # In LLVM 3.5+, the system library dependencies (e.g. "-lz") are accessed
        # using the separate "--system-libs" flag.
        llvm_set(SYSTEM_LIBS system-libs)
        string(REPLACE "\n" " " LLVM_LDFLAGS "${LLVM_LDFLAGS} ${LLVM_SYSTEM_LIBS}")
    endif()
    llvm_set(LIBRARY_DIRS libdir true)
    llvm_set_libs(LIBRARIES libs)
    llvm_set(TARGETS_TO_BUILD targets-built)
    string(REGEX MATCHALL "${pattern}[^ ]+" LLVM_TARGETS_TO_BUILD ${LLVM_TARGETS_TO_BUILD})
endif()

# On CMake builds of LLVM, the output of llvm-config --cxxflags does not
# include -fno-rtti, leading to linker errors. Be sure to add it.
if(CMAKE_COMPILER_IS_GNUCXX OR (${CMAKE_CXX_COMPILER_ID} STREQUAL "Clang"))
    if(NOT ${LLVM_CXXFLAGS} MATCHES "-fno-rtti")
        set(LLVM_CXXFLAGS "${LLVM_CXXFLAGS} -fno-rtti")
    endif()
endif()

string(REGEX REPLACE "([0-9]+).*" "\\1" LLVM_VERSION_MAJOR "${LLVM_VERSION_STRING}" )
string(REGEX REPLACE "[0-9]+\\.([0-9]+).*[A-Za-z]*" "\\1" LLVM_VERSION_MINOR "${LLVM_VERSION_STRING}" )

# Use the default CMake facilities for handling QUIET/REQUIRED.
include(FindPackageHandleStandardArgs)

if(${CMAKE_VERSION} VERSION_LESS "2.8.4")
  # The VERSION_VAR argument is not supported on pre-2.8.4, work around this.
  set(VERSION_VAR dummy)
endif()

find_package_handle_standard_args(LLVM
    REQUIRED_VARS LLVM_ROOT_DIR LLVM_HOST_TARGET
    VERSION_VAR LLVM_VERSION_STRING)