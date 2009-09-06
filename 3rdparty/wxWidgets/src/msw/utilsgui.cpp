///////////////////////////////////////////////////////////////////////////////
// Name:        msw/utilsgui.cpp
// Purpose:     Various utility functions only available in GUI
// Author:      Vadim Zeitlin
// Modified by:
// Created:     21.06.2003 (extracted from msw/utils.cpp)
// RCS-ID:      $Id: utilsgui.cpp 40369 2006-07-29 20:33:10Z VZ $
// Copyright:   (c) Julian Smart
// License:     wxWindows licence
///////////////////////////////////////////////////////////////////////////////

// ============================================================================
// declarations
// ============================================================================

// ----------------------------------------------------------------------------
// headers
// ----------------------------------------------------------------------------

// for compilers that support precompilation, includes "wx.h".
#include "wx/wxprec.h"

#ifdef __BORLANDC__
    #pragma hdrstop
#endif

#ifndef WX_PRECOMP
    #include "wx/cursor.h"
    #include "wx/window.h"
    #include "wx/utils.h"
#endif //WX_PRECOMP

#include "wx/dynlib.h"

#include "wx/msw/private.h"     // includes <windows.h>

// ============================================================================
// implementation
// ============================================================================

// ----------------------------------------------------------------------------
// functions to work with .INI files
// ----------------------------------------------------------------------------

// Reading and writing resources (eg WIN.INI, .Xdefaults)
#if wxUSE_RESOURCES
bool wxWriteResource(const wxString& section, const wxString& entry, const wxString& value, const wxString& file)
{
  if (file != wxEmptyString)
// Don't know what the correct cast should be, but it doesn't
// compile in BC++/16-bit without this cast.
#if !defined(__WIN32__)
    return (WritePrivateProfileString((const char*) section, (const char*) entry, (const char*) value, (const char*) file) != 0);
#else
    return (WritePrivateProfileString((LPCTSTR)WXSTRINGCAST section, (LPCTSTR)WXSTRINGCAST entry, (LPCTSTR)value, (LPCTSTR)WXSTRINGCAST file) != 0);
#endif
  else
    return (WriteProfileString((LPCTSTR)WXSTRINGCAST section, (LPCTSTR)WXSTRINGCAST entry, (LPCTSTR)WXSTRINGCAST value) != 0);
}

bool wxWriteResource(const wxString& section, const wxString& entry, float value, const wxString& file)
{
    wxString buf;
    buf.Printf(wxT("%.4f"), value);

    return wxWriteResource(section, entry, buf, file);
}

bool wxWriteResource(const wxString& section, const wxString& entry, long value, const wxString& file)
{
    wxString buf;
    buf.Printf(wxT("%ld"), value);

    return wxWriteResource(section, entry, buf, file);
}

bool wxWriteResource(const wxString& section, const wxString& entry, int value, const wxString& file)
{
    wxString buf;
    buf.Printf(wxT("%d"), value);

    return wxWriteResource(section, entry, buf, file);
}

bool wxGetResource(const wxString& section, const wxString& entry, wxChar **value, const wxString& file)
{
    static const wxChar defunkt[] = wxT("$$default");

    wxChar buf[1024];
    if (file != wxEmptyString)
    {
        int n = GetPrivateProfileString(section, entry, defunkt,
                                        buf, WXSIZEOF(buf), file);
        if (n == 0 || wxStrcmp(buf, defunkt) == 0)
            return false;
    }
    else
    {
        int n = GetProfileString(section, entry, defunkt, buf, WXSIZEOF(buf));
        if (n == 0 || wxStrcmp(buf, defunkt) == 0)
            return false;
    }
    if (*value) delete[] (*value);
    *value = wxStrcpy(new wxChar[wxStrlen(buf) + 1], buf);
    return true;
}

bool wxGetResource(const wxString& section, const wxString& entry, float *value, const wxString& file)
{
    wxChar *s = NULL;
    bool succ = wxGetResource(section, entry, (wxChar **)&s, file);
    if (succ)
    {
        *value = (float)wxStrtod(s, NULL);
        delete[] s;
        return true;
    }
    else return false;
}

bool wxGetResource(const wxString& section, const wxString& entry, long *value, const wxString& file)
{
    wxChar *s = NULL;
    bool succ = wxGetResource(section, entry, (wxChar **)&s, file);
    if (succ)
    {
        *value = wxStrtol(s, NULL, 10);
        delete[] s;
        return true;
    }
    else return false;
}

bool wxGetResource(const wxString& section, const wxString& entry, int *value, const wxString& file)
{
    wxChar *s = NULL;
    bool succ = wxGetResource(section, entry, (wxChar **)&s, file);
    if (succ)
    {
        *value = (int)wxStrtol(s, NULL, 10);
        delete[] s;
        return true;
    }
    else return false;
}
#endif // wxUSE_RESOURCES

// ---------------------------------------------------------------------------
// helper functions for showing a "busy" cursor
// ---------------------------------------------------------------------------

static HCURSOR gs_wxBusyCursor = 0;     // new, busy cursor
static HCURSOR gs_wxBusyCursorOld = 0;  // old cursor
static int gs_wxBusyCursorCount = 0;

extern HCURSOR wxGetCurrentBusyCursor()
{
    return gs_wxBusyCursor;
}

// Set the cursor to the busy cursor for all windows
void wxBeginBusyCursor(const wxCursor *cursor)
{
    if ( gs_wxBusyCursorCount++ == 0 )
    {
        gs_wxBusyCursor = (HCURSOR)cursor->GetHCURSOR();
#ifndef __WXMICROWIN__
        gs_wxBusyCursorOld = ::SetCursor(gs_wxBusyCursor);
#endif
    }
    //else: nothing to do, already set
}

// Restore cursor to normal
void wxEndBusyCursor()
{
    wxCHECK_RET( gs_wxBusyCursorCount > 0,
                 wxT("no matching wxBeginBusyCursor() for wxEndBusyCursor()") );

    if ( --gs_wxBusyCursorCount == 0 )
    {
#ifndef __WXMICROWIN__
        ::SetCursor(gs_wxBusyCursorOld);
#endif
        gs_wxBusyCursorOld = 0;
    }
}

// true if we're between the above two calls
bool wxIsBusy()
{
  return gs_wxBusyCursorCount > 0;
}

// Check whether this window wants to process messages, e.g. Stop button
// in long calculations.
bool wxCheckForInterrupt(wxWindow *wnd)
{
    wxCHECK( wnd, false );

    MSG msg;
    while ( ::PeekMessage(&msg, GetHwndOf(wnd), 0, 0, PM_REMOVE) )
    {
        ::TranslateMessage(&msg);
        ::DispatchMessage(&msg);
    }

    return true;
}

// MSW only: get user-defined resource from the .res file.
// Returns NULL or newly-allocated memory, so use delete[] to clean up.

#ifndef __WXMICROWIN__
wxChar *wxLoadUserResource(const wxString& resourceName, const wxString& resourceType)
{
    HRSRC hResource = ::FindResource(wxGetInstance(), resourceName, resourceType);
    if ( hResource == 0 )
        return NULL;

    HGLOBAL hData = ::LoadResource(wxGetInstance(), hResource);
    if ( hData == 0 )
        return NULL;

    wxChar *theText = (wxChar *)::LockResource(hData);
    if ( !theText )
        return NULL;

    // Not all compilers put a zero at the end of the resource (e.g. BC++ doesn't).
    // so we need to find the length of the resource.
    int len = ::SizeofResource(wxGetInstance(), hResource);
    wxChar  *s = new wxChar[len+1];
    wxStrncpy(s,theText,len);
    s[len]=0;

    // wxChar *s = copystring(theText);

    // Obsolete in WIN32
#ifndef __WIN32__
    UnlockResource(hData);
#endif

    // No need??
    //  GlobalFree(hData);

    return s;
}
#endif // __WXMICROWIN__

// ----------------------------------------------------------------------------
// get display info
// ----------------------------------------------------------------------------

// See also the wxGetMousePosition in window.cpp
// Deprecated: use wxPoint wxGetMousePosition() instead
void wxGetMousePosition( int* x, int* y )
{
    POINT pt;
    GetCursorPos( & pt );
    if ( x ) *x = pt.x;
    if ( y ) *y = pt.y;
}

// Return true if we have a colour display
bool wxColourDisplay()
{
#ifdef __WXMICROWIN__
    // MICROWIN_TODO
    return true;
#else
    // this function is called from wxDC ctor so it is called a *lot* of times
    // hence we optimize it a bit but doing the check only once
    //
    // this should be MT safe as only the GUI thread (holding the GUI mutex)
    // can call us
    static int s_isColour = -1;

    if ( s_isColour == -1 )
    {
        ScreenHDC dc;
        int noCols = ::GetDeviceCaps(dc, NUMCOLORS);

        s_isColour = (noCols == -1) || (noCols > 2);
    }

    return s_isColour != 0;
#endif
}

// Returns depth of screen
int wxDisplayDepth()
{
    ScreenHDC dc;
    return GetDeviceCaps(dc, PLANES) * GetDeviceCaps(dc, BITSPIXEL);
}

// Get size of display
void wxDisplaySize(int *width, int *height)
{
#ifdef __WXMICROWIN__
    RECT rect;
    HWND hWnd = GetDesktopWindow();
    ::GetWindowRect(hWnd, & rect);

    if ( width )
        *width = rect.right - rect.left;
    if ( height )
        *height = rect.bottom - rect.top;
#else // !__WXMICROWIN__
    ScreenHDC dc;

    if ( width )
        *width = ::GetDeviceCaps(dc, HORZRES);
    if ( height )
        *height = ::GetDeviceCaps(dc, VERTRES);
#endif // __WXMICROWIN__/!__WXMICROWIN__
}

void wxDisplaySizeMM(int *width, int *height)
{
#ifdef __WXMICROWIN__
    // MICROWIN_TODO
    if ( width )
        *width = 0;
    if ( height )
        *height = 0;
#else
    ScreenHDC dc;

    if ( width )
        *width = ::GetDeviceCaps(dc, HORZSIZE);
    if ( height )
        *height = ::GetDeviceCaps(dc, VERTSIZE);
#endif
}

void wxClientDisplayRect(int *x, int *y, int *width, int *height)
{
#if defined(__WXMICROWIN__)
    *x = 0; *y = 0;
    wxDisplaySize(width, height);
#else
    // Determine the desktop dimensions minus the taskbar and any other
    // special decorations...
    RECT r;

    SystemParametersInfo(SPI_GETWORKAREA, 0, &r, 0);
    if (x)      *x = r.left;
    if (y)      *y = r.top;
    if (width)  *width = r.right - r.left;
    if (height) *height = r.bottom - r.top;
#endif
}

// ---------------------------------------------------------------------------
// window information functions
// ---------------------------------------------------------------------------

wxString WXDLLEXPORT wxGetWindowText(WXHWND hWnd)
{
    wxString str;

    if ( hWnd )
    {
        int len = GetWindowTextLength((HWND)hWnd) + 1;
        ::GetWindowText((HWND)hWnd, wxStringBuffer(str, len), len);
    }

    return str;
}

wxString WXDLLEXPORT wxGetWindowClass(WXHWND hWnd)
{
    wxString str;

    // MICROWIN_TODO
#ifndef __WXMICROWIN__
    if ( hWnd )
    {
        int len = 256; // some starting value

        for ( ;; )
        {
            int count = ::GetClassName((HWND)hWnd, wxStringBuffer(str, len), len);

            if ( count == len )
            {
                // the class name might have been truncated, retry with larger
                // buffer
                len *= 2;
            }
            else
            {
                break;
            }
        }
    }
#endif // !__WXMICROWIN__

    return str;
}

WXWORD WXDLLEXPORT wxGetWindowId(WXHWND hWnd)
{
    return (WXWORD)GetWindowLong((HWND)hWnd, GWL_ID);
}

// ----------------------------------------------------------------------------
// Metafile helpers
// ----------------------------------------------------------------------------

extern void PixelToHIMETRIC(LONG *x, LONG *y)
{
    ScreenHDC hdcRef;

    int iWidthMM = GetDeviceCaps(hdcRef, HORZSIZE),
        iHeightMM = GetDeviceCaps(hdcRef, VERTSIZE),
        iWidthPels = GetDeviceCaps(hdcRef, HORZRES),
        iHeightPels = GetDeviceCaps(hdcRef, VERTRES);

    *x *= (iWidthMM * 100);
    *x /= iWidthPels;
    *y *= (iHeightMM * 100);
    *y /= iHeightPels;
}

extern void HIMETRICToPixel(LONG *x, LONG *y)
{
    ScreenHDC hdcRef;

    int iWidthMM = GetDeviceCaps(hdcRef, HORZSIZE),
        iHeightMM = GetDeviceCaps(hdcRef, VERTSIZE),
        iWidthPels = GetDeviceCaps(hdcRef, HORZRES),
        iHeightPels = GetDeviceCaps(hdcRef, VERTRES);

    *x *= iWidthPels;
    *x /= (iWidthMM * 100);
    *y *= iHeightPels;
    *y /= (iHeightMM * 100);
}

void wxDrawLine(HDC hdc, int x1, int y1, int x2, int y2)
{
#ifdef __WXWINCE__
    POINT points[2];
    points[0].x = x1;
    points[0].y = y1;
    points[1].x = x2;
    points[1].y = y2;
    Polyline(hdc, points, 2);
#else
    MoveToEx(hdc, x1, y1, NULL); LineTo((HDC) hdc, x2, y2);
#endif
}


// ----------------------------------------------------------------------------
// Shell API wrappers
// ----------------------------------------------------------------------------

extern bool wxEnableFileNameAutoComplete(HWND hwnd)
{
#if wxUSE_DYNLIB_CLASS
    typedef HRESULT (WINAPI *SHAutoComplete_t)(HWND, DWORD);

    static SHAutoComplete_t s_pfnSHAutoComplete = NULL;
    static bool s_initialized = false;

    if ( !s_initialized )
    {
        s_initialized = true;

        wxLogNull nolog;
        wxDynamicLibrary dll(_T("shlwapi.dll"));
        if ( dll.IsLoaded() )
        {
            s_pfnSHAutoComplete =
                (SHAutoComplete_t)dll.GetSymbol(_T("SHAutoComplete"));
            if ( s_pfnSHAutoComplete )
            {
                // won't be unloaded until the process termination, no big deal
                dll.Detach();
            }
        }
    }

    if ( !s_pfnSHAutoComplete )
        return false;

    HRESULT hr = s_pfnSHAutoComplete(hwnd, 0x10 /* SHACF_FILESYS_ONLY */);
    if ( FAILED(hr) )
    {
        wxLogApiError(_T("SHAutoComplete"), hr);
        return false;
    }

    return true;
#else
    wxUnusedVar(hwnd);
    return false;
#endif // wxUSE_DYNLIB_CLASS/!wxUSE_DYNLIB_CLASS
}

