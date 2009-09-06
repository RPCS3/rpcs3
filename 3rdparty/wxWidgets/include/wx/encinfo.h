///////////////////////////////////////////////////////////////////////////////
// Name:        wx/encinfo.h
// Purpose:     declares wxNativeEncodingInfo struct
// Author:      Vadim Zeitlin
// Modified by:
// Created:     19.09.2003 (extracted from wx/fontenc.h)
// RCS-ID:      $Id: encinfo.h 40865 2006-08-27 09:42:42Z VS $
// Copyright:   (c) 2003 Vadim Zeitlin <vadim@wxwidgets.org>
// Licence:     wxWindows licence
///////////////////////////////////////////////////////////////////////////////

#ifndef _WX_ENCINFO_H_
#define _WX_ENCINFO_H_

#include "wx/string.h"

// ----------------------------------------------------------------------------
// wxNativeEncodingInfo contains all encoding parameters for this platform
// ----------------------------------------------------------------------------

// This private structure specifies all the parameters needed to create a font
// with the given encoding on this platform.
//
// Under X, it contains the last 2 elements of the font specifications
// (registry and encoding).
//
// Under Windows, it contains a number which is one of predefined CHARSET_XXX
// values.
//
// Under all platforms it also contains a facename string which should be
// used, if not empty, to create fonts in this encoding (this is the only way
// to create a font of non-standard encoding (like KOI8) under Windows - the
// facename specifies the encoding then)

struct WXDLLEXPORT wxNativeEncodingInfo
{
    wxString facename;          // may be empty meaning "any"
#ifndef __WXPALMOS__
    wxFontEncoding encoding;    // so that we know what this struct represents

#if defined(__WXMSW__) || \
    defined(__WXPM__)  || \
    defined(__WXMAC__) || \
    defined(__WXCOCOA__) // FIXME: __WXCOCOA__

    wxNativeEncodingInfo()
        : facename()
        , encoding(wxFONTENCODING_SYSTEM)
        , charset(0) /* ANSI_CHARSET */
    { }

    int      charset;
#elif defined(_WX_X_FONTLIKE)
    wxString xregistry,
             xencoding;
#elif defined(__WXGTK20__)
    // No way to specify this in Pango as this
    // seems to be handled internally.
#elif defined(__WXMGL__)
    int      mglEncoding;
#elif defined(__WXDFB__)
    // DirectFB uses UTF-8 internally, doesn't use font encodings
#else
    #error "Unsupported toolkit"
#endif
#endif // !__WXPALMOS__
    // this struct is saved in config by wxFontMapper, so it should know to
    // serialise itself (implemented in platform-specific code)
    bool FromString(const wxString& s);
    wxString ToString() const;
};

#endif // _WX_ENCINFO_H_

