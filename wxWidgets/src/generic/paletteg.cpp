/////////////////////////////////////////////////////////////////////////////
// Name:        src/generic/paletteg.cpp
// Purpose:
// Author:      Robert Roebling
// Created:     01/02/97
// RCS-ID:      $Id: paletteg.cpp 42752 2006-10-30 19:26:48Z VZ $
// Copyright:   (c) 1998 Robert Roebling and Julian Smart
// Licence:     wxWindows licence
/////////////////////////////////////////////////////////////////////////////

// For compilers that support precompilation, includes "wx.h".
#include "wx/wxprec.h"

#if defined(__BORLANDC__)
    #pragma hdrstop
#endif

#if wxUSE_PALETTE

#include "wx/palette.h"

//-----------------------------------------------------------------------------
// wxPalette
//-----------------------------------------------------------------------------

struct wxPaletteEntry
{
    unsigned char red, green, blue;
};

class wxPaletteRefData: public wxObjectRefData
{
  public:

    wxPaletteRefData(void);
    virtual ~wxPaletteRefData(void);

    int m_count;
    wxPaletteEntry *m_entries;
};

wxPaletteRefData::wxPaletteRefData()
{
    m_count = 0;
    m_entries = NULL;
}

wxPaletteRefData::~wxPaletteRefData()
{
    delete[] m_entries;
}

//-----------------------------------------------------------------------------

#define M_PALETTEDATA ((wxPaletteRefData *)m_refData)

IMPLEMENT_DYNAMIC_CLASS(wxPalette,wxGDIObject)

wxPalette::wxPalette()
{
    m_refData = NULL;
}

wxPalette::wxPalette(int n, const unsigned char *red, const unsigned char *green, const unsigned char *blue)
{
    Create(n, red, green, blue);
}

wxPalette::~wxPalette()
{
}

bool wxPalette::IsOk() const
{
    return (m_refData != NULL);
}

int wxPalette::GetColoursCount() const
{
    if (m_refData)
        return M_PALETTEDATA->m_count;
    
    return 0;    
}

bool wxPalette::Create(int n,
                       const unsigned char *red,
                       const unsigned char *green,
                       const unsigned char *blue)
{
    UnRef();
    m_refData = new wxPaletteRefData();

    M_PALETTEDATA->m_count = n;
    M_PALETTEDATA->m_entries = new wxPaletteEntry[n];

    wxPaletteEntry *e = M_PALETTEDATA->m_entries;
    for (int i = 0; i < n; i++, e++)
    {
        e->red = red[i];
        e->green = green[i];
        e->blue = blue[i];
    }

    return true;
}

int wxPalette::GetPixel( unsigned char red,
                         unsigned char green,
                         unsigned char blue ) const
{
    if (!m_refData) return wxNOT_FOUND;

    int closest = 0;
    double d,distance = 1000.0; // max. dist is 256

    wxPaletteEntry *e = M_PALETTEDATA->m_entries;
    for (int i = 0; i < M_PALETTEDATA->m_count; i++, e++)
    {
        if ((d = 0.299 * abs(red - e->red) +
                 0.587 * abs(green - e->green) +
                 0.114 * abs(blue - e->blue)) < distance) {
            distance = d;
            closest = i;
        }
    }
    return closest;
}

bool wxPalette::GetRGB(int pixel,
                       unsigned char *red,
                       unsigned char *green,
                       unsigned char *blue) const
{
    if (!m_refData) return false;
    if (pixel >= M_PALETTEDATA->m_count) return false;

    wxPaletteEntry& p = M_PALETTEDATA->m_entries[pixel];
    if (red) *red = p.red;
    if (green) *green = p.green;
    if (blue) *blue = p.blue;
    return true;
}

#endif // wxUSE_PALETTE
