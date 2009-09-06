/////////////////////////////////////////////////////////////////////////////
// Name:        wx/unix/fontutil.h
// Purpose:     font-related helper functions for Unix/X11
// Author:      Vadim Zeitlin
// Modified by:
// Created:     05.11.99
// RCS-ID:      $Id: fontutil.h 27408 2004-05-23 20:53:33Z JS $
// Copyright:   (c) wxWidgets team
// Licence:     wxWindows licence
/////////////////////////////////////////////////////////////////////////////

#ifndef _WX_UNIX_FONTUTIL_H_
#define _WX_UNIX_FONTUTIL_H_

#ifdef __X__
    typedef WXFontStructPtr wxNativeFont;
#elif defined(__WXGTK__)
    typedef GdkFont *wxNativeFont;
#else
    #error "Unsupported toolkit"
#endif

// returns the handle of the nearest available font or 0
extern wxNativeFont
wxLoadQueryNearestFont(int pointSize,
                       int family,
                       int style,
                       int weight,
                       bool underlined,
                       const wxString &facename,
                       wxFontEncoding encoding,
                       wxString* xFontName = (wxString *)NULL);

// returns the font specified by the given XLFD
extern wxNativeFont wxLoadFont(const wxString& fontSpec);

#endif // _WX_UNIX_FONTUTIL_H_
