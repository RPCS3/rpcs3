/////////////////////////////////////////////////////////////////////////////
// Name:        wx/palette.h
// Purpose:     Common header and base class for wxPalette
// Author:      Julian Smart
// Modified by:
// Created:
// RCS-ID:      $Id: palette.h 61872 2009-09-09 22:37:05Z VZ $
// Copyright:   (c) Julian Smart
// Licence:     wxWindows Licence
/////////////////////////////////////////////////////////////////////////////

#ifndef _WX_PALETTE_H_BASE_
#define _WX_PALETTE_H_BASE_

#include "wx/defs.h"

#if wxUSE_PALETTE

#include "wx/object.h"
#include "wx/gdiobj.h"

// wxPaletteBase
class WXDLLEXPORT wxPaletteBase: public wxGDIObject
{
public:
    virtual ~wxPaletteBase() { }

    virtual bool Ok() const { return IsOk(); }
    virtual bool IsOk() const = 0;
    virtual int GetColoursCount() const { wxFAIL_MSG( wxT("not implemented") ); return 0; }
};

#if defined(__WXPALMOS__)
    #include "wx/palmos/palette.h"
#elif defined(__WXMSW__)
    #include "wx/msw/palette.h"
#elif defined(__WXMOTIF__)
    #include "wx/motif/palette.h"
#elif defined(__WXGTK__) || defined(__WXCOCOA__)
    #include "wx/generic/paletteg.h"
#elif defined(__WXX11__)
    #include "wx/x11/palette.h"
#elif defined(__WXMGL__)
    #include "wx/mgl/palette.h"
#elif defined(__WXMAC__)
    #include "wx/mac/palette.h"
#elif defined(__WXPM__)
    #include "wx/os2/palette.h"
#endif

#if WXWIN_COMPATIBILITY_2_4
    #define wxColorMap wxPalette
    #define wxColourMap wxPalette
#endif

#endif // wxUSE_PALETTE

#endif
    // _WX_PALETTE_H_BASE_
