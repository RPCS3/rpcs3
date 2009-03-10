///////////////////////////////////////////////////////////////////////////////
// Name:        src/common/dpycmn.cpp
// Purpose:     wxDisplay and wxDisplayImplSingle implementation
// Author:      Vadim Zeitlin
// Modified by:
// Created:     01.03.03
// RCS-ID:      $Id: dpycmn.cpp 41548 2006-10-02 05:38:05Z PC $
// Copyright:   (c) 2003-2006 Vadim Zeitlin <vadim@wxwindows.org>
// License:     wxWindows licence
///////////////////////////////////////////////////////////////////////////////

// ============================================================================
// declarations
// ============================================================================

// ----------------------------------------------------------------------------
// headers
// ----------------------------------------------------------------------------

// For compilers that support precompilation, includes "wx.h".
#include "wx/wxprec.h"

#ifdef __BORLANDC__
    #pragma hdrstop
#endif

#ifndef WX_PRECOMP
    #include "wx/gdicmn.h"
    #include "wx/window.h"
    #include "wx/module.h"
#endif //WX_PRECOMP

#include "wx/display.h"
#include "wx/display_impl.h"

#if wxUSE_DISPLAY

#include "wx/arrimpl.cpp"
WX_DEFINE_OBJARRAY(wxArrayVideoModes)

const wxVideoMode wxDefaultVideoMode;

#endif // wxUSE_DISPLAY

// ----------------------------------------------------------------------------
// globals
// ----------------------------------------------------------------------------

// the factory object used by wxDisplay
//
// created on demand and destroyed by wxDisplayModule
static wxDisplayFactory *gs_factory = NULL;

// ----------------------------------------------------------------------------
// wxDisplayImplSingle: trivial implementation working for main display only
// ----------------------------------------------------------------------------

class WXDLLEXPORT wxDisplayImplSingle : public wxDisplayImpl
{
public:
    wxDisplayImplSingle() : wxDisplayImpl(0) { }

    virtual wxRect GetGeometry() const
    {
        wxRect r;
        wxDisplaySize(&r.width, &r.height);
        return r;
    }

    virtual wxRect GetClientArea() const { return wxGetClientDisplayRect(); }

    virtual wxString GetName() const { return wxString(); }

#if wxUSE_DISPLAY
    // no video modes support for us, provide just the stubs

    virtual wxArrayVideoModes GetModes(const wxVideoMode& WXUNUSED(mode)) const
    {
        return wxArrayVideoModes();
    }

    virtual wxVideoMode GetCurrentMode() const { return wxVideoMode(); }

    virtual bool ChangeMode(const wxVideoMode& WXUNUSED(mode)) { return false; }
#endif // wxUSE_DISPLAY


    DECLARE_NO_COPY_CLASS(wxDisplayImplSingle)
};

// ----------------------------------------------------------------------------
// wxDisplayModule is used to cleanup gs_factory
// ----------------------------------------------------------------------------

class wxDisplayModule : public wxModule
{
public:
    virtual bool OnInit() { return true; }
    virtual void OnExit()
    {
        if ( gs_factory )
        {
            delete gs_factory;
            gs_factory = NULL;
        }
    }

    DECLARE_DYNAMIC_CLASS(wxDisplayModule)
};

IMPLEMENT_DYNAMIC_CLASS(wxDisplayModule, wxModule)

// ============================================================================
// wxDisplay implementation
// ============================================================================

// ----------------------------------------------------------------------------
// ctor/dtor
// ----------------------------------------------------------------------------

wxDisplay::wxDisplay(unsigned n)
{
    wxASSERT_MSG( n < GetCount(),
                    wxT("An invalid index was passed to wxDisplay") );

    m_impl = Factory().CreateDisplay(n);
}

wxDisplay::~wxDisplay()
{
    delete m_impl;
}

// ----------------------------------------------------------------------------
// static functions forwarded to wxDisplayFactory
// ----------------------------------------------------------------------------

/* static */ unsigned wxDisplay::GetCount()
{
    return Factory().GetCount();
}

/* static */ int wxDisplay::GetFromPoint(const wxPoint& pt)
{
    return Factory().GetFromPoint(pt);
}

/* static */ int wxDisplay::GetFromWindow(wxWindow *window)
{
    wxCHECK_MSG( window, wxNOT_FOUND, _T("invalid window") );

    return Factory().GetFromWindow(window);
}

// ----------------------------------------------------------------------------
// functions forwarded to wxDisplayImpl
// ----------------------------------------------------------------------------

wxRect wxDisplay::GetGeometry() const
{
    wxCHECK_MSG( IsOk(), wxRect(), _T("invalid wxDisplay object") );

    return m_impl->GetGeometry();
}

wxRect wxDisplay::GetClientArea() const
{
    wxCHECK_MSG( IsOk(), wxRect(), _T("invalid wxDisplay object") );

    return m_impl->GetClientArea();
}

wxString wxDisplay::GetName() const
{
    wxCHECK_MSG( IsOk(), wxString(), _T("invalid wxDisplay object") );

    return m_impl->GetName();
}

bool wxDisplay::IsPrimary() const
{
    return m_impl && m_impl->GetIndex() == 0;
}

#if wxUSE_DISPLAY

wxArrayVideoModes wxDisplay::GetModes(const wxVideoMode& mode) const
{
    wxCHECK_MSG( IsOk(), wxArrayVideoModes(), _T("invalid wxDisplay object") );

    return m_impl->GetModes(mode);
}

wxVideoMode wxDisplay::GetCurrentMode() const
{
    wxCHECK_MSG( IsOk(), wxVideoMode(), _T("invalid wxDisplay object") );

    return m_impl->GetCurrentMode();
}

bool wxDisplay::ChangeMode(const wxVideoMode& mode)
{
    wxCHECK_MSG( IsOk(), false, _T("invalid wxDisplay object") );

    return m_impl->ChangeMode(mode);
}

#endif // wxUSE_DIRECTDRAW

// ----------------------------------------------------------------------------
// static functions implementation
// ----------------------------------------------------------------------------

// if wxUSE_DISPLAY == 1 this is implemented in port-specific code
#if !wxUSE_DISPLAY

/* static */ wxDisplayFactory *wxDisplay::CreateFactory()
{
    return new wxDisplayFactorySingle;
}

#endif // !wxUSE_DISPLAY

/* static */ wxDisplayFactory& wxDisplay::Factory()
{
    if ( !gs_factory )
    {
        gs_factory = CreateFactory();
    }

    return *gs_factory;
}

// ============================================================================
// wxDisplayFactory implementation
// ============================================================================

int wxDisplayFactory::GetFromWindow(wxWindow *window)
{
    // consider that the window belongs to the display containing its centre
    const wxRect r(window->GetRect());
    return GetFromPoint(wxPoint(r.x + r.width/2, r.y + r.height/2));
}

// ============================================================================
// wxDisplayFactorySingle implementation
// ============================================================================

/* static */
wxDisplayImpl *wxDisplayFactorySingle::CreateDisplay(unsigned n)
{
    // we recognize the main display only
    return n != 0 ? NULL : new wxDisplayImplSingle;
}

int wxDisplayFactorySingle::GetFromPoint(const wxPoint& pt)
{
    if ( pt.x >= 0 && pt.y >= 0 )
    {
        int w, h;
        wxDisplaySize(&w, &h);

        if ( pt.x < w && pt.y < h )
            return 0;
    }

    // the point is outside of the screen
    return wxNOT_FOUND;
}
