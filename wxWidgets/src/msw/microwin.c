/*
/////////////////////////////////////////////////////////////////////////////
// Name:        microwin.cpp
// Purpose:     Extra implementation for MicroWindows
// Author:      Julian Smart
// Created:     2001-05-31
// RCS-ID:      $Id: microwin.c 35650 2005-09-23 12:56:45Z MR $
// Copyright:   (c) Julian Smart
// Licence:     wxWidgets licence
/////////////////////////////////////////////////////////////////////////////

*/

#include "mwtypes.h"
#include "windows.h"
#include "wintern.h"
#include "device.h"
#include "wx/msw/microwin.h"

void GdMoveCursor(MWCOORD x, MWCOORD y);
void MwSetCursor(HWND wp, PMWCURSOR pcursor);

BOOL SetCursorPos(int x, int y)
{
    GdMoveCursor(x, y);
    return TRUE;
}

HCURSOR SetCursor(HCURSOR hCursor)
{
    /* TODO */
    return 0;
}

int GetScrollPosWX (HWND hWnd, int iSBar)
{
    int pos = 0;
    if (GetScrollPos(hWnd, iSBar, & pos))
        return pos;
    else
        return 0;
}

BOOL ScrollWindow(HWND hWnd, int xAmount, int yAmount,
		  CONST RECT* lpRect, CONST RECT* lpClipRect)
{
    /* TODO */
    return FALSE;
}

HWND WindowFromPoint(POINT pt)
{
    /* TODO */
    return 0;
}

SHORT GetKeyState(int nVirtKey)
{
    /* TODO */
    return 0;
}

HWND SetParent(HWND hWndChild, HWND hWndNewParent)
{
    /* TODO */
    return 0;
}

VOID DragAcceptFiles(HWND hWnd, BOOL b)
{
    /* TODO */
}

BOOL IsDialogMessage(HWND hWnd, MSG* msg)
{
    /* TODO */
    return FALSE;
}

DWORD GetMessagePos(VOID)
{
    /* TODO */
    return 0;
}

BOOL IsIconic(HWND hWnd)
{
    /* TODO */
    return FALSE;
}

int SetMapMode(HDC hDC, int mode)
{
    return MM_TEXT;
}

int GetMapMode(HDC hDC)
{
    return MM_TEXT;
}

HCURSOR LoadCursor(HINSTANCE hInst, int cursor)
{
    /* TODO */
    return 0;
}

DWORD GetModuleFileName(HINSTANCE hInst, LPSTR name, DWORD sz)
{
    /* TODO */
    name[0] = 0;
    return 0;
}

VOID DestroyIcon(HICON hIcon)
{
    /* TODO */
}

COLORREF GetTextColor(HDC hdc)
{
    if (!hdc)
        return CLR_INVALID;
    return hdc->textcolor ;
}

COLORREF GetBkColor(HDC hdc)
{
    if (!hdc)
        return CLR_INVALID;
    return hdc->bkcolor ;
}

HPALETTE SelectPalette(HDC hdc, HPALETTE hPalette, BOOL b)
{
    /* TODO */
    return 0;
}

BOOL IntersectClipRect(HDC hdc, int x, int y,
                                int right, int bottom)
{
    /* TODO */
    HRGN rgn = CreateRectRgn(x, y, right, bottom);

    BOOL ret = (ExtSelectClipRgn(hdc, rgn, RGN_AND) != ERROR);
    DeleteObject(rgn);
    return ret;
}

BOOL GetClipBox(HDC hdc, RECT* rect)
{
    MWCLIPREGION* r;
    MWRECT mwrect;

    if (!hdc->region)
        return FALSE;

    r = ((MWRGNOBJ*) hdc->region)->rgn;
    GdGetRegionBox(r, & mwrect);
    rect->left = mwrect.left;
    rect->top = mwrect.top;
    rect->right = mwrect.right;
    rect->bottom = mwrect.bottom;

    return TRUE;    
}

BOOL DrawIconEx(HDC hdc, int x, int y, HICON hIcon, int w, int h, UINT istepIfAniCur, HBRUSH hbrFlickerFreeDraw, UINT diFlags)
{
    /* TODO */
    return FALSE;
}

BOOL SetViewportExtEx(HDC hdc, int x, int y, LPSIZE lpSize)
{
    /* TODO */
    return FALSE;
}

BOOL SetViewportOrgEx(HDC hdc, int x, int y, LPPOINT lpPoint)
{
    /* TODO */
    return FALSE;
}

BOOL SetWindowExtEx(HDC hdc, int x, int y, LPSIZE lpSize)
{
    /* TODO */
    return FALSE;
}

BOOL SetWindowOrgEx(HDC hdc, int x, int y, LPPOINT lpSize)
{
    /* TODO */
    return FALSE;
}

BOOL ExtFloodFill(HDC hdc, int x, int y, COLORREF col, UINT flags)
{
    /* TODO */
    return FALSE;
}

int SetPolyFillMode(HDC hdc, int mode)
{
    /* TODO */
    return 0;
}

BOOL RoundRect(HDC hdc, int left, int top, int right, int bottom, int r1, int r2)
{
    /* TODO */
    return Rectangle(hdc, left, top, right, bottom);
}

BOOL MaskBlt(HDC hdc, int x, int y, int w, int h,
              HDC hDCSource, int xSrc, int ySrc, HBITMAP hBitmapMask, int xMask, int yMask, DWORD rop)
{
    /* TODO */
    return FALSE;  
}

UINT RealizePalette(HDC hDC)
{
    /* TODO */
    return 0;
}

BOOL SetBrushOrgEx(HDC hdc, int xOrigin, int yOrigin, LPPOINT lpPoint)
{
    /* TODO */
    return FALSE;
}

int GetObject(HGDIOBJ hObj, int sz, LPVOID logObj)
{
    if (sz == sizeof(LOGFONT))
    {
        LOGFONT* logFont = (LOGFONT*) logObj;
        MWFONTINFO fi;
        HFONT hFont = (HFONT) hObj;

        GdGetFontInfo(((MWFONTOBJ*) hFont)->pfont, &fi);

	/* FIXME many items are guessed for the time being*/
	logFont->lfHeight = fi.height;

   	 /* reversed for kaffe port
	logFont->tmAscent = fi.height - fi.baseline;
	logFont->tmDescent= fi.baseline;
	 */

	logFont->lfWidth = fi.widths['x'];
	logFont->lfWeight = FW_NORMAL;
        logFont->lfEscapement = 0;
        logFont->lfOrientation = 0;
        logFont->lfOutPrecision = OUT_OUTLINE_PRECIS;
        logFont->lfClipPrecision = CLIP_DEFAULT_PRECIS;
        logFont->lfQuality = DEFAULT_QUALITY;
	logFont->lfItalic = 0;
	logFont->lfUnderline = 0;
	logFont->lfStrikeOut = 0;
	/* note that win32 has the TMPF_FIXED_PITCH flags REVERSED...*/
	logFont->lfPitchAndFamily = fi.fixed?
			FF_DONTCARE: (FF_DONTCARE | TMPF_FIXED_PITCH);
	logFont->lfCharSet = OEM_CHARSET;
	/* TODO I don't know how to get the font name. May
	 * test for different font classes.
         */
        logFont->lfFaceName[0] = 0;
#if 0
	strncpy(logFont->lfFaceName, ??, sizeof(logFont->lfFaceName));
#endif
        return sz;
    }
    else
    {
        return 0;
    }
}

/* Not in wingdi.c in earlier versions of MicroWindows */
#if 0
HBITMAP WINAPI
CreateCompatibleBitmap(HDC hdc, int nWidth, int nHeight)
{
	MWBITMAPOBJ *	hbitmap;
	int		size;
	int		linelen;

	if(!hdc)
		return NULL;

	nWidth = MWMAX(nWidth, 1);
	nHeight = MWMAX(nHeight, 1);

	/* calc memory allocation size and linelen from width and height*/
	if(!GdCalcMemGCAlloc(hdc->psd, nWidth, nHeight, 0, 0, &size, &linelen))
		return NULL;

	/* allocate gdi object*/
	hbitmap = (MWBITMAPOBJ *)GdItemAlloc(sizeof(MWBITMAPOBJ)-1+size);
	if(!hbitmap)
		return NULL;
	hbitmap->hdr.type = OBJ_BITMAP;
	hbitmap->hdr.stockobj = FALSE;
	hbitmap->width = nWidth;
	hbitmap->height = nHeight;

	/* create compatible with hdc*/
	hbitmap->planes = hdc->psd->planes;
	hbitmap->bpp = hdc->psd->bpp;
	hbitmap->linelen = linelen;
	hbitmap->size = size;

	return (HBRUSH)hbitmap;
}
#endif

/* Attempt by JACS to create arbitrary bitmap
 * TODO: make use of lpData
 */

HBITMAP WINAPI
CreateBitmap( int nWidth, int nHeight, int nPlanes, int bPP, LPCVOID lpData)
{
	MWBITMAPOBJ *	hbitmap;
	int		size;
	int		linelen;

        HDC hScreenDC;

        hScreenDC = GetDC(NULL);

	nWidth = MWMAX(nWidth, 1);
	nHeight = MWMAX(nHeight, 1);

	/* calc memory allocation size and linelen from width and height*/
	if(!GdCalcMemGCAlloc(hScreenDC->psd, nWidth, nHeight, nPlanes, bPP, &size, &linelen))
        {
                ReleaseDC(NULL, hScreenDC);
		return NULL;
	}
        ReleaseDC(NULL, hScreenDC);

	/* allocate gdi object*/
	hbitmap = (MWBITMAPOBJ *)GdItemAlloc(sizeof(MWBITMAPOBJ)-1+size);
	if(!hbitmap)
		return NULL;
	hbitmap->hdr.type = OBJ_BITMAP;
	hbitmap->hdr.stockobj = FALSE;
	hbitmap->width = nWidth;
	hbitmap->height = nHeight;

	/* create with specified parameters */
	hbitmap->planes = nPlanes;
	hbitmap->bpp = bPP;
	hbitmap->linelen = linelen;
	hbitmap->size = size;

        /* TODO: copy data */

	return (HBRUSH)hbitmap;
}
