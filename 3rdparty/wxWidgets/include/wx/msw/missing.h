/////////////////////////////////////////////////////////////////////////////
// Name:        wx/msw/missing.h
// Purpose:     Declarations for parts of the Win32 SDK that are missing in
//              the versions that come with some compilers
// Created:     2002/04/23
// RCS-ID:      $Id: missing.h 48436 2007-08-28 19:26:16Z JS $
// Copyright:   (c) 2002 Mattia Barbon
// Licence:     wxWindows licence
/////////////////////////////////////////////////////////////////////////////

#ifndef _WX_MISSING_H_
#define _WX_MISSING_H_

/*
 * The following are required for VC++ 6.
 */

// Needed by cursor.cpp
#ifndef IDC_HAND
    #define IDC_HAND MAKEINTRESOURCE(32649)
#endif

// Needed by strconv.cpp
#ifndef WC_NO_BEST_FIT_CHARS
    #define WC_NO_BEST_FIT_CHARS 0x400
#endif

#ifndef WM_CONTEXTMENU
    #define WM_CONTEXTMENU      0x007B
#endif

// Needed by toplevel.cpp
#ifndef WM_UPDATEUISTATE
    #define WM_UPDATEUISTATE    0x0128
#endif

#ifndef WM_CHANGEUISTATE
    #define WM_CHANGEUISTATE    0x0127
#endif

#ifndef WM_PRINTCLIENT
    #define WM_PRINTCLIENT 0x318
#endif

// Needed by toplevel.cpp
#ifndef UIS_SET
    #define UIS_SET         1
    #define UIS_CLEAR       2
    #define UIS_INITIALIZE  3
#endif

#ifndef UISF_HIDEFOCUS
    #define UISF_HIDEFOCUS  1
#endif

#ifndef UISF_HIDEACCEL
    #define UISF_HIDEACCEL 2
#endif

#ifndef OFN_EXPLORER
    #define OFN_EXPLORER 0x00080000
#endif

#ifndef OFN_ENABLESIZING
    #define OFN_ENABLESIZING 0x00800000
#endif

// Needed by window.cpp
#if wxUSE_MOUSEWHEEL
    #ifndef WM_MOUSEWHEEL
        #define WM_MOUSEWHEEL           0x020A
    #endif
    #ifndef WHEEL_DELTA
        #define WHEEL_DELTA             120
    #endif
    #ifndef SPI_GETWHEELSCROLLLINES
        #define SPI_GETWHEELSCROLLLINES 104
    #endif
#endif // wxUSE_MOUSEWHEEL

// Needed by window.cpp
#ifndef VK_OEM_1
    #define VK_OEM_1        0xBA
    #define VK_OEM_2        0xBF
    #define VK_OEM_3        0xC0
    #define VK_OEM_4        0xDB
    #define VK_OEM_5        0xDC
    #define VK_OEM_6        0xDD
    #define VK_OEM_7        0xDE
#endif

#ifndef VK_OEM_COMMA
    #define VK_OEM_PLUS     0xBB
    #define VK_OEM_COMMA    0xBC
    #define VK_OEM_MINUS    0xBD
    #define VK_OEM_PERIOD   0xBE
#endif

#ifndef SM_TABLETPC
    #define SM_TABLETPC 86
#endif

#ifndef INKEDIT_CLASS
#   define INKEDIT_CLASSW  L"INKEDIT"
#   ifdef UNICODE
#       define INKEDIT_CLASS   INKEDIT_CLASSW
#   else
#       define INKEDIT_CLASS   "INKEDIT"
#   endif
#endif

#ifndef EM_SETINKINSERTMODE
#   define EM_SETINKINSERTMODE (WM_USER + 0x0204)
#endif

#ifndef EM_SETUSEMOUSEFORINPUT
#define EM_SETUSEMOUSEFORINPUT (WM_USER + 0x224)
#endif

#ifndef TPM_RECURSE
#define TPM_RECURSE 1
#endif


#ifndef WS_EX_LAYOUTRTL
#define WS_EX_LAYOUTRTL 0x00400000
#endif

#ifndef WS_EX_COMPOSITED
#define WS_EX_COMPOSITED 0x02000000L
#endif

#ifndef WS_EX_LAYERED
#define WS_EX_LAYERED 0x00080000
#endif

#ifndef LWA_ALPHA
#define LWA_ALPHA 2
#endif

#if defined __VISUALC__ && __VISUALC__ <= 1200 && !defined MIIM_BITMAP
#define MIIM_STRING      0x00000040
#define MIIM_BITMAP      0x00000080
#define MIIM_FTYPE       0x00000100
#define HBMMENU_CALLBACK            ((HBITMAP) -1)
typedef struct tagMENUINFO
{
    DWORD   cbSize;
    DWORD   fMask;
    DWORD   dwStyle;
    UINT    cyMax;
    HBRUSH  hbrBack;
    DWORD   dwContextHelpID;
    DWORD   dwMenuData;
}   MENUINFO, FAR *LPMENUINFO;
struct wxMENUITEMINFO_
{
    UINT cbSize;
    UINT fMask;
    UINT fType;
    UINT fState;
    UINT wID;
    HMENU hSubMenu;
    HBITMAP hbmpChecked;
    HBITMAP hbmpUnchecked;
    DWORD dwItemData;
    LPTSTR dwTypeData;
    UINT cch;
    HBITMAP hbmpItem;
};
#else
#define wxMENUITEMINFO_ MENUITEMINFO
#endif

/*
 * The following are required for VC++ 5 when the PSDK is not available.
 */

#if defined __VISUALC__ && __VISUALC__ <= 1100

#ifndef VER_NT_WORKSTATION

typedef struct _OSVERSIONINFOEXA {
    DWORD dwOSVersionInfoSize;
    DWORD dwMajorVersion;
    DWORD dwMinorVersion;
    DWORD dwBuildNumber;
    DWORD dwPlatformId;
    CHAR szCSDVersion[128];
    WORD wServicePackMajor;
    WORD wServicePackMinor;
    WORD wSuiteMask;
    BYTE wProductType;
    BYTE wReserved;
} OSVERSIONINFOEXA, *POSVERSIONINFOEXA, *LPOSVERSIONINFOEXA;
typedef struct _OSVERSIONINFOEXW {
    DWORD dwOSVersionInfoSize;
    DWORD dwMajorVersion;
    DWORD dwMinorVersion;
    DWORD dwBuildNumber;
    DWORD dwPlatformId;
    WCHAR szCSDVersion[128];
    WORD wServicePackMajor;
    WORD wServicePackMinor;
    WORD wSuiteMask;
    BYTE wProductType;
    BYTE wReserved;
} OSVERSIONINFOEXW, *POSVERSIONINFOEXW, *LPOSVERSIONINFOEXW;

#ifdef UNICODE
typedef OSVERSIONINFOW OSVERSIONINFO,*POSVERSIONINFO,*LPOSVERSIONINFO;
typedef OSVERSIONINFOEXW OSVERSIONINFOEX,*POSVERSIONINFOEX,*LPOSVERSIONINFOEX;
#else
typedef OSVERSIONINFOA OSVERSIONINFO,*POSVERSIONINFO,*LPOSVERSIONINFO;
typedef OSVERSIONINFOEXA OSVERSIONINFOEX,*POSVERSIONINFOEX,*LPOSVERSIONINFOEX;
#endif

#endif // defined VER_NT_WORKSTATION

#ifndef CP_SYMBOL
    #define CP_SYMBOL 42
#endif

// NMLVCUSTOMDRAW originally didn't have the iSubItem member. It was added
// with IE4, as was IPN_FIRST which is used as a test :-(.
//
#ifndef IPN_FIRST

typedef struct wxtagNMLVCUSTOMDRAW_ {
    NMCUSTOMDRAW nmcd;
    COLORREF     clrText;
    COLORREF     clrTextBk;
    int          iSubItem;
} wxNMLVCUSTOMDRAW_, *wxLPNMLVCUSTOMDRAW_;

#define NMLVCUSTOMDRAW wxNMLVCUSTOMDRAW_
#define LPNMLVCUSTOMDRAW wxLPNMLVCUSTOMDRAW_

#endif // defined IPN_FIRST

#endif // defined __VISUALC__ && __VISUALC__ <= 1100

// ----------------------------------------------------------------------------
// ListView common control
// Needed by listctrl.cpp
// ----------------------------------------------------------------------------

#ifndef LVS_EX_FULLROWSELECT
    #define LVS_EX_FULLROWSELECT 0x00000020
#endif

#ifndef LVS_EX_LABELTIP
    #define LVS_EX_LABELTIP 0x00004000
#endif

#ifndef LVS_EX_SUBITEMIMAGES
    #define LVS_EX_SUBITEMIMAGES 0x00000002
#endif

#ifndef HDN_GETDISPINFOW
    #define HDN_GETDISPINFOW (HDN_FIRST-29)
#endif

 /*
  * In addition to the above, the following are required for several compilers.
  */

#if !defined(CCS_VERT)
#define CCS_VERT                0x00000080L
#endif

#if !defined(CCS_RIGHT)
#define CCS_RIGHT               (CCS_VERT|CCS_BOTTOM)
#endif

#if !defined(TB_SETDISABLEDIMAGELIST)
    #define TB_SETDISABLEDIMAGELIST (WM_USER + 54)
#endif // !defined(TB_SETDISABLEDIMAGELIST)

#ifndef CFM_BACKCOLOR
    #define CFM_BACKCOLOR 0x04000000
#endif

#ifndef HANGUL_CHARSET
    #define HANGUL_CHARSET 129
#endif

#ifndef CCM_SETUNICODEFORMAT
    #define CCM_SETUNICODEFORMAT 8197
#endif

// ----------------------------------------------------------------------------
// Tree control
// ----------------------------------------------------------------------------

#ifndef TV_FIRST
    #define TV_FIRST                0x1100
#endif

#ifndef TVS_FULLROWSELECT
    #define TVS_FULLROWSELECT       0x1000
#endif

#ifndef TVM_SETBKCOLOR
    #define TVM_SETBKCOLOR          (TV_FIRST + 29)
    #define TVM_SETTEXTCOLOR        (TV_FIRST + 30)
#endif

 /*
  * The following are required for BC++ 5.5 (none at present.)
  */

 /*
  * The following are specifically required for Digital Mars C++
  */

#ifdef __DMC__

typedef struct _OSVERSIONINFOEX {
    DWORD dwOSVersionInfoSize;
    DWORD dwMajorVersion;
    DWORD dwMinorVersion;
    DWORD dwBuildNumber;
    DWORD dwPlatformId;
    TCHAR szCSDVersion[ 128 ];
    WORD  wServicePackMajor;
    WORD  wServicePackMinor;
    WORD  wSuiteMask;
    BYTE  wProductType;
    BYTE  wReserved;
} OSVERSIONINFOEX;

#ifndef _TrackMouseEvent
    #define _TrackMouseEvent TrackMouseEvent
#endif

#ifndef LVM_SETEXTENDEDLISTVIEWSTYLE
    #define LVM_SETEXTENDEDLISTVIEWSTYLE (0x1000 + 54)
#endif

#ifndef LVM_GETSUBITEMRECT
    #define LVM_GETSUBITEMRECT           (0x1000 + 56)
#endif

#ifndef LVCF_IMAGE
    #define LVCF_IMAGE             0x0010
#endif

#ifndef Header_GetItemRect
    #define Header_GetItemRect(w,i,r) \
            (BOOL)SendMessage((w),HDM_GETITEMRECT,(WPARAM)(i),(LPARAM)(r))
#endif

#ifndef HDM_GETITEMRECT
    #define HDM_GETITEMRECT (HDM_FIRST+7)
#endif

#ifndef ListView_GetHeader
    #define ListView_GetHeader(w) (HWND)SendMessage((w),LVM_GETHEADER,0,0)
#endif

#ifndef ListView_GetSubItemRect
    #define ListView_GetSubItemRect(w, i, s, c, p) (HWND)SendMessage(w,LVM_GETSUBITEMRECT,i, ((p) ? ((((LPRECT)(p))->top = s), (((LPRECT)(p))->left = c), (LPARAM)(p)) : (LPARAM)(LPRECT)NULL))
#endif

#ifndef LVM_GETHEADER
    #define LVM_GETHEADER (LVM_FIRST+31)
#endif

#ifndef LVSICF_NOSCROLL
    #define LVSICF_NOINVALIDATEALL  0x0001
    #define LVSICF_NOSCROLL         0x0002
#endif

#ifndef CP_SYMBOL
    #define CP_SYMBOL 42
#endif

// ----------------------------------------------------------------------------
// wxDisplay
// ----------------------------------------------------------------------------

// The windows headers with Digital Mars lack some typedefs.
// typedef them as my_XXX and then #define to rename to XXX in case
// a newer version of Digital Mars fixes the headers
// (or up to date PSDK is in use with older version)
// also we use any required definition (MONITOR_DEFAULTTONULL) to recognize
// whether whole missing block needs to be included

#ifndef MONITOR_DEFAULTTONULL

    #define HMONITOR_DECLARED
    DECLARE_HANDLE(HMONITOR);
    typedef BOOL(CALLBACK* my_MONITORENUMPROC)(HMONITOR,HDC,LPRECT,LPARAM);
    #define MONITORENUMPROC my_MONITORENUMPROC
    typedef struct my_tagMONITORINFO {
        DWORD cbSize;
        RECT rcMonitor;
        RECT rcWork;
        DWORD dwFlags;
    } my_MONITORINFO,*my_LPMONITORINFO;
    #define MONITORINFO my_MONITORINFO
    #define LPMONITORINFO my_LPMONITORINFO

    typedef struct my_MONITORINFOEX : public my_tagMONITORINFO
    {
        TCHAR       szDevice[CCHDEVICENAME];
    } my_MONITORINFOEX, *my_LPMONITORINFOEX;
    #define MONITORINFOEX my_MONITORINFOEX
    #define LPMONITORINFOEX my_LPMONITORINFOEX

    #ifndef MONITOR_DEFAULTTONULL
        #define MONITOR_DEFAULTTONULL 0
    #endif // MONITOR_DEFAULTTONULL

    #ifndef MONITORINFOF_PRIMARY
        #define MONITORINFOF_PRIMARY 1
    #endif // MONITORINFOF_PRIMARY

    #ifndef DDENUM_ATTACHEDSECONDARYDEVICES
        #define DDENUM_ATTACHEDSECONDARYDEVICES 1
    #endif

#endif // MONITOR_DEFAULTTONULL

// ----------------------------------------------------------------------------
// Tree control
// ----------------------------------------------------------------------------

#ifndef TVIS_FOCUSED
    #define TVIS_FOCUSED            0x0001
#endif

#ifndef TVS_CHECKBOXES
    #define TVS_CHECKBOXES          0x0100
#endif

#ifndef TVITEM
    #define TVITEM TV_ITEM
#endif

#endif
    // DMC++

 /*
  * The following are specifically required for OpenWatcom C++ (none at present)
  */

#if defined(__WATCOMC__)
#endif

 /*
  * The following are specifically required for MinGW (none at present)
  */

#if defined (__MINGW32__)

#if !wxCHECK_W32API_VERSION(3,1)

#include <windows.h>
#include "wx/msw/winundef.h"

typedef struct
{
    RECT       rgrc[3];
    WINDOWPOS *lppos;
} NCCALCSIZE_PARAMS, *LPNCCALCSIZE_PARAMS;

#endif

#endif

 /*
  * In addition to the declarations for VC++, the following are required for WinCE
  */

#ifdef __WXWINCE__
    #include "wx/msw/wince/missing.h"
#endif

 /*
  * The following are specifically required for Wine
  */

#ifdef __WINE__
    #ifndef ENUM_CURRENT_SETTINGS
        #define ENUM_CURRENT_SETTINGS   ((DWORD)-1)
    #endif
    #ifndef BROADCAST_QUERY_DENY
        #define BROADCAST_QUERY_DENY    1112363332
    #endif
#endif  // defined __WINE__

#endif
    // _WX_MISSING_H_
