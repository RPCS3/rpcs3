/////////////////////////////////////////////////////////////////////////////
// Name:        wx/generic/colour.h
// Purpose:     wxColour class
// Author:      Julian Smart
// Modified by:
// Created:     01/02/97
// RCS-ID:      $Id: colour.h 41751 2006-10-08 21:56:55Z VZ $
// Copyright:   (c) Julian Smart
// Licence:     wxWindows licence
/////////////////////////////////////////////////////////////////////////////

#ifndef _WX_GENERIC_COLOUR_H_
#define _WX_GENERIC_COLOUR_H_

#include "wx/object.h"

// Colour
class WXDLLEXPORT wxColour: public wxColourBase
{
public:
    // constructors
    // ------------

    // default
    wxColour();
    DEFINE_STD_WXCOLOUR_CONSTRUCTORS

    // copy ctors and assignment operators
    wxColour(const wxColour& col);
    wxColour& operator=(const wxColour& col);

    // dtor
    virtual ~wxColour();

    // accessors
    bool Ok() const { return IsOk(); }
    bool IsOk() const { return m_isInit; }

    unsigned char Red() const { return m_red; }
    unsigned char Green() const { return m_green; }
    unsigned char Blue() const { return m_blue; }
    unsigned char Alpha() const { return m_alpha; }

    // comparison
    bool operator==(const wxColour& colour) const
    {
        return (m_red == colour.m_red &&
                m_green == colour.m_green &&
                m_blue == colour.m_blue &&
                m_alpha == colour.m_alpha &&
                m_isInit == colour.m_isInit);
    }

    bool operator!=(const wxColour& colour) const { return !(*this == colour); }

protected:

    // Helper function
    void Init();

    virtual void
    InitRGBA(unsigned char r, unsigned char g, unsigned char b, unsigned char a);

private:
    bool m_isInit;
    unsigned char m_red;
    unsigned char m_blue;
    unsigned char m_green;
    unsigned char m_alpha;

private:
    DECLARE_DYNAMIC_CLASS(wxColour)
};

#endif // _WX_GENERIC_COLOUR_H_
