/////////////////////////////////////////////////////////////////////////////
// Name:        wx/msw/palette.h
// Purpose:     wxPalette class
// Author:      Julian Smart
// Modified by:
// Created:     01/02/97
// RCS-ID:      $Id: palette.h 53135 2008-04-12 02:31:04Z VZ $
// Copyright:   (c) Julian Smart
// Licence:     wxWindows licence
/////////////////////////////////////////////////////////////////////////////

#ifndef _WX_PALETTE_H_
#define _WX_PALETTE_H_

#include "wx/gdiobj.h"

class WXDLLIMPEXP_FWD_CORE wxPalette;

class WXDLLEXPORT wxPaletteRefData: public wxGDIRefData
{
    friend class WXDLLIMPEXP_FWD_CORE wxPalette;
public:
    wxPaletteRefData(void);
    virtual ~wxPaletteRefData(void);
protected:
 WXHPALETTE m_hPalette;
};

#define M_PALETTEDATA ((wxPaletteRefData *)m_refData)

class WXDLLEXPORT wxPalette: public wxPaletteBase
{
public:
    wxPalette();
    wxPalette(int n, const unsigned char *red, const unsigned char *green, const unsigned char *blue);
    virtual ~wxPalette(void);
    bool Create(int n, const unsigned char *red, const unsigned char *green, const unsigned char *blue);
    
    int GetPixel(unsigned char red, unsigned char green, unsigned char blue) const;
    bool GetRGB(int pixel, unsigned char *red, unsigned char *green, unsigned char *blue) const;

    virtual int GetColoursCount() const;

    virtual bool Ok() const { return IsOk(); }
    virtual bool IsOk(void) const { return (m_refData != NULL) ; }

    virtual bool FreeResource(bool force = false);

    // implemetation
    inline WXHPALETTE GetHPALETTE(void) const { return (M_PALETTEDATA ? M_PALETTEDATA->m_hPalette : 0); }
    void SetHPALETTE(WXHPALETTE pal);
  
private:
    DECLARE_DYNAMIC_CLASS(wxPalette)
};

#endif
    // _WX_PALETTE_H_
