/////////////////////////////////////////////////////////////////////////////
// Name:        wx/generic/paletteg.h
// Purpose:
// Author:      Robert Roebling
// Created:     01/02/97
// RCS-ID:      $Id: paletteg.h 42752 2006-10-30 19:26:48Z VZ $
// Copyright:   (c) 1998 Robert Roebling and Julian Smart
// Licence:     wxWindows licence
/////////////////////////////////////////////////////////////////////////////


#ifndef __WX_PALETTEG_H__
#define __WX_PALETTEG_H__

#include "wx/defs.h"
#include "wx/object.h"
#include "wx/gdiobj.h"
#include "wx/gdicmn.h"

//-----------------------------------------------------------------------------
// classes
//-----------------------------------------------------------------------------

class WXDLLIMPEXP_CORE wxPalette;

//-----------------------------------------------------------------------------
// wxPalette
//-----------------------------------------------------------------------------

class WXDLLIMPEXP_CORE wxPalette: public wxPaletteBase
{
public:
    wxPalette();
    wxPalette( int n, const unsigned char *red, const unsigned char *green, const unsigned char *blue );
    virtual ~wxPalette();
    virtual bool Ok() const { return IsOk(); }
    virtual bool IsOk() const;

    bool Create( int n, const unsigned char *red, const unsigned char *green, const unsigned char *blue);
    int GetPixel( unsigned char red, unsigned char green, unsigned char blue ) const;
    bool GetRGB( int pixel, unsigned char *red, unsigned char *green, unsigned char *blue ) const;

    virtual int GetColoursCount() const;

private:
    DECLARE_DYNAMIC_CLASS(wxPalette)
};

#endif // __WX_PALETTEG_H__
