/////////////////////////////////////////////////////////////////////////////
// Name:        wx/msw/private.h
// Purpose:     Private declarations: as this header is only included by
//              wxWidgets itself, it may contain identifiers which don't start
//              with "wx".
// Author:      Julian Smart
// Modified by:
// Created:     01/02/97
// RCS-ID:      $Id: private.h 62189 2009-09-28 16:02:08Z JS $
// Copyright:   (c) Julian Smart
// Licence:     wxWindows licence
/////////////////////////////////////////////////////////////////////////////

#ifndef _WX_PRIVATE_H_
#define _WX_PRIVATE_H_

#include "wx/msw/wrapwin.h"

#ifdef __WXMICROWIN__
    // Extra prototypes and symbols not defined by MicroWindows
    #include "wx/msw/microwin.h"
#endif

#include "wx/log.h"

class WXDLLIMPEXP_FWD_CORE wxFont;
class WXDLLIMPEXP_FWD_CORE wxWindow;
class WXDLLIMPEXP_FWD_CORE wxWindowBase;

// ---------------------------------------------------------------------------
// private constants
// ---------------------------------------------------------------------------

// 260 was taken from windef.h
#ifndef MAX_PATH
    #define MAX_PATH  260
#endif

// ---------------------------------------------------------------------------
// standard icons from the resources
// ---------------------------------------------------------------------------

#if wxUSE_GUI

extern WXDLLEXPORT_DATA(HICON) wxSTD_FRAME_ICON;
extern WXDLLEXPORT_DATA(HICON) wxSTD_MDIPARENTFRAME_ICON;
extern WXDLLEXPORT_DATA(HICON) wxSTD_MDICHILDFRAME_ICON;
extern WXDLLEXPORT_DATA(HICON) wxDEFAULT_FRAME_ICON;
extern WXDLLEXPORT_DATA(HICON) wxDEFAULT_MDIPARENTFRAME_ICON;
extern WXDLLEXPORT_DATA(HICON) wxDEFAULT_MDICHILDFRAME_ICON;
extern WXDLLEXPORT_DATA(HFONT) wxSTATUS_LINE_FONT;

#endif // wxUSE_GUI

// ---------------------------------------------------------------------------
// global data
// ---------------------------------------------------------------------------

extern WXDLLIMPEXP_DATA_BASE(HINSTANCE) wxhInstance;

// ---------------------------------------------------------------------------
// define things missing from some compilers' headers
// ---------------------------------------------------------------------------

#if defined(__WXWINCE__) || (defined(__GNUWIN32__) && !wxUSE_NORLANDER_HEADERS)
#ifndef ZeroMemory
    inline void ZeroMemory(void *buf, size_t len) { memset(buf, 0, len); }
#endif
#endif // old mingw32

// this defines a CASTWNDPROC macro which casts a pointer to the type of a
// window proc
#if defined(STRICT) || defined(__GNUC__)
    typedef WNDPROC WndProcCast;
#else
    typedef FARPROC WndProcCast;
#endif


#define CASTWNDPROC (WndProcCast)



// ---------------------------------------------------------------------------
// some stuff for old Windows versions (FIXME: what does it do here??)
// ---------------------------------------------------------------------------

#if !defined(APIENTRY)  // NT defines APIENTRY, 3.x not
    #define APIENTRY FAR PASCAL
#endif

#ifdef __WIN32__
    #define _EXPORT
#else
    #define _EXPORT _export
#endif

#ifndef __WIN32__
    typedef signed short int SHORT;
#endif

#if !defined(__WIN32__)  // 3.x uses FARPROC for dialogs
#ifndef STRICT
    #define DLGPROC FARPROC
#endif
#endif

/*
 * Decide what window classes we're going to use
 * for this combination of CTl3D/FAFA settings
 */

#define STATIC_CLASS     wxT("STATIC")
#define STATIC_FLAGS     (SS_LEFT|WS_CHILD|WS_VISIBLE)
#define CHECK_CLASS      wxT("BUTTON")
#define CHECK_FLAGS      (BS_AUTOCHECKBOX|WS_TABSTOP|WS_CHILD)
#define CHECK_IS_FAFA    FALSE
#define RADIO_CLASS      wxT("BUTTON")
#define RADIO_FLAGS      (BS_AUTORADIOBUTTON|WS_CHILD|WS_VISIBLE)
#define RADIO_SIZE       20
#define RADIO_IS_FAFA    FALSE
#define PURE_WINDOWS
#define GROUP_CLASS      wxT("BUTTON")
#define GROUP_FLAGS      (BS_GROUPBOX|WS_CHILD|WS_VISIBLE)

/*
#define BITCHECK_FLAGS   (FB_BITMAP|FC_BUTTONDRAW|FC_DEFAULT|WS_VISIBLE)
#define BITRADIO_FLAGS   (FC_BUTTONDRAW|FB_BITMAP|FC_RADIO|WS_CHILD|WS_VISIBLE)
*/

// ---------------------------------------------------------------------------
// misc macros
// ---------------------------------------------------------------------------

#define MEANING_CHARACTER '0'
#define DEFAULT_ITEM_WIDTH  100
#define DEFAULT_ITEM_HEIGHT 80

// Scale font to get edit control height
//#define EDIT_HEIGHT_FROM_CHAR_HEIGHT(cy)    (3*(cy)/2)
#define EDIT_HEIGHT_FROM_CHAR_HEIGHT(cy)    (cy+8)

// Generic subclass proc, for panel item moving/sizing and intercept
// EDIT control VK_RETURN messages
extern LONG APIENTRY _EXPORT
  wxSubclassedGenericControlProc(WXHWND hWnd, WXUINT message, WXWPARAM wParam, WXLPARAM lParam);

// ---------------------------------------------------------------------------
// useful macros and functions
// ---------------------------------------------------------------------------

// a wrapper macro for ZeroMemory()
#if defined(__WIN32__) && !defined(__WXMICROWIN__)
#define wxZeroMemory(obj)   ::ZeroMemory(&obj, sizeof(obj))
#else
#define wxZeroMemory(obj)   memset((void*) & obj, 0, sizeof(obj))
#endif

// This one is a macro so that it can be tested with #ifdef, it will be
// undefined if it cannot be implemented for a given compiler.
// Vc++, bcc, dmc, ow, mingw, codewarrior (and rsxnt) have _get_osfhandle.
// Cygwin has get_osfhandle. Others are currently unknown, e.g. Salford,
// Intel, Visual Age.
#if defined(__WXWINCE__)
    #define wxGetOSFHandle(fd) ((HANDLE)fd)
    #define wxOpenOSFHandle(h, flags) ((int)wxPtrToUInt(h))
#elif defined(__CYGWIN__)
    #define wxGetOSFHandle(fd) ((HANDLE)get_osfhandle(fd))
#elif defined(__VISUALC__) \
   || defined(__BORLANDC__) \
   || defined(__DMC__) \
   || defined(__WATCOMC__) \
   || defined(__MINGW32__) \
   || (defined(__MWERKS__) && defined(__MSL__))
    #define wxGetOSFHandle(fd) ((HANDLE)_get_osfhandle(fd))
    #define wxOpenOSFHandle(h, flags) (_open_osfhandle(wxPtrToUInt(h), flags))
    #define wx_fdopen _fdopen
#endif

// close the handle in the class dtor
class AutoHANDLE
{
public:
    wxEXPLICIT AutoHANDLE(HANDLE handle) : m_handle(handle) { }

    bool IsOk() const { return m_handle != INVALID_HANDLE_VALUE; }
    operator HANDLE() const { return m_handle; }

    ~AutoHANDLE() { if ( IsOk() ) ::CloseHandle(m_handle); }

protected:
    HANDLE m_handle;
};

// a template to make initializing Windows styructs less painful: it zeroes all
// the struct fields and also sets cbSize member to the correct value (and so
// can be only used with structures which have this member...)
template <class T>
struct WinStruct : public T
{
    WinStruct()
    {
        ::ZeroMemory(this, sizeof(T));

        // explicit qualification is required here for this to be valid C++
        this->cbSize = sizeof(T);
    }
};


#if wxUSE_GUI

#include "wx/gdicmn.h"
#include "wx/colour.h"

// make conversion from wxColour and COLORREF a bit less painful
inline COLORREF wxColourToRGB(const wxColour& c)
{
    return RGB(c.Red(), c.Green(), c.Blue());
}

inline COLORREF wxColourToPalRGB(const wxColour& c)
{
    return PALETTERGB(c.Red(), c.Green(), c.Blue());
}

inline wxColour wxRGBToColour(COLORREF rgb)
{
    return wxColour(GetRValue(rgb), GetGValue(rgb), GetBValue(rgb));
}

inline void wxRGBToColour(wxColour& c, COLORREF rgb)
{
    c.Set(GetRValue(rgb), GetGValue(rgb), GetBValue(rgb));
}

// get the standard colour map for some standard colours - see comment in this
// function to understand why is it needed and when should it be used
//
// it returns a wxCOLORMAP (can't use COLORMAP itself here as comctl32.dll
// might be not included/available) array of size wxSTD_COLOUR_MAX
//
// NB: if you change these colours, update wxBITMAP_STD_COLOURS in the
//     resources as well: it must have the same number of pixels!
enum wxSTD_COLOUR
{
    wxSTD_COL_BTNTEXT,
    wxSTD_COL_BTNSHADOW,
    wxSTD_COL_BTNFACE,
    wxSTD_COL_BTNHIGHLIGHT,
    wxSTD_COL_MAX
};

struct WXDLLEXPORT wxCOLORMAP
{
    COLORREF from, to;
};

// this function is implemented in src/msw/window.cpp
extern wxCOLORMAP *wxGetStdColourMap();

// create a wxRect from Windows RECT
inline wxRect wxRectFromRECT(const RECT& rc)
{
    return wxRect(rc.left, rc.top, rc.right - rc.left, rc.bottom - rc.top);
}

// copy Windows RECT to our wxRect
inline void wxCopyRECTToRect(const RECT& rc, wxRect& rect)
{
    rect = wxRectFromRECT(rc);
}

// and vice versa
inline void wxCopyRectToRECT(const wxRect& rect, RECT& rc)
{
    // note that we don't use wxRect::GetRight() as it is one of compared to
    // wxRectFromRECT() above
    rc.top = rect.y;
    rc.left = rect.x;
    rc.right = rect.x + rect.width;
    rc.bottom = rect.y + rect.height;
}

// translations between HIMETRIC units (which OLE likes) and pixels (which are
// liked by all the others) - implemented in msw/utilsexc.cpp
extern void HIMETRICToPixel(LONG *x, LONG *y);
extern void PixelToHIMETRIC(LONG *x, LONG *y);

// Windows convention of the mask is opposed to the wxWidgets one, so we need
// to invert the mask each time we pass one/get one to/from Windows
extern HBITMAP wxInvertMask(HBITMAP hbmpMask, int w = 0, int h = 0);

// Creates an icon or cursor depending from a bitmap
//
// The bitmap must be valid and it should have a mask. If it doesn't, a default
// mask is created using light grey as the transparent colour.
extern HICON wxBitmapToHICON(const wxBitmap& bmp);

// Same requirments as above apply and the bitmap must also have the correct
// size.
extern
HCURSOR wxBitmapToHCURSOR(const wxBitmap& bmp, int hotSpotX, int hotSpotY);

// get (x, y) from DWORD - notice that HI/LOWORD can *not* be used because they
// will fail on system with multiple monitors where the coords may be negative
//
// these macros are standard now (Win98) but some older headers don't have them
#ifndef GET_X_LPARAM
    #define GET_X_LPARAM(lp) ((int)(short)LOWORD(lp))
    #define GET_Y_LPARAM(lp) ((int)(short)HIWORD(lp))
#endif // GET_X_LPARAM

// get the current state of SHIFT/CTRL keys
inline bool wxIsShiftDown()
{
//    return (::GetKeyState(VK_SHIFT) & 0x100) != 0;
    // Returns different negative values on WinME and WinNT,
    // so simply test for negative value.
    return ::GetKeyState(VK_SHIFT) < 0;
}

inline bool wxIsCtrlDown()
{
//    return (::GetKeyState(VK_CONTROL) & 0x100) != 0;
    // Returns different negative values on WinME and WinNT,
    // so simply test for negative value.
    return ::GetKeyState(VK_CONTROL) < 0;
}

// wrapper around GetWindowRect() and GetClientRect() APIs doing error checking
// for Win32
inline RECT wxGetWindowRect(HWND hwnd)
{
    RECT rect;

    if ( !::GetWindowRect(hwnd, &rect) )
        wxLogLastError(_T("GetWindowRect"));

    return rect;
}

inline RECT wxGetClientRect(HWND hwnd)
{
    RECT rect;

    if ( !::GetClientRect(hwnd, &rect) )
        wxLogLastError(_T("GetClientRect"));

    return rect;
}

// ---------------------------------------------------------------------------
// small helper classes
// ---------------------------------------------------------------------------

// create an instance of this class and use it as the HDC for screen, will
// automatically release the DC going out of scope
class ScreenHDC
{
public:
    ScreenHDC() { m_hdc = ::GetDC(NULL);    }
   ~ScreenHDC() { ::ReleaseDC(NULL, m_hdc); }

    operator HDC() const { return m_hdc; }

private:
    HDC m_hdc;

    DECLARE_NO_COPY_CLASS(ScreenHDC)
};

// the same as ScreenHDC but for window DCs
class WindowHDC
{
public:
    WindowHDC(HWND hwnd) { m_hdc = ::GetDC(m_hwnd = hwnd); }
   ~WindowHDC() { ::ReleaseDC(m_hwnd, m_hdc); }

    operator HDC() const { return m_hdc; }

private:
   HWND m_hwnd;
   HDC m_hdc;

   DECLARE_NO_COPY_CLASS(WindowHDC)
};

// the same as ScreenHDC but for memory DCs: creates the HDC compatible with
// the given one (screen by default) in ctor and destroys it in dtor
class MemoryHDC
{
public:
    MemoryHDC(HDC hdc = 0) { m_hdc = ::CreateCompatibleDC(hdc); }
   ~MemoryHDC() { ::DeleteDC(m_hdc); }

    operator HDC() const { return m_hdc; }

private:
    HDC m_hdc;

    DECLARE_NO_COPY_CLASS(MemoryHDC)
};

// a class which selects a GDI object into a DC in its ctor and deselects in
// dtor
class SelectInHDC
{
private:
    void DoInit(HGDIOBJ hgdiobj) { m_hgdiobj = ::SelectObject(m_hdc, hgdiobj); }

public:
    SelectInHDC() : m_hdc(NULL) { }
    SelectInHDC(HDC hdc, HGDIOBJ hgdiobj) : m_hdc(hdc) { DoInit(hgdiobj); }

    void Init(HDC hdc, HGDIOBJ hgdiobj)
    {
        wxASSERT_MSG( !m_hdc, _T("initializing twice?") );

        m_hdc = hdc;

        DoInit(hgdiobj);
    }

    ~SelectInHDC() { if ( m_hdc ) ::SelectObject(m_hdc, m_hgdiobj); }

    // return true if the object was successfully selected
    operator bool() const { return m_hgdiobj != 0; }

private:
    HDC m_hdc;
    HGDIOBJ m_hgdiobj;

    DECLARE_NO_COPY_CLASS(SelectInHDC)
};

// a class which cleans up any GDI object
class AutoGDIObject
{
protected:
    AutoGDIObject() { m_gdiobj = NULL; }
    AutoGDIObject(HGDIOBJ gdiobj) : m_gdiobj(gdiobj) { }
    ~AutoGDIObject() { if ( m_gdiobj ) ::DeleteObject(m_gdiobj); }

    void InitGdiobj(HGDIOBJ gdiobj)
    {
        wxASSERT_MSG( !m_gdiobj, _T("initializing twice?") );

        m_gdiobj = gdiobj;
    }

    HGDIOBJ GetObject() const { return m_gdiobj; }

private:
    HGDIOBJ m_gdiobj;
};

// TODO: all this asks for using a AutoHandler<T, CreateFunc> template...

// a class for temporary brushes
class AutoHBRUSH : private AutoGDIObject
{
public:
    AutoHBRUSH(COLORREF col)
        : AutoGDIObject(::CreateSolidBrush(col)) { }

    operator HBRUSH() const { return (HBRUSH)GetObject(); }
};

// a class for temporary fonts
class AutoHFONT : private AutoGDIObject
{
private:
public:
    AutoHFONT()
        : AutoGDIObject() { }

    AutoHFONT(const LOGFONT& lf)
        : AutoGDIObject(::CreateFontIndirect(&lf)) { }

    void Init(const LOGFONT& lf) { InitGdiobj(::CreateFontIndirect(&lf)); }

    operator HFONT() const { return (HFONT)GetObject(); }
};

// a class for temporary pens
class AutoHPEN : private AutoGDIObject
{
public:
    AutoHPEN(COLORREF col)
        : AutoGDIObject(::CreatePen(PS_SOLID, 0, col)) { }

    operator HPEN() const { return (HPEN)GetObject(); }
};

// classes for temporary bitmaps
class AutoHBITMAP : private AutoGDIObject
{
public:
    AutoHBITMAP(HBITMAP hbmp) : AutoGDIObject(hbmp) { }

    operator HBITMAP() const { return (HBITMAP)GetObject(); }
};

class CompatibleBitmap : public AutoHBITMAP
{
public:
    CompatibleBitmap(HDC hdc, int w, int h)
        : AutoHBITMAP(::CreateCompatibleBitmap(hdc, w, h))
    {
    }
};

class MonoBitmap : public AutoHBITMAP
{
public:
    MonoBitmap(int w, int h)
        : AutoHBITMAP(::CreateBitmap(w, h, 1, 1, 0))
    {
    }
};

// class automatically destroys the region object
class AutoHRGN : private AutoGDIObject
{
public:
    AutoHRGN(HRGN hrgn) : AutoGDIObject(hrgn) { }

    operator HRGN() const { return (HRGN)GetObject(); }
};

// class sets the specified clipping region during its life time
class HDCClipper
{
public:
    HDCClipper(HDC hdc, HRGN hrgn)
        : m_hdc(hdc)
    {
        if ( !::SelectClipRgn(hdc, hrgn) )
            wxLogLastError(_T("SelectClipRgn"));
    }

    ~HDCClipper()
    {
        ::SelectClipRgn(m_hdc, NULL);
    }

private:
    HDC m_hdc;

    DECLARE_NO_COPY_CLASS(HDCClipper)
};

// set the given map mode for the life time of this object
//
// NB: SetMapMode() is not supported by CE so we also define a helper macro
//     to avoid using it there
#ifdef __WXWINCE__
    #define wxCHANGE_HDC_MAP_MODE(hdc, mm)
#else // !__WXWINCE__
    class HDCMapModeChanger
    {
    public:
        HDCMapModeChanger(HDC hdc, int mm)
            : m_hdc(hdc)
        {
            m_modeOld = ::SetMapMode(hdc, mm);
            if ( !m_modeOld )
                wxLogLastError(_T("SelectClipRgn"));
        }

        ~HDCMapModeChanger()
        {
            if ( m_modeOld )
                ::SetMapMode(m_hdc, m_modeOld);
        }

    private:
        HDC m_hdc;
        int m_modeOld;

        DECLARE_NO_COPY_CLASS(HDCMapModeChanger)
    };

    #define wxCHANGE_HDC_MAP_MODE(hdc, mm) \
        HDCMapModeChanger wxMAKE_UNIQUE_NAME(wxHDCMapModeChanger)(hdc, mm)
#endif // __WXWINCE__/!__WXWINCE__

// smart buffeer using GlobalAlloc/GlobalFree()
class GlobalPtr
{
public:
    // allocates a block of given size
    GlobalPtr(size_t size, unsigned flags = GMEM_MOVEABLE)
    {
        m_hGlobal = ::GlobalAlloc(flags, size);
        if ( !m_hGlobal )
            wxLogLastError(_T("GlobalAlloc"));
    }

    ~GlobalPtr()
    {
        if ( m_hGlobal && ::GlobalFree(m_hGlobal) )
            wxLogLastError(_T("GlobalFree"));
    }

    // implicit conversion
    operator HGLOBAL() const { return m_hGlobal; }

private:
    HGLOBAL m_hGlobal;

    DECLARE_NO_COPY_CLASS(GlobalPtr)
};

// when working with global pointers (which is unfortunately still necessary
// sometimes, e.g. for clipboard) it is important to unlock them exactly as
// many times as we lock them which just asks for using a "smart lock" class
class GlobalPtrLock
{
public:
    GlobalPtrLock(HGLOBAL hGlobal) : m_hGlobal(hGlobal)
    {
        m_ptr = GlobalLock(hGlobal);
        if ( !m_ptr )
            wxLogLastError(_T("GlobalLock"));
    }

    ~GlobalPtrLock()
    {
        if ( !GlobalUnlock(m_hGlobal) )
        {
#ifdef __WXDEBUG__
            // this might happen simply because the block became unlocked
            DWORD dwLastError = ::GetLastError();
            if ( dwLastError != NO_ERROR )
            {
                wxLogApiError(_T("GlobalUnlock"), dwLastError);
            }
#endif // __WXDEBUG__
        }
    }

    operator void *() const { return m_ptr; }

private:
    HGLOBAL m_hGlobal;
    void *m_ptr;

    DECLARE_NO_COPY_CLASS(GlobalPtrLock)
};

// register the class when it is first needed and unregister it in dtor
class ClassRegistrar
{
public:
    // ctor doesn't register the class, call Initialize() for this
    ClassRegistrar() { m_registered = -1; }

    // return true if the class is already registered
    bool IsInitialized() const { return m_registered != -1; }

    // return true if the class had been already registered
    bool IsRegistered() const { return m_registered == 1; }

    // try to register the class if not done yet, return true on success
    bool Register(const WNDCLASS& wc)
    {
        // we should only be called if we hadn't been initialized yet
        wxASSERT_MSG( m_registered == -1,
                        _T("calling ClassRegistrar::Register() twice?") );

        m_registered = ::RegisterClass(&wc) ? 1 : 0;
        if ( !IsRegistered() )
        {
            wxLogLastError(_T("RegisterClassEx()"));
        }
        else
        {
            m_clsname = wc.lpszClassName;
        }

        return m_registered == 1;
    }

    // get the name of the registered class (returns empty string if not
    // registered)
    const wxString& GetName() const { return m_clsname; }

    // unregister the class if it had been registered
    ~ClassRegistrar()
    {
        if ( IsRegistered() )
        {
            if ( !::UnregisterClass(m_clsname, wxhInstance) )
            {
                wxLogLastError(_T("UnregisterClass"));
            }
        }
    }

private:
    // initial value is -1 which means that we hadn't tried registering the
    // class yet, it becomes true or false (1 or 0) when Initialize() is called
    int m_registered;

    // the name of the class, only non empty if it had been registered
    wxString m_clsname;
};

// ---------------------------------------------------------------------------
// macros to make casting between WXFOO and FOO a bit easier: the GetFoo()
// returns Foo cast to the Windows type for oruselves, while GetFooOf() takes
// an argument which should be a pointer or reference to the object of the
// corresponding class (this depends on the macro)
// ---------------------------------------------------------------------------

#define GetHwnd()               ((HWND)GetHWND())
#define GetHwndOf(win)          ((HWND)((win)->GetHWND()))
// old name
#define GetWinHwnd              GetHwndOf

#define GetHdc()                ((HDC)GetHDC())
#define GetHdcOf(dc)            ((HDC)(dc).GetHDC())

#define GetHbitmap()            ((HBITMAP)GetHBITMAP())
#define GetHbitmapOf(bmp)       ((HBITMAP)(bmp).GetHBITMAP())

#define GetHicon()              ((HICON)GetHICON())
#define GetHiconOf(icon)        ((HICON)(icon).GetHICON())

#define GetHaccel()             ((HACCEL)GetHACCEL())
#define GetHaccelOf(table)      ((HACCEL)((table).GetHACCEL()))

#define GetHbrush()             ((HBRUSH)GetResourceHandle())
#define GetHbrushOf(brush)      ((HBRUSH)(brush).GetResourceHandle())

#define GetHmenu()              ((HMENU)GetHMenu())
#define GetHmenuOf(menu)        ((HMENU)menu->GetHMenu())

#define GetHcursor()            ((HCURSOR)GetHCURSOR())
#define GetHcursorOf(cursor)    ((HCURSOR)(cursor).GetHCURSOR())

#define GetHfont()              ((HFONT)GetHFONT())
#define GetHfontOf(font)        ((HFONT)(font).GetHFONT())

#define GetHimagelist()         ((HIMAGELIST)GetHIMAGELIST())
#define GetHimagelistOf(imgl)   ((HIMAGELIST)imgl->GetHIMAGELIST())

#define GetHpalette()           ((HPALETTE)GetHPALETTE())
#define GetHpaletteOf(pal)      ((HPALETTE)(pal).GetHPALETTE())

#define GetHpen()               ((HPEN)GetResourceHandle())
#define GetHpenOf(pen)          ((HPEN)(pen).GetResourceHandle())

#define GetHrgn()               ((HRGN)GetHRGN())
#define GetHrgnOf(rgn)          ((HRGN)(rgn).GetHRGN())

#endif // wxUSE_GUI

// ---------------------------------------------------------------------------
// global functions
// ---------------------------------------------------------------------------

extern "C"
{
    WXDLLIMPEXP_BASE HINSTANCE wxGetInstance();
}

WXDLLIMPEXP_BASE void wxSetInstance(HINSTANCE hInst);

// return the full path of the given module
inline wxString wxGetFullModuleName(HMODULE hmod)
{
    wxString fullname;
    if ( !::GetModuleFileName
            (
                hmod,
                wxStringBuffer(fullname, MAX_PATH),
                MAX_PATH
            ) )
    {
        wxLogLastError(_T("GetModuleFileName"));
    }

    return fullname;
}

// return the full path of the program file
inline wxString wxGetFullModuleName()
{
    return wxGetFullModuleName((HMODULE)wxGetInstance());
}

// return the run-time version of the OS in a format similar to
// WINVER/_WIN32_WINNT compile-time macros:
//
//      0x0300      Windows NT 3.51
//      0x0400      Windows 95, NT4
//      0x0410      Windows 98
//      0x0500      Windows ME, 2000
//      0x0501      Windows XP
//      0x0502      Windows 2003
//      0x0600      Longhorn
//
// for the other Windows versions 0 is currently returned
enum wxWinVersion
{
    wxWinVersion_Unknown = 0,

    wxWinVersion_3 = 0x0300,
    wxWinVersion_NT3 = wxWinVersion_3,

    wxWinVersion_4 = 0x0400,
    wxWinVersion_95 = wxWinVersion_4,
    wxWinVersion_NT4 = wxWinVersion_4,
    wxWinVersion_98 = 0x0410,

    wxWinVersion_5 = 0x0500,
    wxWinVersion_ME = wxWinVersion_5,
    wxWinVersion_NT5 = wxWinVersion_5,
    wxWinVersion_2000 = wxWinVersion_5,
    wxWinVersion_XP = 0x0501,
    wxWinVersion_2003 = 0x0502,

    wxWinVersion_6 = 0x0600,
    wxWinVersion_NT6 = 0x0600
};

WXDLLIMPEXP_BASE wxWinVersion wxGetWinVersion();

#if wxUSE_GUI

// cursor stuff
extern HCURSOR wxGetCurrentBusyCursor();    // from msw/utils.cpp
extern const wxCursor *wxGetGlobalCursor(); // from msw/cursor.cpp

WXDLLEXPORT void wxGetCharSize(WXHWND wnd, int *x, int *y, const wxFont& the_font);
WXDLLEXPORT void wxFillLogFont(LOGFONT *logFont, const wxFont *font);
WXDLLEXPORT wxFont wxCreateFontFromLogFont(const LOGFONT *logFont);
WXDLLEXPORT wxFontEncoding wxGetFontEncFromCharSet(int charset);

WXDLLEXPORT void wxSliderEvent(WXHWND control, WXWORD wParam, WXWORD pos);
WXDLLEXPORT void wxScrollBarEvent(WXHWND hbar, WXWORD wParam, WXWORD pos);

// Find maximum size of window/rectangle
extern WXDLLEXPORT void wxFindMaxSize(WXHWND hwnd, RECT *rect);

// Safely get the window text (i.e. without using fixed size buffer)
extern WXDLLEXPORT wxString wxGetWindowText(WXHWND hWnd);

// get the window class name
extern WXDLLEXPORT wxString wxGetWindowClass(WXHWND hWnd);

// get the window id (should be unsigned, hence this is not wxWindowID which
// is, for mainly historical reasons, signed)
extern WXDLLEXPORT WXWORD wxGetWindowId(WXHWND hWnd);

// check if hWnd's WNDPROC is wndProc. Return true if yes, false if they are
// different
extern WXDLLEXPORT bool wxCheckWindowWndProc(WXHWND hWnd, WXFARPROC wndProc);

// Does this window style specify any border?
inline bool wxStyleHasBorder(long style)
{
    return (style & (wxSIMPLE_BORDER | wxRAISED_BORDER |
                     wxSUNKEN_BORDER | wxDOUBLE_BORDER)) != 0;
}

// ----------------------------------------------------------------------------
// functions mapping HWND to wxWindow
// ----------------------------------------------------------------------------

// this function simply checks whether the given hWnd corresponds to a wxWindow
// and returns either that window if it does or NULL otherwise
extern WXDLLEXPORT wxWindow* wxFindWinFromHandle(WXHWND hWnd);

// find the window for HWND which is part of some wxWindow, i.e. unlike
// wxFindWinFromHandle() above it will also work for "sub controls" of a
// wxWindow.
//
// returns the wxWindow corresponding to the given HWND or NULL.
extern WXDLLEXPORT wxWindow *wxGetWindowFromHWND(WXHWND hwnd);

// Get the size of an icon
extern WXDLLEXPORT wxSize wxGetHiconSize(HICON hicon);

// Lines are drawn differently for WinCE and regular WIN32
WXDLLEXPORT void wxDrawLine(HDC hdc, int x1, int y1, int x2, int y2);

// fill the client rect of the given window on the provided dc using this brush
inline void wxFillRect(HWND hwnd, HDC hdc, HBRUSH hbr)
{
    RECT rc;
    ::GetClientRect(hwnd, &rc);
    ::FillRect(hdc, &rc, hbr);
}

// ----------------------------------------------------------------------------
// 32/64 bit helpers
// ----------------------------------------------------------------------------

#ifdef __WIN64__

inline void *wxGetWindowProc(HWND hwnd)
{
    return (void *)::GetWindowLongPtr(hwnd, GWLP_WNDPROC);
}

inline void *wxGetWindowUserData(HWND hwnd)
{
    return (void *)::GetWindowLongPtr(hwnd, GWLP_USERDATA);
}

inline WNDPROC wxSetWindowProc(HWND hwnd, WNDPROC func)
{
    return (WNDPROC)::SetWindowLongPtr(hwnd, GWLP_WNDPROC, (LONG_PTR)func);
}

inline void *wxSetWindowUserData(HWND hwnd, void *data)
{
    return (void *)::SetWindowLongPtr(hwnd, GWLP_USERDATA, (LONG_PTR)data);
}

#else // __WIN32__

// note that the casts to LONG_PTR here are required even on 32-bit machines
// for the 64-bit warning mode of later versions of MSVC (C4311/4312)
inline WNDPROC wxGetWindowProc(HWND hwnd)
{
    return (WNDPROC)(LONG_PTR)::GetWindowLong(hwnd, GWL_WNDPROC);
}

inline void *wxGetWindowUserData(HWND hwnd)
{
    return (void *)(LONG_PTR)::GetWindowLong(hwnd, GWL_USERDATA);
}

inline WNDPROC wxSetWindowProc(HWND hwnd, WNDPROC func)
{
    return (WNDPROC)(LONG_PTR)::SetWindowLong(hwnd, GWL_WNDPROC, (LONG_PTR)func);
}

inline void *wxSetWindowUserData(HWND hwnd, void *data)
{
    return (void *)(LONG_PTR)::SetWindowLong(hwnd, GWL_USERDATA, (LONG_PTR)data);
}

#endif // __WIN64__/__WIN32__

#endif // wxUSE_GUI

#endif // _WX_PRIVATE_H_
