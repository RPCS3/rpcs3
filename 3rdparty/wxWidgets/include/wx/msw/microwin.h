/*
/////////////////////////////////////////////////////////////////////////////
// Name:        microwin.h
// Purpose:     Extra implementation for MicroWindows
// Author:      Julian Smart
// Created:     2001-05-31
// RCS-ID:      $Id: microwin.h 35650 2005-09-23 12:56:45Z MR $
// Copyright:   (c) Julian Smart
// Licence:     wxWindows licence
/////////////////////////////////////////////////////////////////////////////
*/

#ifndef _WX_MICROWIN_H_
#define _WX_MICROWIN_H_

/* Implemented by microwin.cpp */

#ifdef __cplusplus
extern "C" {
#endif

BOOL SetCursorPos(int x, int y);

HCURSOR SetCursor(HCURSOR hCursor);

/* Implemented with wrong number of args by MicroWindows */
/* so we need to use a different name */
int GetScrollPosWX (HWND hWnd, int iSBar);

BOOL ScrollWindow(HWND, int xAmount, int yAmount,
                 CONST RECT* lpRect, CONST RECT* lpClipRect);

HWND WindowFromPoint(POINT pt);
SHORT GetKeyState(int nVirtKey);
HWND  SetParent(HWND hWndChild, HWND hWndNewParent);
VOID DragAcceptFiles(HWND, BOOL);
BOOL IsDialogMessage(HWND hWnd, MSG* msg);
DWORD GetMessagePos(VOID);
BOOL IsIconic(HWND hWnd);
int SetMapMode(HDC hDC, int mode);
int GetMapMode(HDC hDC);
HCURSOR LoadCursor(HINSTANCE hInst, int cursor);
DWORD GetModuleFileName(HINSTANCE hInst, LPSTR name, DWORD sz);
VOID DestroyIcon(HICON hIcon);
COLORREF GetTextColor(HDC hdc);
COLORREF GetBkColor(HDC hdc);
HPALETTE SelectPalette(HDC hdc, HPALETTE hPalette, BOOL b);
BOOL IntersectClipRect(HDC hdc, int x, int y,
               int w, int h);
BOOL GetClipBox(HDC hdc, RECT* rect);
BOOL DrawIconEx(HDC hdc, int x, int y, HICON hIcon, int w, int h, UINT istepIfAniCur, HBRUSH hbrFlickerFreeDraw, UINT diFlags);
BOOL SetViewportExtEx(HDC hdc, int x, int y, LPSIZE lpSize);
BOOL SetViewportOrgEx(HDC hdc, int x, int y, LPPOINT lpPoint);
BOOL SetWindowExtEx(HDC hdc, int x, int y, LPSIZE lpSize);
BOOL SetWindowOrgEx(HDC hdc, int x, int y, LPPOINT lpSize);
BOOL ExtFloodFill(HDC hdc, int x, int y, COLORREF col, UINT flags);
int SetPolyFillMode(HDC hdc, int mode);
BOOL RoundRect(HDC hdc, int left, int top, int right, int bottom, int r1, int r2);
BOOL MaskBlt(HDC hdc, int x, int y, int w, int h,
              HDC hDCSource, int xSrc, int ySrc, HBITMAP hBitmapMask, int xMask, int yMask, DWORD rop);
UINT RealizePalette(HDC hDC);
BOOL SetBrushOrgEx(HDC hdc, int xOrigin, int yOrigin, LPPOINT lpPoint);
int GetObject(HGDIOBJ hObj, int sz, LPVOID logObj);

/* For some reason these aren't defined in the headers */
BOOL  EnableScrollBar (HWND hWnd, int iSBar, BOOL bEnable) ;
BOOL  GetScrollPos (HWND hWnd, int iSBar, int* pPos);
BOOL  GetScrollRange (HWND hWnd, int iSBar, int* pMinPos, int* pMaxPos);
BOOL  SetScrollPos (HWND hWnd, int iSBar, int iNewPos);
BOOL  SetScrollRange (HWND hWnd, int iSBar, int iMinPos, int iMaxPos);
BOOL  SetScrollInfo (HWND hWnd, int iSBar,
             LPCSCROLLINFO lpsi, BOOL fRedraw);
BOOL  GetScrollInfo(HWND hWnd, int iSBar, LPSCROLLINFO lpsi);
BOOL  ShowScrollBar (HWND hWnd, int iSBar, BOOL bShow);
HBITMAP WINAPI
CreateBitmap( int width, int height, int nPlanes, int bPP, LPCVOID lpData);

#ifdef __cplusplus
}
#endif

/*
 * Key State Masks for Mouse Messages
 */
#ifndef MK_LBUTTON
#define MK_LBUTTON          0x0001
#define MK_RBUTTON          0x0002
#define MK_SHIFT            0x0004
#define MK_CONTROL          0x0008
#define MK_MBUTTON          0x0010
#endif

/*
 * DrawIcon flags
 */

#ifndef DI_MASK
#define DI_MASK         0x0001
#define DI_IMAGE        0x0002
#define DI_NORMAL       0x0003
#define DI_COMPAT       0x0004
#define DI_DEFAULTSIZE  0x0008
#endif

/* TODO: May have to fake these message */
#ifndef WM_INITDIALOG
#define WM_INITDIALOG       0x0110
#endif
#ifndef WM_QUERYENDSESSION
#define WM_QUERYENDSESSION              0x0011
#endif
#ifndef WM_ENDSESSION
#define WM_ENDSESSION                   0x0016
#endif
#ifndef WM_SETCURSOR
#define WM_SETCURSOR                    0x0020
#endif
#ifndef WM_GETMINMAXINFO
#define WM_GETMINMAXINFO                0x0024
typedef struct tagMINMAXINFO {
    POINT ptReserved;
    POINT ptMaxSize;
    POINT ptMaxPosition;
    POINT ptMinTrackSize;
    POINT ptMaxTrackSize;
} MINMAXINFO, *PMINMAXINFO, *LPMINMAXINFO;

#endif
#ifndef WM_SYSCOMMAND
#define WM_SYSCOMMAND                   0x0112
#endif
#ifndef WM_SYSCOLORCHANGE
#define WM_SYSCOLORCHANGE               0x0015
#endif
#ifndef WM_QUERYNEWPALETTE
#define WM_QUERYNEWPALETTE              0x030F
#endif
#ifndef WM_PALETTEISCHANGING
#define WM_PALETTEISCHANGING            0x0310
#endif
#ifndef WM_PALETTECHANGED
#define WM_PALETTECHANGED               0x0311
#endif
#ifndef WM_NOTIFY
#define WM_NOTIFY                       0x004E
#endif
#ifndef WM_DROPFILES
#define WM_DROPFILES                    0x0233
#endif

#ifndef PALETTERGB
#define PALETTERGB RGB
#endif

#ifndef MM_TEXT
#define MM_TEXT 1
#define MM_LOMETRIC 2
#define MM_HIMETRIC 3
#define MM_LOENGLISH 4
#define MM_HIENGLISH 5
#define MM_TWIPS 6
#define MM_ISOTROPIC 7
#define MM_ANISOTROPIC 8
#endif

#ifndef SC_MAXIMIZE
#define SC_MINIMIZE 0xF020
#define SC_MAXIMIZE 0xF030
#endif

// TODO: all of them
#ifndef IDC_ARROW
#define IDC_ARROW 1
#endif

/*
 * Standard Cursor IDs
 */
#ifndef MAKEINTRESOURCE
#define MAKEINTRESOURCE(r) r
#endif

#ifndef IDC_ARROW
#define IDC_ARROW           MAKEINTRESOURCE(32512)
#define IDC_IBEAM           MAKEINTRESOURCE(32513)
#define IDC_WAIT            MAKEINTRESOURCE(32514)
#define IDC_CROSS           MAKEINTRESOURCE(32515)
#define IDC_UPARROW         MAKEINTRESOURCE(32516)
#define IDC_SIZE            MAKEINTRESOURCE(32640) /* OBSOLETE: use IDC_SIZEALL */
#define IDC_ICON            MAKEINTRESOURCE(32641) /* OBSOLETE: use IDC_ARROW */
#define IDC_SIZENWSE        MAKEINTRESOURCE(32642)
#define IDC_SIZENESW        MAKEINTRESOURCE(32643)
#define IDC_SIZEWE          MAKEINTRESOURCE(32644)
#define IDC_SIZENS          MAKEINTRESOURCE(32645)
#define IDC_SIZEALL         MAKEINTRESOURCE(32646)
#define IDC_NO              MAKEINTRESOURCE(32648) /* not in win3.1 */
#if(WINVER >= 0x0500)
#define IDC_HAND            MAKEINTRESOURCE(32649)
#endif /* WINVER >= 0x0500 */
#define IDC_APPSTARTING     MAKEINTRESOURCE(32650) /* not in win3.1 */
#if(WINVER >= 0x0400)
#define IDC_HELP            MAKEINTRESOURCE(32651)
#endif /* WINVER >= 0x0400 */
#endif

/* ExtFloodFill style flags */
#define  FLOODFILLBORDER   0
#define  FLOODFILLSURFACE  1

/* PolyFill() Modes */
#define ALTERNATE                    1
#define WINDING                      2
#define POLYFILL_LAST                2

/* Quaternary raster codes */
#define MAKEROP4(fore,back) (DWORD)((((back) << 8) & 0xFF000000) | (fore))

/* Device Parameters for GetDeviceCaps() */
#define DRIVERVERSION 0     /* Device driver version                    */
#define TECHNOLOGY    2     /* Device classification                    */
#define HORZSIZE      4     /* Horizontal size in millimeters           */
#define VERTSIZE      6     /* Vertical size in millimeters             */

/* Ternary raster operations */
/* Now defined by MicroWindows */
#if 0
#define DSTINVERT           (DWORD)0x00550009 /* dest = (NOT dest)               */
#define WHITENESS           (DWORD)0x00FF0062 /* dest = WHITE                    */
#define SRCERASE            (DWORD)0x00440328 /* dest = source AND (NOT dest )   */
#define MERGEPAINT          (DWORD)0x00BB0226 /* dest = (NOT source) OR dest     */
#define SRCPAINT            (DWORD)0x00EE0086 /* dest = source OR dest           */
#define NOTSRCCOPY          (DWORD)0x00330008 /* dest = (NOT source)             */
#endif

#endif /* _WX_MICROWIN_H_ */
