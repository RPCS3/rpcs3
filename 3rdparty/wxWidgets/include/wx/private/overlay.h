/////////////////////////////////////////////////////////////////////////////
// Name:        wx/private/overlay.h
// Purpose:     wxOverlayImpl declaration
// Author:      Stefan Csomor
// Modified by:
// Created:     2006-10-20
// RCS-ID:      $Id: overlay.h 53135 2008-04-12 02:31:04Z VZ $
// Copyright:   (c) wxWidgets team
// Licence:     wxWindows licence
/////////////////////////////////////////////////////////////////////////////

#ifndef _WX_PRIVATE_OVERLAY_H_
#define _WX_PRIVATE_OVERLAY_H_

#include "wx/overlay.h"

#ifdef wxHAS_NATIVE_OVERLAY

#if defined(__WXMAC__)
    #include "wx/mac/carbon/private/overlay.h"
#elif defined(__WXDFB__)
    #include "wx/dfb/private/overlay.h"
#else
    #error "unknown native wxOverlay implementation"
#endif

#else // !wxHAS_NATIVE_OVERLAY

#include "wx/bitmap.h"

class WXDLLIMPEXP_FWD_CORE wxWindow;

// generic implementation of wxOverlay
class wxOverlayImpl
{
public:
    wxOverlayImpl();
    ~wxOverlayImpl();


    // clears the overlay without restoring the former state
    // to be done eg when the window content has been changed and repainted
    void Reset();

    // returns true if it has been setup
    bool IsOk();

    void Init(wxWindowDC* dc, int x , int y , int width , int height);

    void BeginDrawing(wxWindowDC* dc);

    void EndDrawing(wxWindowDC* dc);

    void Clear(wxWindowDC* dc);

private:
    wxBitmap m_bmpSaved ;
    int m_x ;
    int m_y ;
    int m_width ;
    int m_height ;
// this is to enable wxMOTIF and UNIV to compile....
// currently (10 oct 06) we don't use m_window
// ce - how do we fix this
#if defined(__WXGTK__) || defined(__WXMSW__)
    wxWindow* m_window ;
#endif
} ;

#endif // wxHAS_NATIVE_OVERLAY/!wxHAS_NATIVE_OVERLAY

#endif // _WX_PRIVATE_OVERLAY_H_
