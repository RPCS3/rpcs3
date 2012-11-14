/////////////////////////////////////////////////////////////////////////////
// Name:        src/msw/display.cpp
// Purpose:     MSW Implementation of wxDisplay class
// Author:      Royce Mitchell III, Vadim Zeitlin
// Modified by: Ryan Norton (IsPrimary override)
// Created:     06/21/02
// RCS-ID:      $Id: display.cpp 56865 2008-11-20 17:46:46Z VZ $
// Copyright:   (c) wxWidgets team
// Copyright:   (c) 2002-2006 wxWidgets team
// Licence:     wxWindows licence
/////////////////////////////////////////////////////////////////////////////

// ===========================================================================
// declarations
// ===========================================================================

// ---------------------------------------------------------------------------
// headers
// ---------------------------------------------------------------------------

// For compilers that support precompilation, includes "wx.h".
#include "wx/wxprec.h"

#ifdef __BORLANDC__
    #pragma hdrstop
#endif

#if wxUSE_DISPLAY

#include "wx/display.h"

#ifndef WX_PRECOMP
    #include "wx/dynarray.h"
    #include "wx/app.h"
    #include "wx/frame.h"
#endif

#include "wx/dynload.h"
#include "wx/sysopt.h"

#include "wx/display_impl.h"
#include "wx/msw/wrapwin.h"
#include "wx/msw/missing.h"

// define this to use DirectDraw for display mode switching: this is disabled
// by default because ddraw.h is now always available and also it's not really
// clear what are the benefits of using DirectDraw compared to the standard API

#if !defined(wxUSE_DIRECTDRAW)
    #define wxUSE_DIRECTDRAW 0
#endif

#ifndef __WXWINCE__
    // Older versions of windef.h don't define HMONITOR.  Unfortunately, we
    // can't directly test whether HMONITOR is defined or not in windef.h as
    // it's not a macro but a typedef, so we test for an unrelated symbol which
    // is only defined in winuser.h if WINVER >= 0x0500
    #if !defined(HMONITOR_DECLARED) && !defined(MNS_NOCHECK)
        DECLARE_HANDLE(HMONITOR);
        typedef BOOL(CALLBACK * MONITORENUMPROC )(HMONITOR, HDC, LPRECT, LPARAM);
        typedef struct tagMONITORINFO
        {
            DWORD   cbSize;
            RECT    rcMonitor;
            RECT    rcWork;
            DWORD   dwFlags;
        } MONITORINFO, *LPMONITORINFO;
        typedef struct tagMONITORINFOEX : public tagMONITORINFO
        {
            TCHAR       szDevice[CCHDEVICENAME];
        } MONITORINFOEX, *LPMONITORINFOEX;
        #define MONITOR_DEFAULTTONULL       0x00000000
        #define MONITOR_DEFAULTTOPRIMARY    0x00000001
        #define MONITOR_DEFAULTTONEAREST    0x00000002
        #define MONITORINFOF_PRIMARY        0x00000001
        #define HMONITOR_DECLARED
    #endif
#endif // !__WXWINCE__

#if wxUSE_DIRECTDRAW
    #include <ddraw.h>

    // we don't want to link with ddraw.lib which contains the real
    // IID_IDirectDraw2 definition
    const GUID wxIID_IDirectDraw2 =
     { 0xB3A6F3E0, 0x2B43, 0x11CF, { 0xA2,0xDE,0x00,0xAA,0x00,0xB9,0x33,0x56 } };
#endif // wxUSE_DIRECTDRAW

// display functions are found in different DLLs under WinCE and normal Win32
#ifdef __WXWINCE__
static const wxChar displayDllName[] = _T("coredll.dll");
#else
static const wxChar displayDllName[] = _T("user32.dll");
#endif

// ----------------------------------------------------------------------------
// typedefs for dynamically loaded Windows functions
// ----------------------------------------------------------------------------

typedef LONG (WINAPI *ChangeDisplaySettingsEx_t)(LPCTSTR lpszDeviceName,
                                                 LPDEVMODE lpDevMode,
                                                 HWND hwnd,
                                                 DWORD dwFlags,
                                                 LPVOID lParam);

#if wxUSE_DIRECTDRAW
typedef BOOL (PASCAL *DDEnumExCallback_t)(GUID *pGuid,
                                          LPTSTR driverDescription,
                                          LPTSTR driverName,
                                          LPVOID lpContext,
                                          HMONITOR hmon);

typedef HRESULT (WINAPI *DirectDrawEnumerateEx_t)(DDEnumExCallback_t lpCallback,
                                                  LPVOID lpContext,
                                                  DWORD dwFlags);

typedef HRESULT (WINAPI *DirectDrawCreate_t)(GUID *lpGUID,
                                             LPDIRECTDRAW *lplpDD,
                                             IUnknown *pUnkOuter);
#endif // wxUSE_DIRECTDRAW

typedef BOOL (WINAPI *EnumDisplayMonitors_t)(HDC,LPCRECT,MONITORENUMPROC,LPARAM);
typedef HMONITOR (WINAPI *MonitorFromPoint_t)(POINT,DWORD);
typedef HMONITOR (WINAPI *MonitorFromWindow_t)(HWND,DWORD);
typedef BOOL (WINAPI *GetMonitorInfo_t)(HMONITOR,LPMONITORINFO);

#ifndef __WXWINCE__
// emulation of ChangeDisplaySettingsEx() for Win95
LONG WINAPI ChangeDisplaySettingsExForWin95(LPCTSTR WXUNUSED(lpszDeviceName),
                                            LPDEVMODE lpDevMode,
                                            HWND WXUNUSED(hwnd),
                                            DWORD dwFlags,
                                            LPVOID WXUNUSED(lParam))
{
    return ::ChangeDisplaySettings(lpDevMode, dwFlags);
}
#endif // !__WXWINCE__

// ----------------------------------------------------------------------------
// display information classes
// ----------------------------------------------------------------------------

struct wxDisplayInfo
{
    wxDisplayInfo(HMONITOR hmon = NULL)
    {
        m_hmon = hmon;
        m_flags = (DWORD)-1;
    }

    virtual ~wxDisplayInfo() { }


    // use GetMonitorInfo() to fill in all of our fields if needed (i.e. if it
    // hadn't been done before)
    void Initialize();


    // handle of this monitor used by MonitorXXX() functions, never NULL
    HMONITOR m_hmon;

    // the entire area of this monitor in virtual screen coordinates
    wxRect m_rect;

    // the work or client area, i.e. the area available for the normal windows
    wxRect m_rectClient;

    // the display device name for this monitor, empty initially and retrieved
    // on demand by DoGetName()
    wxString m_devName;

    // the flags of this monitor, also used as initialization marker: if this
    // is -1, GetMonitorInfo() hadn't been called yet
    DWORD m_flags;
};

WX_DEFINE_ARRAY_PTR(wxDisplayInfo *, wxDisplayInfoArray);

// ----------------------------------------------------------------------------
// common base class for all Win32 wxDisplayImpl versions
// ----------------------------------------------------------------------------

class wxDisplayImplWin32Base : public wxDisplayImpl
{
public:
    wxDisplayImplWin32Base(unsigned n, wxDisplayInfo& info)
        : wxDisplayImpl(n),
          m_info(info)
    {
    }

    virtual wxRect GetGeometry() const;
    virtual wxRect GetClientArea() const;
    virtual wxString GetName() const;
    virtual bool IsPrimary() const;

    virtual wxVideoMode GetCurrentMode() const;

protected:
    // convert a DEVMODE to our wxVideoMode
    static wxVideoMode ConvertToVideoMode(const DEVMODE& dm)
    {
        // note that dmDisplayFrequency may be 0 or 1 meaning "standard one"
        // and although 0 is ok for us we don't want to return modes with 1hz
        // refresh
        return wxVideoMode(dm.dmPelsWidth,
                           dm.dmPelsHeight,
                           dm.dmBitsPerPel,
                           dm.dmDisplayFrequency > 1 ? dm.dmDisplayFrequency : 0);
    }

    wxDisplayInfo& m_info;
};

// ----------------------------------------------------------------------------
// common base class for all Win32 wxDisplayFactory versions
// ----------------------------------------------------------------------------

// functions dynamically bound by wxDisplayFactoryWin32Base::Initialize()
static MonitorFromPoint_t gs_MonitorFromPoint = NULL;
static MonitorFromWindow_t gs_MonitorFromWindow = NULL;
static GetMonitorInfo_t gs_GetMonitorInfo = NULL;

class wxDisplayFactoryWin32Base : public wxDisplayFactory
{
public:
    virtual ~wxDisplayFactoryWin32Base();

    bool IsOk() const { return !m_displays.empty(); }

    virtual unsigned GetCount() { return unsigned(m_displays.size()); }
    virtual int GetFromPoint(const wxPoint& pt);
    virtual int GetFromWindow(wxWindow *window);

protected:
    // ctor checks if the current system supports multimon API and dynamically
    // bind the functions we need if this is the case and sets
    // ms_supportsMultimon if they're available
    wxDisplayFactoryWin32Base();

    // delete all m_displays elements: can be called from the derived class
    // dtor if it is important to do this before destroying it (as in
    // wxDisplayFactoryDirectDraw case), otherwise will be done by our dtor
    void Clear();

    // find the monitor corresponding to the given handle, return wxNOT_FOUND
    // if not found
    int FindDisplayFromHMONITOR(HMONITOR hmon) const;


    // flag indicating whether gs_MonitorXXX functions are available
    static int ms_supportsMultimon;

    // the array containing information about all available displays, should be
    // filled by the derived class ctors
    wxDisplayInfoArray m_displays;


    DECLARE_NO_COPY_CLASS(wxDisplayFactoryWin32Base)
};

// ----------------------------------------------------------------------------
// wxDisplay implementation using Windows multi-monitor support functions
// ----------------------------------------------------------------------------

class wxDisplayImplMultimon : public wxDisplayImplWin32Base
{
public:
    wxDisplayImplMultimon(unsigned n, wxDisplayInfo& info)
        : wxDisplayImplWin32Base(n, info)
    {
    }

    virtual wxArrayVideoModes GetModes(const wxVideoMode& mode) const;
    virtual bool ChangeMode(const wxVideoMode& mode);

private:
    DECLARE_NO_COPY_CLASS(wxDisplayImplMultimon)
};

class wxDisplayFactoryMultimon : public wxDisplayFactoryWin32Base
{
public:
    wxDisplayFactoryMultimon();

    virtual wxDisplayImpl *CreateDisplay(unsigned n);

private:
    // EnumDisplayMonitors() callback
    static BOOL CALLBACK MultimonEnumProc(HMONITOR hMonitor,
                                          HDC hdcMonitor,
                                          LPRECT lprcMonitor,
                                          LPARAM dwData);


    // add a monitor description to m_displays array
    void AddDisplay(HMONITOR hMonitor, LPRECT lprcMonitor);
};

// ----------------------------------------------------------------------------
// wxDisplay implementation using DirectDraw
// ----------------------------------------------------------------------------

#if wxUSE_DIRECTDRAW

struct wxDisplayInfoDirectDraw : wxDisplayInfo
{
    wxDisplayInfoDirectDraw(const GUID& guid, HMONITOR hmon, LPTSTR name)
        : wxDisplayInfo(hmon),
          m_guid(guid)
    {
        m_pDD2 = NULL;
        m_devName = name;
    }

    virtual ~wxDisplayInfoDirectDraw()
    {
        if ( m_pDD2 )
            m_pDD2->Release();
    }


    // IDirectDraw object used to control this display, may be NULL
    IDirectDraw2 *m_pDD2;

    // DirectDraw GUID for this display, only valid when using DirectDraw
    const GUID m_guid;


    DECLARE_NO_COPY_CLASS(wxDisplayInfoDirectDraw)
};

class wxDisplayImplDirectDraw : public wxDisplayImplWin32Base
{
public:
    wxDisplayImplDirectDraw(unsigned n, wxDisplayInfo& info, IDirectDraw2 *pDD2)
        : wxDisplayImplWin32Base(n, info),
          m_pDD2(pDD2)
    {
        m_pDD2->AddRef();
    }

    virtual ~wxDisplayImplDirectDraw()
    {
        m_pDD2->Release();
    }

    virtual wxArrayVideoModes GetModes(const wxVideoMode& mode) const;
    virtual bool ChangeMode(const wxVideoMode& mode);

private:
    IDirectDraw2 *m_pDD2;

    DECLARE_NO_COPY_CLASS(wxDisplayImplDirectDraw)
};

class wxDisplayFactoryDirectDraw : public wxDisplayFactoryWin32Base
{
public:
    wxDisplayFactoryDirectDraw();
    virtual ~wxDisplayFactoryDirectDraw();

    virtual wxDisplayImpl *CreateDisplay(unsigned n);

private:
    // callback used with DirectDrawEnumerateEx()
    static BOOL WINAPI DDEnumExCallback(GUID *pGuid,
                                        LPTSTR driverDescription,
                                        LPTSTR driverName,
                                        LPVOID lpContext,
                                        HMONITOR hmon);

    // add a monitor description to m_displays array
    void AddDisplay(const GUID& guid, HMONITOR hmon, LPTSTR name);


    // ddraw.dll
    wxDynamicLibrary m_dllDDraw;

    // dynamically resolved DirectDrawCreate()
    DirectDrawCreate_t m_pfnDirectDrawCreate;

    DECLARE_NO_COPY_CLASS(wxDisplayFactoryDirectDraw)
};

#endif // wxUSE_DIRECTDRAW


// ============================================================================
// common classes implementation
// ============================================================================

// ----------------------------------------------------------------------------
// wxDisplay
// ----------------------------------------------------------------------------

/* static */ wxDisplayFactory *wxDisplay::CreateFactory()
{
    // we have 2 implementations for modern Windows: one using standard Win32
    // and another using DirectDraw, the choice between them is done using a
    // system option

#if wxUSE_DIRECTDRAW
    if ( wxSystemOptions::GetOptionInt(_T("msw.display.directdraw")) )
    {
        wxDisplayFactoryDirectDraw *factoryDD = new wxDisplayFactoryDirectDraw;
        if ( factoryDD->IsOk() )
            return factoryDD;

        delete factoryDD;
    }
#endif // wxUSE_DIRECTDRAW

    wxDisplayFactoryMultimon *factoryMM = new wxDisplayFactoryMultimon;
    if ( factoryMM->IsOk() )
        return factoryMM;

    delete factoryMM;


    // finally fall back to a stub implementation if all else failed (Win95?)
    return new wxDisplayFactorySingle;
}

// ----------------------------------------------------------------------------
// wxDisplayInfo
// ----------------------------------------------------------------------------

void wxDisplayInfo::Initialize()
{
    if ( m_flags == (DWORD)-1 )
    {
        WinStruct<MONITORINFOEX> monInfo;
        if ( !gs_GetMonitorInfo(m_hmon, (LPMONITORINFO)&monInfo) )
        {
            wxLogLastError(_T("GetMonitorInfo"));
            m_flags = 0;
            return;
        }

        wxCopyRECTToRect(monInfo.rcMonitor, m_rect);
        wxCopyRECTToRect(monInfo.rcWork, m_rectClient);
        m_devName = monInfo.szDevice;
        m_flags = monInfo.dwFlags;
    }
}

// ----------------------------------------------------------------------------
// wxDisplayImplWin32Base
// ----------------------------------------------------------------------------

wxRect wxDisplayImplWin32Base::GetGeometry() const
{
    if ( m_info.m_rect.IsEmpty() )
        m_info.Initialize();

    return m_info.m_rect;
}

wxRect wxDisplayImplWin32Base::GetClientArea() const
{
    if ( m_info.m_rectClient.IsEmpty() )
        m_info.Initialize();

    return m_info.m_rectClient;
}

wxString wxDisplayImplWin32Base::GetName() const
{
    if ( m_info.m_devName.empty() )
        m_info.Initialize();

    return m_info.m_devName;
}

bool wxDisplayImplWin32Base::IsPrimary() const
{
    if ( m_info.m_flags == (DWORD)-1 )
        m_info.Initialize();

    return (m_info.m_flags & MONITORINFOF_PRIMARY) != 0;
}

wxVideoMode wxDisplayImplWin32Base::GetCurrentMode() const
{
    wxVideoMode mode;

    // The first parameter of EnumDisplaySettings() must be NULL under Win95
    // according to MSDN.  The version of GetName() we implement for Win95
    // returns an empty string.
    const wxString name = GetName();
    const wxChar * const deviceName = name.empty() ? NULL : name.c_str();

    DEVMODE dm;
    dm.dmSize = sizeof(dm);
    dm.dmDriverExtra = 0;
    if ( !::EnumDisplaySettings(deviceName, ENUM_CURRENT_SETTINGS, &dm) )
    {
        wxLogLastError(_T("EnumDisplaySettings(ENUM_CURRENT_SETTINGS)"));
    }
    else
    {
        mode = ConvertToVideoMode(dm);
    }

    return mode;
}

// ----------------------------------------------------------------------------
// wxDisplayFactoryWin32Base
// ----------------------------------------------------------------------------

int wxDisplayFactoryWin32Base::ms_supportsMultimon = -1;

wxDisplayFactoryWin32Base::wxDisplayFactoryWin32Base()
{
    if ( ms_supportsMultimon == -1 )
    {
        ms_supportsMultimon = 0;

        wxLogNull noLog;

        wxDynamicLibrary dllDisplay(displayDllName, wxDL_VERBATIM);

        gs_MonitorFromPoint = (MonitorFromPoint_t)
            dllDisplay.GetSymbol(wxT("MonitorFromPoint"));
        if ( !gs_MonitorFromPoint )
            return;

        gs_MonitorFromWindow = (MonitorFromWindow_t)
            dllDisplay.GetSymbol(wxT("MonitorFromWindow"));
        if ( !gs_MonitorFromWindow )
            return;

        gs_GetMonitorInfo = (GetMonitorInfo_t)
            dllDisplay.GetSymbolAorW(wxT("GetMonitorInfo"));
        if ( !gs_GetMonitorInfo )
            return;

        ms_supportsMultimon = 1;

        // we can safely let dllDisplay go out of scope, the DLL itself will
        // still remain loaded as all Win32 programs use it
    }
}

void wxDisplayFactoryWin32Base::Clear()
{
    WX_CLEAR_ARRAY(m_displays);
}

wxDisplayFactoryWin32Base::~wxDisplayFactoryWin32Base()
{
    Clear();
}

// helper for GetFromPoint() and GetFromWindow()
int wxDisplayFactoryWin32Base::FindDisplayFromHMONITOR(HMONITOR hmon) const
{
    if ( hmon )
    {
        const size_t count = m_displays.size();
        for ( size_t n = 0; n < count; n++ )
        {
            if ( hmon == m_displays[n]->m_hmon )
                return n;
        }
    }

    return wxNOT_FOUND;
}

int wxDisplayFactoryWin32Base::GetFromPoint(const wxPoint& pt)
{
    POINT pt2;
    pt2.x = pt.x;
    pt2.y = pt.y;

    return FindDisplayFromHMONITOR(gs_MonitorFromPoint(pt2,
                                                       MONITOR_DEFAULTTONULL));
}

int wxDisplayFactoryWin32Base::GetFromWindow(wxWindow *window)
{
    return FindDisplayFromHMONITOR(gs_MonitorFromWindow(GetHwndOf(window),
                                                        MONITOR_DEFAULTTONULL));
}

// ============================================================================
// wxDisplay implementation using Win32 multimon API
// ============================================================================

// ----------------------------------------------------------------------------
// wxDisplayFactoryMultimon initialization
// ----------------------------------------------------------------------------

wxDisplayFactoryMultimon::wxDisplayFactoryMultimon()
{
    if ( !ms_supportsMultimon )
        return;

    // look up EnumDisplayMonitors() which we don't need with DirectDraw
    // implementation
    EnumDisplayMonitors_t pfnEnumDisplayMonitors;
    {
        wxLogNull noLog;

        wxDynamicLibrary dllDisplay(displayDllName, wxDL_VERBATIM);
        pfnEnumDisplayMonitors = (EnumDisplayMonitors_t)
            dllDisplay.GetSymbol(wxT("EnumDisplayMonitors"));
        if ( !pfnEnumDisplayMonitors )
            return;
    }

    // enumerate all displays
    if ( !pfnEnumDisplayMonitors(NULL, NULL, MultimonEnumProc, (LPARAM)this) )
    {
        wxLogLastError(wxT("EnumDisplayMonitors"));
    }
}

/* static */
BOOL CALLBACK
wxDisplayFactoryMultimon::MultimonEnumProc(
  HMONITOR hMonitor,        // handle to display monitor
  HDC WXUNUSED(hdcMonitor), // handle to monitor-appropriate device context
  LPRECT lprcMonitor,       // pointer to monitor intersection rectangle
  LPARAM dwData             // data passed from EnumDisplayMonitors (this)
)
{
    wxDisplayFactoryMultimon *const self = (wxDisplayFactoryMultimon *)dwData;
    self->AddDisplay(hMonitor, lprcMonitor);

    // continue the enumeration
    return TRUE;
}

// ----------------------------------------------------------------------------
// wxDisplayFactoryMultimon helper functions
// ----------------------------------------------------------------------------

void wxDisplayFactoryMultimon::AddDisplay(HMONITOR hMonitor, LPRECT lprcMonitor)
{
    wxDisplayInfo *info = new wxDisplayInfo(hMonitor);

    // we also store the display geometry
    info->m_rect = wxRect(lprcMonitor->left, lprcMonitor->top,
                          lprcMonitor->right - lprcMonitor->left,
                          lprcMonitor->bottom - lprcMonitor->top);

    // now add this monitor to the array
    m_displays.Add(info);
}

// ----------------------------------------------------------------------------
// wxDisplayFactoryMultimon inherited pure virtuals implementation
// ----------------------------------------------------------------------------

wxDisplayImpl *wxDisplayFactoryMultimon::CreateDisplay(unsigned n)
{
    wxCHECK_MSG( n < m_displays.size(), NULL, _T("invalid display index") );

    return new wxDisplayImplMultimon(n, *(m_displays[n]));
}

// ----------------------------------------------------------------------------
// wxDisplayImplMultimon implementation
// ----------------------------------------------------------------------------

wxArrayVideoModes
wxDisplayImplMultimon::GetModes(const wxVideoMode& modeMatch) const
{
    wxArrayVideoModes modes;

    // The first parameter of EnumDisplaySettings() must be NULL under Win95
    // according to MSDN.  The version of GetName() we implement for Win95
    // returns an empty string.
    const wxString name = GetName();
    const wxChar * const deviceName = name.empty() ? NULL : name.c_str();

    DEVMODE dm;
    dm.dmSize = sizeof(dm);
    dm.dmDriverExtra = 0;
    for ( int iModeNum = 0;
          ::EnumDisplaySettings(deviceName, iModeNum, &dm);
          iModeNum++ )
    {
        const wxVideoMode mode = ConvertToVideoMode(dm);
        if ( mode.Matches(modeMatch) )
        {
            modes.Add(mode);
        }
    }

    return modes;
}

bool wxDisplayImplMultimon::ChangeMode(const wxVideoMode& mode)
{
    // prepare ChangeDisplaySettingsEx() parameters
    DEVMODE dm;
    DEVMODE *pDevMode;

    int flags;

    if ( mode == wxDefaultVideoMode )
    {
        // reset the video mode to default
        pDevMode = NULL;
        flags = 0;
    }
    else // change to the given mode
    {
        wxCHECK_MSG( mode.w && mode.h, false,
                        _T("at least the width and height must be specified") );

        wxZeroMemory(dm);
        dm.dmSize = sizeof(dm);
        dm.dmDriverExtra = 0;
        dm.dmFields = DM_PELSWIDTH | DM_PELSHEIGHT;
        dm.dmPelsWidth = mode.w;
        dm.dmPelsHeight = mode.h;

        if ( mode.bpp )
        {
            dm.dmFields |= DM_BITSPERPEL;
            dm.dmBitsPerPel = mode.bpp;
        }

        if ( mode.refresh )
        {
            dm.dmFields |= DM_DISPLAYFREQUENCY;
            dm.dmDisplayFrequency = mode.refresh;
        }

        pDevMode = &dm;

#ifdef __WXWINCE__
        flags = 0;
#else // !__WXWINCE__
        flags = CDS_FULLSCREEN;
#endif // __WXWINCE__/!__WXWINCE__
    }


    // get pointer to the function dynamically
    //
    // we're only called from the main thread, so it's ok to use static
    // variable
    static ChangeDisplaySettingsEx_t pfnChangeDisplaySettingsEx = NULL;
    if ( !pfnChangeDisplaySettingsEx )
    {
        wxDynamicLibrary dllDisplay(displayDllName, wxDL_VERBATIM);
        if ( dllDisplay.IsLoaded() )
        {
            pfnChangeDisplaySettingsEx = (ChangeDisplaySettingsEx_t)
                dllDisplay.GetSymbolAorW(_T("ChangeDisplaySettingsEx"));
        }
        //else: huh, no user32.dll??

#ifndef __WXWINCE__
        if ( !pfnChangeDisplaySettingsEx )
        {
            // we must be under Win95 and so there is no multiple monitors
            // support anyhow
            pfnChangeDisplaySettingsEx = ChangeDisplaySettingsExForWin95;
        }
#endif // !__WXWINCE__
    }

    // do change the mode
    switch ( pfnChangeDisplaySettingsEx
             (
                GetName(),      // display name
                pDevMode,       // dev mode or NULL to reset
                NULL,           // reserved
                flags,
                NULL            // pointer to video parameters (not used)
             ) )
    {
        case DISP_CHANGE_SUCCESSFUL:
            // ok
            {
                // If we have a top-level, full-screen frame, emulate
                // the DirectX behavior and resize it.  This makes this
                // API quite a bit easier to use.
                wxWindow *winTop = wxTheApp->GetTopWindow();
                wxFrame *frameTop = wxDynamicCast(winTop, wxFrame);
                if (frameTop && frameTop->IsFullScreen())
                {
                    wxVideoMode current = GetCurrentMode();
                    frameTop->SetClientSize(current.w, current.h);
                }
            }
            return true;

        case DISP_CHANGE_BADMODE:
            // don't complain about this, this is the only "expected" error
            break;

        default:
            wxFAIL_MSG( _T("unexpected ChangeDisplaySettingsEx() return value") );
    }

    return false;
}


// ============================================================================
// DirectDraw-based wxDisplay implementation
// ============================================================================

#if wxUSE_DIRECTDRAW

// ----------------------------------------------------------------------------
// wxDisplayFactoryDirectDraw initialization
// ----------------------------------------------------------------------------

wxDisplayFactoryDirectDraw::wxDisplayFactoryDirectDraw()
{
    if ( !ms_supportsMultimon )
        return;

#if wxUSE_LOG
    // suppress the errors if ddraw.dll is not found, we're prepared to handle
    // this
    wxLogNull noLog;
#endif

    m_dllDDraw.Load(_T("ddraw.dll"));

    if ( !m_dllDDraw.IsLoaded() )
        return;

    DirectDrawEnumerateEx_t pDDEnumEx = (DirectDrawEnumerateEx_t)
        m_dllDDraw.GetSymbolAorW(_T("DirectDrawEnumerateEx"));
    if ( !pDDEnumEx )
        return;

    // we can't continue without DirectDrawCreate() later, so resolve it right
    // now and fail the initialization if it's not available
    m_pfnDirectDrawCreate = (DirectDrawCreate_t)
        m_dllDDraw.GetSymbol(_T("DirectDrawCreate"));
    if ( !m_pfnDirectDrawCreate )
        return;

    if ( (*pDDEnumEx)(DDEnumExCallback,
                      this,
                      DDENUM_ATTACHEDSECONDARYDEVICES) != DD_OK )
    {
        wxLogLastError(_T("DirectDrawEnumerateEx"));
    }
}

wxDisplayFactoryDirectDraw::~wxDisplayFactoryDirectDraw()
{
    // we must clear m_displays now, before m_dllDDraw is unloaded as otherwise
    // calling m_pDD2->Release() later would crash
    Clear();
}

// ----------------------------------------------------------------------------
// callbacks for monitor/modes enumeration stuff
// ----------------------------------------------------------------------------

BOOL WINAPI
wxDisplayFactoryDirectDraw::DDEnumExCallback(GUID *pGuid,
                                             LPTSTR WXUNUSED(driverDescription),
                                             LPTSTR driverName,
                                             LPVOID lpContext,
                                             HMONITOR hmon)
{
    if ( pGuid )
    {
        wxDisplayFactoryDirectDraw * self =
            wx_static_cast(wxDisplayFactoryDirectDraw *, lpContext);
        self->AddDisplay(*pGuid, hmon, driverName);
    }
    //else: we're called for the primary monitor, skip it

    // continue the enumeration
    return TRUE;
}

// ----------------------------------------------------------------------------
// wxDisplayFactoryDirectDraw helpers
// ----------------------------------------------------------------------------

void wxDisplayFactoryDirectDraw::AddDisplay(const GUID& guid,
                                            HMONITOR hmon,
                                            LPTSTR name)
{
    m_displays.Add(new wxDisplayInfoDirectDraw(guid, hmon, name));
}

// ----------------------------------------------------------------------------
// wxDisplayFactoryDirectDraw inherited pure virtuals implementation
// ----------------------------------------------------------------------------

wxDisplayImpl *wxDisplayFactoryDirectDraw::CreateDisplay(unsigned n)
{
    wxCHECK_MSG( n < m_displays.size(), NULL, _T("invalid display index") );

    wxDisplayInfoDirectDraw *
        info = wx_static_cast(wxDisplayInfoDirectDraw *, m_displays[n]);

    if ( !info->m_pDD2 )
    {
        IDirectDraw *pDD;
        GUID guid(info->m_guid);
        HRESULT hr = (*m_pfnDirectDrawCreate)(&guid, &pDD, NULL);

        if ( FAILED(hr) || !pDD )
        {
            // what to do??
            wxLogApiError(_T("DirectDrawCreate"), hr);
            return NULL;
        }

        // we got IDirectDraw, but we need IDirectDraw2
        hr = pDD->QueryInterface(wxIID_IDirectDraw2, (void **)&info->m_pDD2);
        pDD->Release();

        if ( FAILED(hr) || !info->m_pDD2 )
        {
            wxLogApiError(_T("IDirectDraw::QueryInterface(IDD2)"), hr);
            return NULL;
        }

        // NB: m_pDD2 will now be only destroyed when m_displays is destroyed
        //     which is ok as we don't want to recreate DD objects all the time
    }
    //else: DirectDraw object corresponding to our display already exists

    return new wxDisplayImplDirectDraw(n, *info, info->m_pDD2);
}

// ============================================================================
// wxDisplayImplDirectDraw
// ============================================================================

// ----------------------------------------------------------------------------
// video modes enumeration
// ----------------------------------------------------------------------------

// tiny helper class used to pass information from GetModes() to
// wxDDEnumModesCallback
class wxDDVideoModesAdder
{
public:
    // our Add() method will add modes matching modeMatch to modes array
    wxDDVideoModesAdder(wxArrayVideoModes& modes, const wxVideoMode& modeMatch)
        : m_modes(modes),
          m_modeMatch(modeMatch)
    {
    }

    void Add(const wxVideoMode& mode)
    {
        if ( mode.Matches(m_modeMatch) )
            m_modes.Add(mode);
    }

private:
    wxArrayVideoModes& m_modes;
    const wxVideoMode& m_modeMatch;

    DECLARE_NO_COPY_CLASS(wxDDVideoModesAdder)
};

HRESULT WINAPI wxDDEnumModesCallback(LPDDSURFACEDESC lpDDSurfaceDesc,
                                     LPVOID lpContext)
{
    // we need at least the mode size
    static const DWORD FLAGS_REQUIRED = DDSD_HEIGHT | DDSD_WIDTH;
    if ( (lpDDSurfaceDesc->dwFlags & FLAGS_REQUIRED) == FLAGS_REQUIRED )
    {
        wxDDVideoModesAdder * const vmodes =
            wx_static_cast(wxDDVideoModesAdder *, lpContext);

        vmodes->Add(wxVideoMode(lpDDSurfaceDesc->dwWidth,
                                lpDDSurfaceDesc->dwHeight,
                                lpDDSurfaceDesc->ddpfPixelFormat.dwRGBBitCount,
                                lpDDSurfaceDesc->dwRefreshRate));
    }

    // continue the enumeration
    return DDENUMRET_OK;
}

wxArrayVideoModes
wxDisplayImplDirectDraw::GetModes(const wxVideoMode& modeMatch) const
{
    wxArrayVideoModes modes;
    wxDDVideoModesAdder modesAdder(modes, modeMatch);

    HRESULT hr = m_pDD2->EnumDisplayModes
                         (
                            DDEDM_REFRESHRATES,
                            NULL,                   // all modes
                            &modesAdder,            // callback parameter
                            wxDDEnumModesCallback
                         );

    if ( FAILED(hr) )
    {
        wxLogApiError(_T("IDirectDraw::EnumDisplayModes"), hr);
    }

    return modes;
}

// ----------------------------------------------------------------------------
// video mode switching
// ----------------------------------------------------------------------------

bool wxDisplayImplDirectDraw::ChangeMode(const wxVideoMode& mode)
{
    wxWindow *winTop = wxTheApp->GetTopWindow();
    wxCHECK_MSG( winTop, false, _T("top level window required for DirectX") );

    HRESULT hr = m_pDD2->SetCooperativeLevel
                         (
                            GetHwndOf(winTop),
                            DDSCL_EXCLUSIVE | DDSCL_FULLSCREEN
                         );
    if ( FAILED(hr) )
    {
        wxLogApiError(_T("IDirectDraw2::SetCooperativeLevel"), hr);

        return false;
    }

    hr = m_pDD2->SetDisplayMode(mode.w, mode.h, mode.bpp, mode.refresh, 0);
    if ( FAILED(hr) )
    {
        wxLogApiError(_T("IDirectDraw2::SetDisplayMode"), hr);

        return false;
    }

    return true;
}

#endif // wxUSE_DIRECTDRAW

#endif // wxUSE_DISPLAY
