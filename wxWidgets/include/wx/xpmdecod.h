/////////////////////////////////////////////////////////////////////////////
// Name:        wx/xpmdecod.h
// Purpose:     wxXPMDecoder, XPM reader for wxImage and wxBitmap
// Author:      Vaclav Slavik
// CVS-ID:      $Id: xpmdecod.h 49804 2007-11-10 01:09:42Z VZ $
// Copyright:   (c) 2001 Vaclav Slavik
// Licence:     wxWindows licence
/////////////////////////////////////////////////////////////////////////////

#ifndef _WX_XPMDECOD_H_
#define _WX_XPMDECOD_H_

#include "wx/defs.h"

#if wxUSE_IMAGE && wxUSE_XPM

class WXDLLIMPEXP_FWD_CORE wxImage;
class WXDLLIMPEXP_FWD_BASE wxInputStream;

// --------------------------------------------------------------------------
// wxXPMDecoder class
// --------------------------------------------------------------------------

class WXDLLEXPORT wxXPMDecoder
{
public:
    // constructor, destructor, etc.
    wxXPMDecoder() {}
    ~wxXPMDecoder() {}

#if wxUSE_STREAMS
    // Is the stream XPM file?
    bool CanRead(wxInputStream& stream);
    // Read XPM file from the stream, parse it and create image from it
    wxImage ReadFile(wxInputStream& stream);
#endif
    // Read directly from XPM data (as passed to wxBitmap ctor):
    wxImage ReadData(const char* const* xpm_data);
#ifdef __BORLANDC__
    // needed for Borland 5.5
    wxImage ReadData(char** xpm_data) { return ReadData(wx_const_cast(const char* const*, xpm_data)); }
#endif
};

#endif // wxUSE_IMAGE && wxUSE_XPM

#endif  // _WX_XPM_H_
