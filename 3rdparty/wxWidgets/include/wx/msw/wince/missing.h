/////////////////////////////////////////////////////////////////////////////
// Name:        wince/missing.h
// Purpose:     Missing things in WinCE
// Author:      Marco Cavallini
// Modified by:
// Created:     16/11/2002
// RCS-ID:
// Copyright:   (c) KOAN SAS ( www.koansoftware.com )
// Licence:     wxWindows licence
///////////////////////////////////////////////////////////////////////////////

#ifndef _WX_CE_MISSING_H_
#define _WX_CE_MISSING_H_

#include "wx/msw/private.h"
#include "shellapi.h"

inline BOOL IsIconic( HWND WXUNUSED(hWnd) )
{
    // Probably not right...
#if 0
    long style = GetWindowLong(hWnd, GWL_STYLE);
    return ((style & WS_MINIMIZE) == 0);
#endif
    return FALSE;
}

#ifdef __POCKETPC__
#define SM_CXCURSOR             13
#define SM_CYCURSOR             14
#endif

// Missing from eVC 4 for some reason
#ifndef I_IMAGENONE
#define I_IMAGENONE (-2)
#endif

#ifndef TBSTYLE_NO_DROPDOWN_ARROW
#define TBSTYLE_NO_DROPDOWN_ARROW 0x0080
#endif

#ifndef SHCMBM_GETMENU
#define SHCMBM_GETMENU      (WM_USER + 402)
#endif

#ifndef SHCMBM_SETSUBMENU
#define SHCMBM_SETSUBMENU   (WM_USER + 400) // wparam == id of button, lParam == hmenu, return is old hmenu
#endif

#ifndef SHCMBM_GETSUBMENU
#define SHCMBM_GETSUBMENU   (WM_USER + 401) // lParam == ID
#endif

#ifndef LVS_EX_FULLROWSELECT
    #define LVS_EX_FULLROWSELECT 0x00000020
#endif

#ifndef TVS_FULLROWSELECT
    #define TVS_FULLROWSELECT       0x1000
#endif

#ifndef TVM_SETBKCOLOR
    #define TVM_SETBKCOLOR          (TV_FIRST + 29)
    #define TVM_SETTEXTCOLOR        (TV_FIRST + 30)
#endif

// Used in msgdlg.cpp, evtloop.cpp
#ifndef MB_TASKMODAL
#define MB_TASKMODAL 0x2000
#endif

#ifndef HGDI_ERROR
#define HGDI_ERROR ((HANDLE)(0xFFFFFFFFL))
#endif

// some windows styles don't exist in CE SDK, replace them with closest
// equivalents
#ifndef WS_THICKFRAME
    #define WS_THICKFRAME WS_BORDER
#endif

#ifndef WS_MINIMIZE
    #define WS_MINIMIZE 0
#endif

#ifndef WS_MAXIMIZE
    #define WS_MAXIMIZE 0
#endif


// global memory functions don't exist under CE (good riddance, of course, but
// the existing code still uses them in some places, so make it compile)
//
// update: they're defined in eVC 4 inside "#ifdef UNDER_CE" block
#ifndef UNDER_CE
    #define GlobalAlloc LocalAlloc
    #define GlobalFree LocalFree
    #define GlobalSize LocalSize
    #define GPTR LPTR
    #define GHND LPTR
    #define GMEM_MOVEABLE 0
    #define GMEM_SHARE 0
#endif // !UNDER_CE

// WinCE RTL doesn't implement bsearch() used in encconv.cpp
extern "C" void *
bsearch(const void *key, const void *base, size_t num, size_t size,
        int (wxCMPFUNC_CONV *cmp)(const void *, const void *));

#define O_RDONLY       0x0000  /* open for reading only */
#define O_WRONLY       0x0001  /* open for writing only */
#define O_RDWR         0x0002  /* open for reading and writing */
#define O_APPEND       0x0008  /* writes done at eof */

#define O_CREAT        0x0100  /* create and open file */
#define O_TRUNC        0x0200  /* open and truncate */
#define O_EXCL         0x0400  /* open only if file doesn't already exist */

#define O_TEXT         0x4000  /* file mode is text (translated) */
#define O_BINARY       0x8000  /* file mode is binary (untranslated) */

#ifndef SS_SUNKEN
    #define SS_SUNKEN 0x00001000L
#endif

// unsupported flags for WINDOWPOS structure
#ifndef SWP_NOCOPYBITS
    #define SWP_NOCOPYBITS 0
#endif

#ifndef SWP_NOOWNERZORDER
    #define SWP_NOOWNERZORDER 0
#endif

#ifndef SWP_NOSENDCHANGING
    #define SWP_NOSENDCHANGING 0
#endif

#endif // _WX_CE_MISSING_H_

