/////////////////////////////////////////////////////////////////////////////
// Name:        wx/effects.h
// Purpose:     wxEffects class
//              Draws 3D effects.
// Author:      Julian Smart et al
// Modified by:
// Created:     25/4/2000
// RCS-ID:      $Id: effects.h 39109 2006-05-08 11:31:03Z ABX $
// Copyright:   (c) Julian Smart
// Licence:     wxWindows licence
/////////////////////////////////////////////////////////////////////////////

#ifndef _WX_EFFECTS_H_
#define _WX_EFFECTS_H_

/*
 * wxEffects: various 3D effects
 */

#include "wx/object.h"
#include "wx/colour.h"
#include "wx/gdicmn.h"
#include "wx/dc.h"

class WXDLLEXPORT wxEffects: public wxObject
{
DECLARE_CLASS(wxEffects)

public:
    // Assume system colours
    wxEffects() ;
    // Going from lightest to darkest
    wxEffects(const wxColour& highlightColour, const wxColour& lightShadow,
              const wxColour& faceColour, const wxColour& mediumShadow,
              const wxColour& darkShadow) ;

    // Accessors
    wxColour GetHighlightColour() const { return m_highlightColour; }
    wxColour GetLightShadow() const { return m_lightShadow; }
    wxColour GetFaceColour() const { return m_faceColour; }
    wxColour GetMediumShadow() const { return m_mediumShadow; }
    wxColour GetDarkShadow() const { return m_darkShadow; }

    void SetHighlightColour(const wxColour& c) { m_highlightColour = c; }
    void SetLightShadow(const wxColour& c) { m_lightShadow = c; }
    void SetFaceColour(const wxColour& c) { m_faceColour = c; }
    void SetMediumShadow(const wxColour& c) { m_mediumShadow = c; }
    void SetDarkShadow(const wxColour& c) { m_darkShadow = c; }

    void Set(const wxColour& highlightColour, const wxColour& lightShadow,
             const wxColour& faceColour, const wxColour& mediumShadow,
             const wxColour& darkShadow)
    {
        SetHighlightColour(highlightColour);
        SetLightShadow(lightShadow);
        SetFaceColour(faceColour);
        SetMediumShadow(mediumShadow);
        SetDarkShadow(darkShadow);
    }

    // Draw a sunken edge
    void DrawSunkenEdge(wxDC& dc, const wxRect& rect, int borderSize = 1);

    // Tile a bitmap
    bool TileBitmap(const wxRect& rect, wxDC& dc, const wxBitmap& bitmap);

protected:
    wxColour    m_highlightColour;  // Usually white
    wxColour    m_lightShadow;      // Usually light grey
    wxColour    m_faceColour;       // Usually grey
    wxColour    m_mediumShadow;     // Usually dark grey
    wxColour    m_darkShadow;       // Usually black
};

#endif
