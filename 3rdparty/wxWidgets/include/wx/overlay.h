/////////////////////////////////////////////////////////////////////////////
// Name:        wx/overlay.h
// Purpose:     wxOverlay class
// Author:      Stefan Csomor
// Modified by:
// Created:     2006-10-20
// RCS-ID:      $Id: overlay.h 53135 2008-04-12 02:31:04Z VZ $
// Copyright:   (c) wxWidgets team
// Licence:     wxWindows licence
/////////////////////////////////////////////////////////////////////////////

#ifndef _WX_OVERLAY_H_
#define _WX_OVERLAY_H_

#include "wx/defs.h"

#if defined(wxMAC_USE_CORE_GRAPHICS) && wxMAC_USE_CORE_GRAPHICS
    #define wxHAS_NATIVE_OVERLAY 1
#elif defined(__WXDFB__)
    #define wxHAS_NATIVE_OVERLAY 1
#else
    // don't define wxHAS_NATIVE_OVERLAY
#endif

// ----------------------------------------------------------------------------
// creates an overlay over an existing window, allowing for manipulations like
// rubberbanding etc. This API is not stable yet, not to be used outside wx
// internal code
// ----------------------------------------------------------------------------

class WXDLLIMPEXP_FWD_CORE wxOverlayImpl;
class WXDLLIMPEXP_FWD_CORE wxWindowDC;

class WXDLLEXPORT wxOverlay
{
public:
    wxOverlay();
    ~wxOverlay();

    // clears the overlay without restoring the former state
    // to be done eg when the window content has been changed and repainted
    void Reset();

    // returns (port-specific) implementation of the overlay
    wxOverlayImpl *GetImpl() { return m_impl; }

private:
    friend class WXDLLIMPEXP_FWD_CORE wxDCOverlay;

    // returns true if it has been setup
    bool IsOk();

    void Init(wxWindowDC* dc, int x , int y , int width , int height);

    void BeginDrawing(wxWindowDC* dc);

    void EndDrawing(wxWindowDC* dc);

    void Clear(wxWindowDC* dc);

    wxOverlayImpl* m_impl;

    bool m_inDrawing;


    DECLARE_NO_COPY_CLASS(wxOverlay)
};


class WXDLLEXPORT wxDCOverlay
{
public:
    // connects this overlay to the corresponding drawing dc, if the overlay is
    // not initialized yet this call will do so
    wxDCOverlay(wxOverlay &overlay, wxWindowDC *dc, int x , int y , int width , int height);

    // convenience wrapper that behaves the same using the entire area of the dc
    wxDCOverlay(wxOverlay &overlay, wxWindowDC *dc);

    // removes the connection between the overlay and the dc
    virtual ~wxDCOverlay();

    // clears the layer, restoring the state at the last init
    void Clear();

private:
    void Init(wxWindowDC *dc, int x , int y , int width , int height);

    wxOverlay& m_overlay;

    wxWindowDC* m_dc;


    DECLARE_NO_COPY_CLASS(wxDCOverlay)
};

#endif // _WX_OVERLAY_H_
