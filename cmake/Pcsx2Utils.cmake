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
