#-------------------------------------------------------------------------------
#						detectOperatingSystem
#-------------------------------------------------------------------------------
# This function detects on which OS cmake is run and set a flag to control the
# build process. Supported OS: Linux, MacOSX, Windows
# 
# On linux, it also set a flag for specific distribution (ie Fedora)
#-------------------------------------------------------------------------------
function(detectOperatingSystem)
    # nothing detected yet
    set(MacOSX FALSE PARENT_SCOPE)
    set(Windows FALSE PARENT_SCOPE)
    set(Linux FALSE PARENT_SCOPE)
    set(Fedora FALSE PARENT_SCOPE)

    # check if we are on Linux
    if(${CMAKE_SYSTEM_NAME} STREQUAL "Linux")
        set(Linux TRUE PARENT_SCOPE)

        if (EXISTS /etc/os-release)
            # Read the file without CR character
            file(STRINGS /etc/os-release OS_RELEASE)
            if ("${OS_RELEASE}" MATCHES "^.*ID=fedora.*$")
                set(Fedora TRUE PARENT_SCOPE)
            endif()
        endif()
    endif(${CMAKE_SYSTEM_NAME} STREQUAL "Linux")

    # check if we are on MacOSX
    if(APPLE)
        set(MacOSX TRUE PARENT_SCOPE)
    endif(APPLE)

    # check if we are on Windows
    if(${CMAKE_SYSTEM_NAME} STREQUAL "Windows")
        set(Windows TRUE PARENT_SCOPE)
    endif(${CMAKE_SYSTEM_NAME} STREQUAL "Windows")
endfunction(detectOperatingSystem)

function(write_svnrev_h)
    if (GIT_FOUND)
        execute_process(COMMAND git -C ${CMAKE_SOURCE_DIR} show  -s --format=%ci HEAD
            OUTPUT_VARIABLE tmpvar_WC_INFO
            OUTPUT_STRIP_TRAILING_WHITESPACE)
        # %2014-03-25 16:36:29 +0100
        string(REGEX REPLACE "[%:\\-]" "" tmpvar_WC_INFO "${tmpvar_WC_INFO}")
        string(REGEX REPLACE "([0-9]+) ([0-9]+).*" "\\1\\2" tmpvar_WC_INFO "${tmpvar_WC_INFO}")

        file(WRITE ${CMAKE_BINARY_DIR}/common/include/svnrev.h "#define SVN_REV ${tmpvar_WC_INFO}ll \n#define SVN_MODS 0")
    else()
        file(WRITE ${CMAKE_BINARY_DIR}/common/include/svnrev.h "#define SVN_REV_UNKNOWN\n#define SVN_REV 0ll \n#define SVN_MODS 0")
    endif()
endfunction()

function(check_compiler_version version_warn version_err)
    if("${CMAKE_CXX_COMPILER_ID}" MATCHES "GNU")
        execute_process(COMMAND ${CMAKE_C_COMPILER} -dumpversion OUTPUT_VARIABLE GCC_VERSION)
        string(STRIP "${GCC_VERSION}" GCC_VERSION)
        if(GCC_VERSION VERSION_LESS ${version_err})
            message(FATAL_ERROR "PCSX2 doesn't support your old GCC ${GCC_VERSION}! Please upgrade it ! 
            
            The minimum version is ${version_err} but ${version_warn} is warmly recommended")
        else()
            if(GCC_VERSION VERSION_LESS ${version_warn})
                message(WARNING "PCSX2 will stop to support GCC ${GCC_VERSION} in a near future. Please upgrade it to GCC ${version_warn}.")
            endif()
        endif()
    endif()
endfunction()

function(check_no_parenthesis_in_path)
    if ("${CMAKE_BINARY_DIR}" MATCHES "[()]" OR "${CMAKE_SOURCE_DIR}" MATCHES "[()]")
        message(FATAL_ERROR "Your path contains some parenthesis. Unfortunately Cmake doesn't support them correctly.\nPlease rename your directory to avoid '(' and ')' characters\n")
    endif()
endfunction()
