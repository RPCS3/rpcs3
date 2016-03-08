# - try to find part of DirectX SDK
#
# Cache Variables: (probably not for direct use in your scripts)
#  DIRECTX_INCLUDE_DIR
#
# Variables you should use in your CMakeLists.txt:
#  DIRECTX_LIBRARIES
#  DIRECTX_DXGUID_LIBRARY - deprecated, see below
#  DIRECTX_DXERR_LIBRARY - deprecated, see http://blogs.msdn.com/b/chuckw/archive/2012/04/24/where-s-dxerr-lib.aspx
#  DIRECTX_DINPUT_LIBRARY
#  DIRECTX_DINPUT_INCLUDE_DIR
#  DIRECTX_D3D9_LIBRARY
#  DIRECTX_D3DXOF_LIBRARY
#  DIRECTX_DXGI_LIBRARY
#  DIRECTX_D3DX9_LIBRARIES
#  DIRECTX_XAUDIO_LIBRARY
#  DIRECTX_D2D1_LIBRARY
#  DIRECTX_XINPUT_LIBRARY
#  DIRECTX_DWIRTE_LIBRARY
#  DIRECTX_INCLUDE_DIRS
#  DIRECTX_D3D12_INCLUDE_DIR
#  DIRECTX_FOUND - if this is not true, do not attempt to use this library
#
# Defines these macros:
#  find_directx_include - wrapper for find_path that provides PATHS, HINTS, and PATH_SUFFIXES.
#  find_directx_library - wrapper for find_library that provides PATHS, HINTS, and PATH_SUFFIXES.
# Requires these CMake modules:
#  FindPackageHandleStandardArgs (known included with CMake >=2.6.2)
#  SelectLibraryConfigurations
#
# Original Author:
# 2012 Ryan Pavlik <rpavlik@iastate.edu> <abiryan@ryand.net>
# http://academic.cleardefinition.com
# Iowa State University HCI Graduate Program/VRAC
#
# Copyright Iowa State University 2012.
# Distributed under the Boost Software License, Version 1.0.
# (See accompanying file LICENSE_1_0.txt or copy at
# http://www.boost.org/LICENSE_1_0.txt)


set(DIRECTX_ROOT_DIR
    "${DIRECTX_ROOT_DIR}"
    CACHE
    PATH
    "Root directory to search for DirectX")

if(MSVC)
    file(TO_CMAKE_PATH "$ENV{ProgramFiles}" _PROG_FILES)
    set(_PF86 "ProgramFiles(x86)")
    file(TO_CMAKE_PATH "$ENV{${_PF86}}" _PROG_FILES_X86)
    if(_PROG_FILES_X86)
        set(_PROG_FILES "${_PROG_FILES_X86}")
    endif()
    if(CMAKE_SIZEOF_VOID_P EQUAL 8)
        set(_dx_lib_suffixes lib/x64 lib)
    else()
        set(_dx_lib_suffixes lib/x86 lib)
    endif()
    set(DXSDK_DIRS)

    set(_dx_quiet)
    if(DirectX_FIND_QUIETLY)
        set(_dx_quiet QUIET)
    endif()
    find_package(WindowsSDK ${_dx_quiet})
    if(WINDOWSSDK_FOUND)
        foreach(_dir ${WINDOWSSDK_DIRS})
            get_windowssdk_include_dirs(${_dir} _include_dirs)
            if(_include_dirs)
                list(APPEND DXSDK_DIRS ${_include_dirs})
            endif()
        endforeach()
    endif()

    macro(_append_dxsdk_in_inclusive_range _low _high)
        if((NOT MSVC_VERSION LESS ${_low}) AND (NOT MSVC_VERSION GREATER ${_high}))
            list(APPEND DXSDK_DIRS ${ARGN})
        endif()
    endmacro()
    _append_dxsdk_in_inclusive_range(1500 1600 "${_PROG_FILES}/Microsoft DirectX SDK (June 2010)")
    _append_dxsdk_in_inclusive_range(1400 1600
        "${_PROG_FILES}/Microsoft DirectX SDK (February 2010)"
        "${_PROG_FILES}/Microsoft DirectX SDK (August 2009)"
        "${_PROG_FILES}/Microsoft DirectX SDK (March 2009)"
        "${_PROG_FILES}/Microsoft DirectX SDK (November 2008)"
        "${_PROG_FILES}/Microsoft DirectX SDK (August 2008)"
        "${_PROG_FILES}/Microsoft DirectX SDK (June 2008)"
        "${_PROG_FILES}/Microsoft DirectX SDK (March 2008)")
    _append_dxsdk_in_inclusive_range(1310 1500
        "${_PROG_FILES}/Microsoft DirectX SDK (November 2007)"
        "${_PROG_FILES}/Microsoft DirectX SDK (August 2007)"
        "${_PROG_FILES}/Microsoft DirectX SDK (June 2007)"
        "${_PROG_FILES}/Microsoft DirectX SDK (April 2007)"
        "${_PROG_FILES}/Microsoft DirectX SDK (February 2007)"
        "${_PROG_FILES}/Microsoft DirectX SDK (December 2006)"
        "${_PROG_FILES}/Microsoft DirectX SDK (October 2006)"
        "${_PROG_FILES}/Microsoft DirectX SDK (August 2006)"
        "${_PROG_FILES}/Microsoft DirectX SDK (June 2006)"
        "${_PROG_FILES}/Microsoft DirectX SDK (April 2006)"
        "${_PROG_FILES}/Microsoft DirectX SDK (February 2006)")

    file(TO_CMAKE_PATH "$ENV{DXSDK_DIR}" ENV_DXSDK_DIR)
    if(ENV_DXSDK_DIR)
        list(APPEND DXSDK_DIRS ${ENV_DXSDK_DIR})
    endif()
else()
    set(_dx_lib_suffixes lib)
    set(DXSDK_DIRS /mingw)
endif()


find_path(DIRECTX_INCLUDE_DIR
    NAMES
    dxdiag.h
    dinput.h
    dxerr8.h
    xaudio2.h
    PATHS
    ${DXSDK_DIRS}
    HINTS
    "${DIRECTX_ROOT_DIR}"
    PATH_SUFFIXES
    include)

find_path(DIRECTX_D3D12_INCLUDE_DIR
    NAMES
    d3d12.h
    PATHS
    ${DXSDK_DIRS}
    HINTS
    "${DIRECTX_ROOT_DIR}"
    PATH_SUFFIXES
    include)

set(DXLIB_HINTS)
if(WINDOWSSDK_FOUND AND DIRECTX_INCLUDE_DIR)
    get_windowssdk_from_component("${DIRECTX_INCLUDE_DIR}" _winsdk)
    if(_winsdk)
        get_windowssdk_library_dirs("${_winsdk}" _libdirs)
        if(_libdirs)
            list(APPEND DXLIB_HINTS ${_libdirs})
        endif()
    endif()
endif()

if(WINDOWSSDK_FOUND AND DIRECTX_DINPUT_INCLUDE_DIR)
    get_windowssdk_from_component("${DIRECTX_DINPUT_INCLUDE_DIR}" _winsdk)
    if(_winsdk)
        get_windowssdk_library_dirs("${_winsdk}" _includes)
        if(_includes)
            list(APPEND DXLIB_HINTS ${_includes})
        endif()
    endif()
endif()

find_library(DIRECTX_DXGUID_LIBRARY
    NAMES
    dxguid
    PATHS
    ${DXLIB_HINTS}
    ${DXSDK_DIRS}
    HINTS
    "${DIRECTX_ROOT_DIR}"
    PATH_SUFFIXES
    ${_dx_lib_suffixes})

if(DIRECTX_DXGUID_LIBRARY)
    get_filename_component(_dxsdk_lib_dir ${DIRECTX_DXGUID_LIBRARY} PATH)
    list(APPEND DXLIB_HINTS "${_dxsdk_lib_dir}")
endif()

find_library(DIRECTX_DINPUT_LIBRARY
    NAMES
    dinput8
    dinput
    PATHS
    ${DXSDK_DIRS}
    HINTS
    ${DXLIB_HINTS}
    "${DIRECTX_ROOT_DIR}"
    PATH_SUFFIXES
    ${_dx_lib_suffixes})

find_library(DIRECTX_D2D1_LIBRARY
    NAMES
    d2d1
    PATHS
    ${DXSDK_DIRS}
    HINTS
    ${DXLIB_HINTS}
    "${DIRECTX_ROOT_DIR}"
    PATH_SUFFIXES
    ${_dx_lib_suffixes})


find_library(DIRECTX_DXGI_LIBRARY
    NAMES
    dxgi
    PATHS
    ${DXSDK_DIRS}
    HINTS
    ${DXLIB_HINTS}
    "${DIRECTX_ROOT_DIR}"
    PATH_SUFFIXES
    ${_dx_lib_suffixes})

find_library(DIRECTX_XAUDIO_LIBRARY
    NAMES
    xaudio2
    xaudio2_8
    PATHS
    ${DXSDK_DIRS}
    HINTS
    ${DXLIB_HINTS}
    "${DIRECTX_ROOT_DIR}"
    PATH_SUFFIXES
    ${_dx_lib_suffixes})

find_library(DIRECTX_DWRITE_LIBRARY
    NAMES
    dwrite.lib
    PATHS
    ${DXSDK_DIRS}
    HINTS
    ${DXLIB_HINTS}
    "${DIRECTX_ROOT_DIR}"
    PATH_SUFFIXES
    ${_dx_lib_suffixes})

find_library(DIRECTX_DXERR_LIBRARY
    NAMES
    dxerr
    dxerr9
    dxerr8
    PATHS
    ${DXSDK_DIRS}
    HINTS
    ${DXLIB_HINTS}
    "${DIRECTX_ROOT_DIR}"
    PATH_SUFFIXES
    ${_dx_lib_suffixes})

find_library(DIRECTX_D3D9_LIBRARY
    NAMES
    d3d9
    PATHS
    ${DXSDK_DIRS}
    HINTS
    ${DXLIB_HINTS}
    "${DIRECTX_ROOT_DIR}"
    PATH_SUFFIXES
    ${_dx_lib_suffixes})

find_library(DIRECTX_D3DXOF_LIBRARY
    NAMES
    d3dxof
    PATHS
    ${DXSDK_DIRS}
    HINTS
    ${DXLIB_HINTS}
    "${DIRECTX_ROOT_DIR}"
    PATH_SUFFIXES
    ${_dx_lib_suffixes})

find_library(DIRECTX_D3DX9_LIBRARY_RELEASE
    NAMES
    d3dx9
    PATHS
    ${DXSDK_DIRS}
    HINTS
    ${DXLIB_HINTS}
    "${DIRECTX_ROOT_DIR}"
    PATH_SUFFIXES
    ${_dx_lib_suffixes})

find_library(DIRECTX_D3DX9_LIBRARY_DEBUG
    NAMES
    d3dx9d
    PATHS
    ${DXSDK_DIRS}
    HINTS
    ${DXLIB_HINTS}
    "${DIRECTX_ROOT_DIR}"
    PATH_SUFFIXES
    ${_dx_lib_suffixes})

find_library(DIRECTX_XINPUT_LIBRARY
    NAMES
    Xinput9_1_0
    Xinput
    PATHS
    ${DXSDK_DIRS}
    HINTS
    ${DXLIB_HINTS}
    "${DIRECTX_ROOT_DIR}"
    PATH_SUFFIXES
    ${_dx_lib_suffixes})

include(SelectLibraryConfigurations)
select_library_configurations(DIRECTX_D3DX9)

set(DIRECTX_EXTRA_CHECK)
if(DIRECTX_INCLUDE_DIR)
    if(MSVC80)
        set(DXSDK_DEPRECATION_BUILD 1962)
    endif()

    if(DXSDK_DEPRECATION_BUILD)
        include(CheckCSourceCompiles)
        set(_dinput_old_includes ${CMAKE_REQUIRED_INCLUDES})
        set(CMAKE_REQUIRED_INCLUDES "${DIRECTX_INCLUDE_DIR}")
        check_c_source_compiles("
            #include <dxsdkver.h>
            #if _DXSDK_BUILD_MAJOR >= ${DXSDK_DEPRECATION_BUILD}
            #error
            #else
            int main(int argc, char * argv[]) {
                return 0;
            }
            #endif
            "
            DIRECTX_SDK_SUPPORTS_COMPILER)
        set(DIRECTX_EXTRA_CHECK DIRECTX_SDK_SUPPORTS_COMPILER)
        set(CMAKE_REQUIRED_INCLUDES "${_dinput_old_includes}")
    else()
        # Until proven otherwise.
        set(DIRECTX_SDK_SUPPORTS_COMPILER TRUE)
    endif()
endif()

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(DirectX
    DEFAULT_MSG
    DIRECTX_DXGUID_LIBRARY
    DIRECTX_DINPUT_LIBRARY
    DIRECTX_INCLUDE_DIR
    ${DIRECTX_EXTRA_CHECK})

if(DIRECTX_FOUND)
    set(DIRECTX_LIBRARIES
        "${DIRECTX_DXGI_LIBRARY}"
        "${DIRECTX_D2D1_LIBRARY}"
        "${DIRECTX_DWRITE_LIBRARY}"
        "${DIRECTX_XAUDIO_LIBRARY}"
        "${DIRECTX_XINPUT_LIBRARY}")

    set(DIRECTX_INCLUDE_DIRS "${DIRECTX_INCLUDE_DIR}")

    mark_as_advanced(DIRECTX_ROOT_DIR)
endif()

macro(find_directx_library)
    find_library(${ARGN}
        PATHS
        ${DXSDK_DIRS}
        HINTS
        ${DXLIB_HINTS}
        "${DIRECTX_ROOT_DIR}"
        PATH_SUFFIXES
        ${_dx_lib_suffixes})
endmacro()
macro(find_directx_include)
    find_path(${ARGN}
        PATHS
        ${DXSDK_DIRS}
        HINTS
        ${DIRECTX_INCLUDE_DIR}
        "${DIRECTX_ROOT_DIR}"
        PATH_SUFFIXES
        include)
endmacro()

mark_as_advanced(DIRECTX_DINPUT_LIBRARY
    DIRECTX_DXGUID_LIBRARY
    DIRECTX_DXERR_LIBRARY
    DIRECTX_XINPUT_LIBRARY
    DIRECTX_XAUDIO_LIBRARY
    DIRECTX_DWRITE_LIBRARY
    DIRECTX_DXGI_LIBRARY
    DIRECTX_D2D1_LIBRARY
    DIRECTX_D3D9_LIBRARY
    DIRECTX_D3DXOF_LIBRARY
    DIRECTX_D3DX9_LIBRARY_RELEASE
    DIRECTX_D3DX9_LIBRARY_DEBUG
    DIRECTX_INCLUDE_DIR)