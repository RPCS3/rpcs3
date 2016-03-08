# - try to find DirectX include directories and libraries
#
# Once done this will define:
#
#  DirectX_XYZ_FOUND         - system has the XYZ API
#  DirectX_XYZ_INCLUDE_FOUND - system has the include for the XYZ API
#  DirectX_XYZ_INCLUDE_DIR   - include directory for the XYZ API
#  DirectX_XYZ_LIBRARY       - path/name for the XYZ library
#
# Where XYZ can be any of:
#
#  DDRAW
#  D3D
#  D3DX
#  D3D8
#  D3DX8
#  D3D9
#  D3DX9
#  D3D10
#  D3D10_1
#  D3DX10
#  D3D11
#  D3D11_1
#  D3D11_2
#  D3DX11
#  D3D12
#  D3DX12
#  D2D1
#  XAUDIO


include (CheckIncludeFileCXX)
include (FindPackageMessage)


if (WIN32)
    if (CMAKE_SIZEOF_VOID_P EQUAL 8)
        set (DirectX_ARCHITECTURE x64)
    else ()
        set (DirectX_ARCHITECTURE x86)
    endif ()

    # Can't use "$ENV{ProgramFiles(x86)}" to avoid violating CMP0053.  See
    # http://public.kitware.com/pipermail/cmake-developers/2014-October/023190.html
    set (ProgramFiles_x86 "ProgramFiles(x86)")
    if ("$ENV{${ProgramFiles_x86}}")
        set (ProgramFiles "$ENV{${ProgramFiles_x86}}")
    else ()
        set (ProgramFiles "$ENV{ProgramFiles}")
    endif ()

    find_path (DirectX_ROOT_DIR
        Include/d3d9.h
        PATHS
            "$ENV{DXSDK_DIR}"
            "${ProgramFiles}/Microsoft DirectX SDK (June 2010)"
            "${ProgramFiles}/Microsoft DirectX SDK (February 2010)"
            "${ProgramFiles}/Microsoft DirectX SDK (March 2009)"
            "${ProgramFiles}/Microsoft DirectX SDK (August 2008)"
            "${ProgramFiles}/Microsoft DirectX SDK (June 2008)"
            "${ProgramFiles}/Microsoft DirectX SDK (March 2008)"
            "${ProgramFiles}/Microsoft DirectX SDK (November 2007)"
            "${ProgramFiles}/Microsoft DirectX SDK (August 2007)"
            "${ProgramFiles}/Microsoft DirectX SDK"
        DOC "DirectX SDK root directory"
    )
    if (DirectX_ROOT_DIR)
        set (DirectX_INC_SEARCH_PATH "${DirectX_ROOT_DIR}/Include")
        set (DirectX_LIB_SEARCH_PATH "${DirectX_ROOT_DIR}/Lib/${DirectX_ARCHITECTURE}")
        set (DirectX_BIN_SEARCH_PATH "${DirectX_ROOT_DIR}/Utilities/bin/x86")
    endif ()

    # With VS 2011 and Windows 8 SDK, the DirectX SDK is included as part of
    # the Windows SDK.
    #
    # See also:
    # - http://msdn.microsoft.com/en-us/library/windows/desktop/ee663275.aspx
    if (DEFINED MSVC_VERSION AND NOT ${MSVC_VERSION} LESS 1700)
        set (USE_WINSDK_HEADERS TRUE)
    endif ()

    # Find a header in the DirectX SDK
    macro (find_dxsdk_header var_name header)
        set (include_dir_var "DirectX_${var_name}_INCLUDE_DIR")
        set (include_found_var "DirectX_${var_name}_INCLUDE_FOUND")
        find_path (${include_dir_var} ${header}
            HINTS ${DirectX_INC_SEARCH_PATH}
            DOC "The directory where ${header} resides"
            CMAKE_FIND_ROOT_PATH_BOTH
        )
        if (${include_dir_var})
            set (${include_found_var} TRUE)
            find_package_message (${var_name}_INC "Found ${header} header: ${${include_dir_var}}/${header}" "[${${include_dir_var}}]")
        endif ()
        mark_as_advanced (${include_found_var})
    endmacro ()

    # Find a library in the DirectX SDK
    macro (find_dxsdk_library var_name library)
        # DirectX SDK
        set (library_var "DirectX_${var_name}_LIBRARY")
        find_library (${library_var} ${library}
            HINTS ${DirectX_LIB_SEARCH_PATH}
            DOC "The directory where ${library} resides"
            CMAKE_FIND_ROOT_PATH_BOTH
        )
        if (${library_var})
            find_package_message (${var_name}_LIB "Found ${library} library: ${${library_var}}" "[${${library_var}}]")
        endif ()
        mark_as_advanced (${library_var})
    endmacro ()

    # Find a header in the Windows SDK
    macro (find_winsdk_header var_name header)
        if (USE_WINSDK_HEADERS)
            # Windows SDK
            set (include_dir_var "DirectX_${var_name}_INCLUDE_DIR")
            set (include_found_var "DirectX_${var_name}_INCLUDE_FOUND")
            check_include_file_cxx (${header} ${include_found_var})
            set (${include_dir_var})
            mark_as_advanced (${include_found_var})
        else ()
            find_dxsdk_header (${var_name} ${header})
        endif ()
    endmacro ()

    # Find a library in the Windows SDK
    macro (find_winsdk_library var_name library)
        if (USE_WINSDK_HEADERS)
            # XXX: We currently just assume the library exists
            set (library_var "DirectX_${var_name}_LIBRARY")
            set (${library_var} ${library})
            mark_as_advanced (${library_var})
        else ()
            find_dxsdk_library (${var_name} ${library})
        endif ()
    endmacro ()

    # Combine header and library variables into an API found variable
    macro (find_combined var_name inc_var_name lib_var_name)
        if (DirectX_${inc_var_name}_INCLUDE_FOUND AND DirectX_${lib_var_name}_LIBRARY)
            set (DirectX_${var_name}_FOUND 1)
            find_package_message (${var_name} "Found ${var_name} API" "[${DirectX_${lib_var_name}_LIBRARY}][${DirectX_${inc_var_name}_INCLUDE_DIR}]")
        endif ()
    endmacro ()

    find_winsdk_header  (DDRAW   ddraw.h)
    find_winsdk_library (DDRAW   ddraw)
    find_combined       (DDRAW   DDRAW DDRAW)

    if (CMAKE_GENERATOR_TOOLSET MATCHES "_xp$")
        set (WINDOWS_XP TRUE)
    endif ()

    if (WINDOWS_XP)
        # Windows 7 SDKs, used by XP toolset, do not include d3d.h
        find_dxsdk_header   (D3D     d3d.h)
    else ()
        find_winsdk_header  (D3D     d3d.h)
    endif ()
    find_combined       (D3D     D3D DDRAW)

    find_dxsdk_header   (D3DX    d3dx.h)
    find_combined       (D3DX    D3DX D3DX)

    find_dxsdk_header   (D3D8    d3d8.h)
    find_dxsdk_library  (D3D8    d3d8)
    find_combined       (D3D8    D3D8 D3D8)

    find_dxsdk_header   (D3DX8   d3dx8.h)
    find_dxsdk_library  (D3DX8   d3dx8)
    find_combined       (D3DX8   D3DX8 D3DX8)

    find_winsdk_header  (D3D9    d3d9.h)
    find_winsdk_library (D3D9    d3d9)
    find_combined       (D3D9    D3D9 D3D9)

    find_dxsdk_header   (D3DX9   d3dx9.h)
    find_dxsdk_library  (D3DX9   d3dx9)
    find_combined       (D3DX9   D3DX9 D3DX9)

    find_winsdk_header  (XAUDIO   xaudio2.h)
    find_winsdk_library (XAUDIO   xaudio2)
    find_combined       (XAUDIO   XAUDIO XAUDIO)

    if (NOT WINDOWS_XP)
        find_winsdk_header  (DXGI    dxgi.h)
        find_winsdk_header  (DXGI1_2 dxgi1_2.h)
        find_winsdk_header  (DXGI1_3 dxgi1_3.h)
        find_winsdk_header  (DXGI1_4 dxgi1_4.h)
        find_winsdk_library (DXGI    dxgi)

        find_winsdk_header  (D3D10   d3d10.h)
        find_winsdk_library (D3D10   d3d10)
        find_combined       (D3D10   D3D10 D3D10)

        find_winsdk_header  (D3D10_1 d3d10_1.h)
        find_winsdk_library (D3D10_1 d3d10_1)
        find_combined       (D3D10_1 D3D10_1 D3D10_1)

        find_dxsdk_header   (D3DX10  d3dx10.h)
        find_dxsdk_library  (D3DX10  d3dx10)
        find_combined       (D3DX10  D3DX10 D3DX10)

        find_winsdk_header  (D3D11   d3d11.h)
        find_winsdk_library (D3D11   d3d11)
        find_combined       (D3D11   D3D11 D3D11)
        find_winsdk_header  (D3D11_1 d3d11_1.h)
        find_combined       (D3D11_1 D3D11_1 D3D11)
        find_winsdk_header  (D3D11_2 d3d11_2.h)
        find_combined       (D3D11_2 D3D11_2 D3D11)
        find_winsdk_header  (D3D11_3 d3d11_3.h)
        find_combined       (D3D11_3 D3D11_3 D3D11)

        find_dxsdk_header   (D3DX11  d3dx11.h)
        find_dxsdk_library  (D3DX11  d3dx11)
        find_combined       (D3DX11  D3DX11 D3DX11)

        find_winsdk_header  (D3D12   d3d12.h)
        find_winsdk_library (D3D12   d3d12)
        find_combined       (D3D12   D3D12 D3D12)

        find_winsdk_header  (D2D1    d2d1.h)
        find_winsdk_library (D2D1    d2d1)
        find_combined       (D2D1    D2D1 D2D1)
        find_winsdk_header  (D2D1_1  d2d1_1.h)
        find_combined       (D2D1_1  D2D1_1 D2D1)
    endif ()

    find_program (DirectX_FXC_EXECUTABLE fxc
        HINTS ${DirectX_BIN_SEARCH_PATH}
        DOC "Path to fxc.exe executable."
    )

endif ()