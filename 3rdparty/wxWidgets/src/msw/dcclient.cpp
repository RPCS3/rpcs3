/////////////////////////////////////////////////////////////////////////////
// Name:        src/msw/dcclient.cpp
// Purpose:     wxClientDC class
// Author:      Julian Smart
// Modified by:
// Created:     01/02/97
// RCS-ID:      $Id: dcclient.cpp 39021 2006-05-04 07:57:04Z ABX $
// Copyright:   (c) Julian Smart
// Licence:     wxWindows licence
/////////////////////////////////////////////////////////////////////////////

// ===========================================================================
// declarations
// ===========================================================================

// ----------------------------------------------------------------------------
// headers
// ----------------------------------------------------------------------------

// For compilers that support precompilation, includes "wx.h".
#include "wx/wxprec.h"

#ifdef __BORLANDC__
    #pragma hdrstop
#endif

#include "wx/dcclient.h"

#ifndef WX_PRECOMP
    #include "wx/string.h"
    #include "wx/log.h"
    #include "wx/window.h"
#endif

#include "wx/msw/private.h"

// ----------------------------------------------------------------------------
// array/list types
// ----------------------------------------------------------------------------

struct WXDLLEXPORT wxPaintDCInfo
{
    wxPaintDCInfo(wxWindow *win, wxDC *dc)
    {
        hwnd = win->GetHWND();
        hdc = dc->GetHDC();
        count = 1;
    }

    WXHWND    hwnd;       // window for this DC
    WXHDC     hdc;        // the DC handle
    size_t    count;      // usage count
};

#include "wx/arrimpl.cpp"

WX_DEFINE_OBJARRAY(wxArrayDCInfo)

// ----------------------------------------------------------------------------
// macros
// ----------------------------------------------------------------------------

IMPLEMENT_DYNAMIC_CLASS(wxWindowDC, wxDC)
IMPLEMENT_DYNAMIC_CLASS(wxClientDC, wxWindowDC)
IMPLEMENT_DYNAMIC_CLASS(wxPaintDC, wxClientDC)
IMPLEMENT_CLASS(wxPaintDCEx, wxPaintDC)

// ----------------------------------------------------------------------------
// global variables
// ----------------------------------------------------------------------------

static PAINTSTRUCT g_paintStruct;

#ifdef __WXDEBUG__
    // a global variable which we check to verify that wxPaintDC are only
    // created in response to WM_PAINT message - doing this from elsewhere is a
    // common programming error among wxWidgets programmers and might lead to
    // very subtle and difficult to debug refresh/repaint bugs.
    int g_isPainting = 0;
#endif // __WXDEBUG__

// ===========================================================================
// implementation
// ===========================================================================

// ----------------------------------------------------------------------------
// wxWindowDC
// ----------------------------------------------------------------------------

wxWindowDC::wxWindowDC()
{
    m_canvas = NULL;
}

wxWindowDC::wxWindowDC(wxWindow *canvas)
{
    wxCHECK_RET( canvas, _T("invalid window in wxWindowDC") );

    m_canvas = canvas;
    m_hDC = (WXHDC) ::GetWindowDC(GetHwndOf(m_canvas));

    // m_bOwnsDC was already set to false in the base class ctor, so the DC
    // will be released (and not deleted) in ~wxDC
    InitDC();
}

void wxWindowDC::InitDC()
{
    // the background mode is only used for text background and is set in
    // DrawText() to OPAQUE as required, otherwise always TRANSPARENT,
    ::SetBkMode(GetHdc(), TRANSPARENT);

    // default bg colour is pne of the window
    SetBackground(wxBrush(m_canvas->GetBackgroundColour(), wxSOLID));

    // since we are a window dc we need to grab the palette from the window
#if wxUSE_PALETTE
    InitializePalette();
#endif
}

void wxWindowDC::DoGetSize(int *width, int *height) const
{
    wxCHECK_RET( m_canvas, _T("wxWindowDC without a window?") );

    m_canvas->GetSize(width, height);
}

// ----------------------------------------------------------------------------
// wxClientDC
// ----------------------------------------------------------------------------

wxClientDC::wxClientDC()
{
    m_canvas = NULL;
}

wxClientDC::wxClientDC(wxWindow *canvas)
{
    wxCHECK_RET( canvas, _T("invalid window in wxClientDC") );

    m_canvas = canvas;
    m_hDC = (WXHDC)::GetDC(GetHwndOf(m_canvas));

    // m_bOwnsDC was already set to false in the base class ctor, so the DC
    // will be released (and not deleted) in ~wxDC

    InitDC();
}

void wxClientDC::InitDC()
{
    wxWindowDC::InitDC();

    // in wxUniv build we must manually do some DC adjustments usually
    // performed by Windows for us
    //
    // we also need to take the menu/toolbar manually into account under
    // Windows CE because they're just another control there, not anything
    // special as usually under Windows
#if defined(__WXUNIVERSAL__) || defined(__WXWINCE__)
    wxPoint ptOrigin = m_canvas->GetClientAreaOrigin();
    if ( ptOrigin.x || ptOrigin.y )
    {
        // no need to shift DC origin if shift is null
        SetDeviceOrigin(ptOrigin.x, ptOrigin.y);
    }

    // clip the DC to avoid overwriting the non client area
    SetClippingRegion(wxPoint(0,0), m_canvas->GetClientSize());
#endif // __WXUNIVERSAL__ || __WXWINCE__
}

wxClientDC::~wxClientDC()
{
}

void wxClientDC::DoGetSize(int *width, int *height) const
{
    wxCHECK_RET( m_canvas, _T("wxClientDC without a window?") );

    m_canvas->GetClientSize(width, height);
}

// ----------------------------------------------------------------------------
// wxPaintDC
// ----------------------------------------------------------------------------

// VZ: initial implementation (by JACS) only remembered the last wxPaintDC
//     created and tried to reuse it - this was supposed to take care of a
//     situation when a derived class OnPaint() calls base class OnPaint()
//     because in this case ::BeginPaint() shouldn't be called second time.
//
//     I'm not sure how useful this is, however we must remember the HWND
//     associated with the last HDC as well - otherwise we may (and will!) try
//     to reuse the HDC for another HWND which is a nice recipe for disaster.
//
//     So we store a list of windows for which we already have the DC and not
//     just one single hDC. This seems to work, but I'm really not sure about
//     the usefullness of the whole idea - IMHO it's much better to not call
//     base class OnPaint() at all, or, if we really want to allow it, add a
//     "wxPaintDC *" parameter to wxPaintEvent which should be used if it's
//     !NULL instead of creating a new DC.

wxArrayDCInfo wxPaintDC::ms_cache;

wxPaintDC::wxPaintDC()
{
    m_canvas = NULL;
}

wxPaintDC::wxPaintDC(wxWindow *canvas)
{
    wxCHECK_RET( canvas, wxT("NULL canvas in wxPaintDC ctor") );

#ifdef __WXDEBUG__
    if ( g_isPainting <= 0 )
    {
        wxFAIL_MSG( wxT("wxPaintDC may be created only in EVT_PAINT handler!") );

        return;
    }
#endif // __WXDEBUG__

    m_canvas = canvas;

    // do we have a DC for this window in the cache?
    wxPaintDCInfo *info = FindInCache();
    if ( info )
    {
        m_hDC = info->hdc;
        info->count++;
    }
    else // not in cache, create a new one
    {
        m_hDC = (WXHDC)::BeginPaint(GetHwndOf(m_canvas), &g_paintStruct);
        if (m_hDC)
            ms_cache.Add(new wxPaintDCInfo(m_canvas, this));
    }

    // (re)set the DC parameters.
    // Note: at this point m_hDC can be NULL under MicroWindows, when dragging.
    if (GetHDC())
        InitDC();
}

wxPaintDC::~wxPaintDC()
{
    if ( m_hDC )
    {
        SelectOldObjects(m_hDC);

        size_t index;
        wxPaintDCInfo *info = FindInCache(&index);

        wxCHECK_RET( info, wxT("existing DC should have a cache entry") );

        if ( --info->count == 0 )
        {
            ::EndPaint(GetHwndOf(m_canvas), &g_paintStruct);

            ms_cache.RemoveAt(index);

            // Reduce the number of bogus reports of non-freed memory
            // at app exit
            if (ms_cache.IsEmpty())
                ms_cache.Clear();
        }
        //else: cached DC entry is still in use

        // prevent the base class dtor from ReleaseDC()ing it again
        m_hDC = 0;
    }
}

wxPaintDCInfo *wxPaintDC::FindInCache(size_t *index) const
{
    wxPaintDCInfo *info = NULL;
    size_t nCache = ms_cache.GetCount();
    for ( size_t n = 0; n < nCache; n++ )
    {
        wxPaintDCInfo *info1 = &ms_cache[n];
        if ( info1->hwnd == m_canvas->GetHWND() )
        {
            info = info1;
            if ( index )
                *index = n;
            break;
        }
    }

    return info;
}

// find the entry for this DC in the cache (keyed by the window)
WXHDC wxPaintDC::FindDCInCache(wxWindow* win)
{
    size_t nCache = ms_cache.GetCount();
    for ( size_t n = 0; n < nCache; n++ )
    {
        wxPaintDCInfo *info = &ms_cache[n];
        if ( info->hwnd == win->GetHWND() )
        {
            return info->hdc;
        }
    }
    return 0;
}

/*
 * wxPaintDCEx
 */

// TODO: don't duplicate wxPaintDC code here!!

wxPaintDCEx::wxPaintDCEx(wxWindow *canvas, WXHDC dc) : saveState(0)
{
    wxCHECK_RET( dc, wxT("wxPaintDCEx requires an existing device context") );

    m_canvas = canvas;

    wxPaintDCInfo *info = FindInCache();
    if ( info )
    {
        m_hDC = info->hdc;
        info->count++;
    }
    else // not in cache, create a new one
    {
        m_hDC = dc;
        ms_cache.Add(new wxPaintDCInfo(m_canvas, this));
        saveState = SaveDC((HDC) dc);
    }
}

wxPaintDCEx::~wxPaintDCEx()
{
    size_t index;
    wxPaintDCInfo *info = FindInCache(&index);

    wxCHECK_RET( info, wxT("existing DC should have a cache entry") );

    if ( --info->count == 0 )
    {
        RestoreDC((HDC) m_hDC, saveState);
        ms_cache.RemoveAt(index);

        // Reduce the number of bogus reports of non-freed memory
        // at app exit
        if (ms_cache.IsEmpty())
            ms_cache.Clear();
    }
    //else: cached DC entry is still in use

    // prevent the base class dtor from ReleaseDC()ing it again
    m_hDC = 0;
}
