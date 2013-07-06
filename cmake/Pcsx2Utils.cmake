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
    # you must have both svn client executable and a source that contains svn metadata
    if(Subversion_FOUND AND EXISTS "${CMAKE_SOURCE_DIR}/.svn")
        # execute subversion
        Subversion_WC_INFO(${CMAKE_SOURCE_DIR} tmpvar)

        # write svnrev.h
        file(WRITE ${CMAKE_BINARY_DIR}/common/include/svnrev.h "#define SVN_REV ${tmpvar_WC_REVISION} \n#define SVN_MODS 0")
    else()
        # write svnrev.h
        file(WRITE ${CMAKE_BINARY_DIR}/common/include/svnrev.h "#define SVN_REV 0 \n#define SVN_MODS 0")
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
