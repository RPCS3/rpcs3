/////////////////////////////////////////////////////////////////////////////
// Name:        wx/generic/icon.h
// Purpose:     wxIcon implementation for ports where it's same as wxBitmap
// Author:      Julian Smart
// Modified by:
// Created:     17/09/98
// RCS-ID:      $Id: icon.h 42752 2006-10-30 19:26:48Z VZ $
// Copyright:   (c) Julian Smart
// Licence:     wxWindows licence
/////////////////////////////////////////////////////////////////////////////

#ifndef _WX_GENERIC_ICON_H_
#define _WX_GENERIC_ICON_H_

#include "wx/bitmap.h"

//-----------------------------------------------------------------------------
// wxIcon
//-----------------------------------------------------------------------------

#ifndef wxICON_DEFAULT_BITMAP_TYPE
#define wxICON_DEFAULT_BITMAP_TYPE wxBITMAP_TYPE_XPM
#endif

class WXDLLIMPEXP_CORE wxIcon: public wxBitmap
{
public:
    wxIcon();

    wxIcon( const char **bits, int width=-1, int height=-1 );
    wxIcon( char **bits, int width=-1, int height=-1 );

    // For compatibility with wxMSW where desired size is sometimes required to
    // distinguish between multiple icons in a resource.
    wxIcon( const wxString& filename,
            wxBitmapType type = wxICON_DEFAULT_BITMAP_TYPE,
            int WXUNUSED(desiredWidth)=-1, int WXUNUSED(desiredHeight)=-1 ) :
        wxBitmap(filename, type)
    {
    }

    wxIcon(const wxIconLocation& loc)
        : wxBitmap(loc.GetFileName(), wxBITMAP_TYPE_ANY)
    {
    }

    // create from bitmap (which should have a mask unless it's monochrome):
    // there shouldn't be any implicit bitmap -> icon conversion (i.e. no
    // ctors, assignment operators...), but it's ok to have such function
    void CopyFromBitmap(const wxBitmap& bmp);

private:
    DECLARE_DYNAMIC_CLASS(wxIcon)
};

#endif // _WX_GENERIC_ICON_H_
