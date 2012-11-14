/////////////////////////////////////////////////////////////////////////////
// Name:        wx/msw/colour.h
// Purpose:     wxColour class
// Author:      Julian Smart
// Modified by:
// Created:     01/02/97
// RCS-ID:      $Id: colour.h 51769 2008-02-13 22:36:43Z VZ $
// Copyright:   (c) Julian Smart
// Licence:     wxWindows licence
/////////////////////////////////////////////////////////////////////////////

#ifndef _WX_COLOUR_H_
#define _WX_COLOUR_H_

#include "wx/object.h"

// ----------------------------------------------------------------------------
// Colour
// ----------------------------------------------------------------------------

class WXDLLEXPORT wxColour : public wxColourBase
{
public:
    // constructors
    // ------------

    wxColour() { Init(); }
    wxColour( ChannelType red, ChannelType green, ChannelType blue,
              ChannelType alpha = wxALPHA_OPAQUE )
        { Set(red, green, blue, alpha); }
    wxColour( unsigned long colRGB ) { Set(colRGB); }
    wxColour(const wxString& colourName) { Init(); Set(colourName); }
    wxColour(const wxChar *colourName) { Init(); Set(colourName); }


    // dtor
    virtual ~wxColour();


    // accessors
    // ---------

    bool Ok() const { return IsOk(); }
    bool IsOk() const { return m_isInit; }

    unsigned char Red() const { return m_red; }
    unsigned char Green() const { return m_green; }
    unsigned char Blue() const { return m_blue; }
    unsigned char Alpha() const { return m_alpha ; }

    // comparison
    bool operator==(const wxColour& colour) const
    {
        return m_isInit == colour.m_isInit
            && m_red == colour.m_red
            && m_green == colour.m_green
            && m_blue == colour.m_blue
            && m_alpha == colour.m_alpha;
    }

    bool operator != (const wxColour& colour) const { return !(*this == colour); }

    WXCOLORREF GetPixel() const { return m_pixel; }


public:
    WXCOLORREF m_pixel;

protected:
    // Helper function
    void Init();

    virtual void
    InitRGBA(unsigned char r, unsigned char g, unsigned char b, unsigned char a);

private:
    bool          m_isInit;
    unsigned char m_red;
    unsigned char m_blue;
    unsigned char m_green;
    unsigned char m_alpha;

private:
    DECLARE_DYNAMIC_CLASS(wxColour)
};

#endif
        // _WX_COLOUR_H_
