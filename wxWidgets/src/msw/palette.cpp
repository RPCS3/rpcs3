/////////////////////////////////////////////////////////////////////////////
// Name:        src/msw/palette.cpp
// Purpose:     wxPalette
// Author:      Julian Smart
// Modified by:
// Created:     04/01/98
// RCS-ID:      $Id: palette.cpp 67095 2011-03-01 00:02:52Z VZ $
// Copyright:   (c) Julian Smart
// Licence:     wxWindows licence
/////////////////////////////////////////////////////////////////////////////

// For compilers that support precompilation, includes "wx.h".
#include "wx/wxprec.h"

#ifdef __BORLANDC__
    #pragma hdrstop
#endif

#if wxUSE_PALETTE

#include "wx/palette.h"

#ifndef WX_PRECOMP
#endif

#include "wx/msw/private.h"

IMPLEMENT_DYNAMIC_CLASS(wxPalette, wxGDIObject)

/*
 * Palette
 *
 */

wxPaletteRefData::wxPaletteRefData(void)
{
    m_hPalette = 0;
}

wxPaletteRefData::~wxPaletteRefData(void)
{
    if ( m_hPalette )
        ::DeleteObject((HPALETTE) m_hPalette);
}

wxPalette::wxPalette()
{
}

wxPalette::wxPalette(int n, const unsigned char *red, const unsigned char *green, const unsigned char *blue)
{
    Create(n, red, green, blue);
}

wxPalette::~wxPalette(void)
{
//    FreeResource(true);
}

bool wxPalette::FreeResource(bool WXUNUSED(force))
{
    if ( M_PALETTEDATA && M_PALETTEDATA->m_hPalette)
    {
        DeleteObject((HPALETTE)M_PALETTEDATA->m_hPalette);
    }
    
    return true;
}

int wxPalette::GetColoursCount() const
{
    if ( M_PALETTEDATA && M_PALETTEDATA->m_hPalette)
    {
        return ::GetPaletteEntries((HPALETTE) M_PALETTEDATA->m_hPalette, 0, 0, NULL );
    }
    
    return 0;
}

bool wxPalette::Create(int n, const unsigned char *red, const unsigned char *green, const unsigned char *blue)
{
    // The number of colours in LOGPALETTE is a WORD so we can't create
    // palettes with more entries than USHRT_MAX.
    if ( n < 0 || n > 65535 )
        return false;

    UnRef();

#if defined(__WXMICROWIN__)

    return false;

#else

    m_refData = new wxPaletteRefData;

    NPLOGPALETTE npPal = (NPLOGPALETTE)LocalAlloc(LMEM_FIXED, sizeof(LOGPALETTE) +
                          (WORD)n * sizeof(PALETTEENTRY));
    if (!npPal)
        return false;

    npPal->palVersion = 0x300;
    npPal->palNumEntries = (WORD)n;

    int i;
    for (i = 0; i < n; i ++)
    {
        npPal->palPalEntry[i].peRed = red[i];
        npPal->palPalEntry[i].peGreen = green[i];
        npPal->palPalEntry[i].peBlue = blue[i];
        npPal->palPalEntry[i].peFlags = 0;
    }
    M_PALETTEDATA->m_hPalette = (WXHPALETTE) CreatePalette((LPLOGPALETTE)npPal);
    LocalFree((HANDLE)npPal);
    return true;

#endif
}

int wxPalette::GetPixel(unsigned char red, unsigned char green, unsigned char blue) const
{
#ifdef __WXMICROWIN__
    return wxNOT_FOUND;
#else
    if ( !m_refData )
        return wxNOT_FOUND;

    return ::GetNearestPaletteIndex((HPALETTE) M_PALETTEDATA->m_hPalette, PALETTERGB(red, green, blue));
#endif
}

bool wxPalette::GetRGB(int index, unsigned char *red, unsigned char *green, unsigned char *blue) const
{
#ifdef __WXMICROWIN__
    return false;
#else
    if ( !m_refData )
        return false;

    if (index < 0 || index > 255)
        return false;

    PALETTEENTRY entry;
    if (::GetPaletteEntries((HPALETTE) M_PALETTEDATA->m_hPalette, index, 1, &entry))
    {
        *red = entry.peRed;
        *green = entry.peGreen;
        *blue = entry.peBlue;
        return true;
    }
    else
        return false;
#endif
}

void wxPalette::SetHPALETTE(WXHPALETTE pal)
{
    if ( !m_refData )
        m_refData = new wxPaletteRefData;

    M_PALETTEDATA->m_hPalette = pal;
}

#endif // wxUSE_PALETTE
