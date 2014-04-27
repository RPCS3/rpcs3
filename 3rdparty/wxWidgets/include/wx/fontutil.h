/////////////////////////////////////////////////////////////////////////////
// Name:        wx/fontutil.h
// Purpose:     font-related helper functions
// Author:      Vadim Zeitlin
// Modified by:
// Created:     05.11.99
// RCS-ID:      $Id: fontutil.h 49563 2007-10-31 20:46:21Z VZ $
// Copyright:   (c) wxWidgets team
// Licence:     wxWindows licence
/////////////////////////////////////////////////////////////////////////////

// General note: this header is private to wxWidgets and is not supposed to be
// included by user code. The functions declared here are implemented in
// msw/fontutil.cpp for Windows, unix/fontutil.cpp for GTK/Motif &c.

#ifndef _WX_FONTUTIL_H_
#define _WX_FONTUTIL_H_

// ----------------------------------------------------------------------------
// headers
// ----------------------------------------------------------------------------

#include "wx/font.h"        // for wxFont and wxFontEncoding

#if defined(__WXMSW__)
    #include "wx/msw/wrapwin.h"
#endif

class WXDLLIMPEXP_FWD_BASE wxArrayString;
struct WXDLLIMPEXP_FWD_CORE wxNativeEncodingInfo;

#if defined(_WX_X_FONTLIKE)

// the symbolic names for the XLFD fields (with examples for their value)
//
// NB: we suppose that the font always starts with the empty token (font name
//     registry field) as we never use nor generate it anyhow
enum wxXLFDField
{
    wxXLFD_FOUNDRY,     // adobe
    wxXLFD_FAMILY,      // courier, times, ...
    wxXLFD_WEIGHT,      // black, bold, demibold, medium, regular, light
    wxXLFD_SLANT,       // r/i/o (roman/italique/oblique)
    wxXLFD_SETWIDTH,    // condensed, expanded, ...
    wxXLFD_ADDSTYLE,    // whatever - usually nothing
    wxXLFD_PIXELSIZE,   // size in pixels
    wxXLFD_POINTSIZE,   // size in points
    wxXLFD_RESX,        // 72, 75, 100, ...
    wxXLFD_RESY,
    wxXLFD_SPACING,     // m/p/c (monospaced/proportional/character cell)
    wxXLFD_AVGWIDTH,    // average width in 1/10 pixels
    wxXLFD_REGISTRY,    // iso8859, rawin, koi8, ...
    wxXLFD_ENCODING,    // 1, r, r, ...
    wxXLFD_MAX
};

#endif // _WX_X_FONTLIKE

// ----------------------------------------------------------------------------
// types
// ----------------------------------------------------------------------------

// wxNativeFontInfo is platform-specific font representation: this struct
// should be considered as opaque font description only used by the native
// functions, the user code can only get the objects of this type from
// somewhere and pass it somewhere else (possibly save them somewhere using
// ToString() and restore them using FromString())
class WXDLLEXPORT wxNativeFontInfo
{
public:
#if wxUSE_PANGO
    PangoFontDescription *description;
#elif defined(_WX_X_FONTLIKE)
    // the members can't be accessed directly as we only parse the
    // xFontName on demand
private:
    // the components of the XLFD
    wxString     fontElements[wxXLFD_MAX];

    // the full XLFD
    wxString     xFontName;

    // true until SetXFontName() is called
    bool         m_isDefault;

    // return true if we have already initialized fontElements
    inline bool HasElements() const;

public:
    // init the elements from an XLFD, return true if ok
    bool FromXFontName(const wxString& xFontName);

    // return false if we were never initialized with a valid XLFD
    bool IsDefault() const { return m_isDefault; }

    // return the XLFD (using the fontElements if necessary)
    wxString GetXFontName() const;

    // get the given XFLD component
    wxString GetXFontComponent(wxXLFDField field) const;

    // change the font component
    void SetXFontComponent(wxXLFDField field, const wxString& value);

    // set the XFLD
    void SetXFontName(const wxString& xFontName);
#elif defined(__WXMSW__)
    LOGFONT      lf;
#elif defined(__WXPM__)
    // OS/2 native structures that define a font
    FATTRS       fa;
    FONTMETRICS  fm;
    FACENAMEDESC fn;
#else // other platforms
    //
    //  This is a generic implementation that should work on all ports
    //  without specific support by the port.
    //
    #define wxNO_NATIVE_FONTINFO

    int           pointSize;
    wxFontFamily  family;
    wxFontStyle   style;
    wxFontWeight  weight;
    bool          underlined;
    wxString      faceName;
    wxFontEncoding encoding;
#endif // platforms

    // default ctor (default copy ctor is ok)
    wxNativeFontInfo() { Init(); }

#if wxUSE_PANGO
private:
    void Init(const wxNativeFontInfo& info);
    void Free();

public:
    wxNativeFontInfo(const wxNativeFontInfo& info) { Init(info); }
    ~wxNativeFontInfo() { Free(); }

    wxNativeFontInfo& operator=(const wxNativeFontInfo& info)
    {
        Free();
        Init(info);
        return *this;
    }
#endif // wxUSE_PANGO

    // reset to the default state
    void Init();

    // init with the parameters of the given font
    void InitFromFont(const wxFont& font)
    {
        // translate all font parameters
        SetStyle((wxFontStyle)font.GetStyle());
        SetWeight((wxFontWeight)font.GetWeight());
        SetUnderlined(font.GetUnderlined());
#if defined(__WXMSW__)
        if ( font.IsUsingSizeInPixels() )
            SetPixelSize(font.GetPixelSize());
        else
            SetPointSize(font.GetPointSize());
#else
        SetPointSize(font.GetPointSize());
#endif

        // set the family/facename
        SetFamily((wxFontFamily)font.GetFamily());
        const wxString& facename = font.GetFaceName();
        if ( !facename.empty() )
        {
            SetFaceName(facename);
        }

        // deal with encoding now (it may override the font family and facename
        // so do it after setting them)
        SetEncoding(font.GetEncoding());
    }

    // accessors and modifiers for the font elements
    int GetPointSize() const;
    wxSize GetPixelSize() const;
    wxFontStyle GetStyle() const;
    wxFontWeight GetWeight() const;
    bool GetUnderlined() const;
    wxString GetFaceName() const;
    wxFontFamily GetFamily() const;
    wxFontEncoding GetEncoding() const;

    void SetPointSize(int pointsize);
    void SetPixelSize(const wxSize& pixelSize);
    void SetStyle(wxFontStyle style);
    void SetWeight(wxFontWeight weight);
    void SetUnderlined(bool underlined);
    bool SetFaceName(const wxString& facename);
    void SetFamily(wxFontFamily family);
    void SetEncoding(wxFontEncoding encoding);

    // sets the first facename in the given array which is found
    // to be valid. If no valid facename is given, sets the
    // first valid facename returned by wxFontEnumerator::GetFacenames().
    // Does not return a bool since it cannot fail.
    void SetFaceName(const wxArrayString &facenames);


    // it is important to be able to serialize wxNativeFontInfo objects to be
    // able to store them (in config file, for example)
    bool FromString(const wxString& s);
    wxString ToString() const;

    // we also want to present the native font descriptions to the user in some
    // human-readable form (it is not platform independent neither, but can
    // hopefully be understood by the user)
    bool FromUserString(const wxString& s);
    wxString ToUserString() const;
};

// ----------------------------------------------------------------------------
// font-related functions (common)
// ----------------------------------------------------------------------------

// translate a wxFontEncoding into native encoding parameter (defined above),
// returning true if an (exact) macth could be found, false otherwise (without
// attempting any substitutions)
extern bool wxGetNativeFontEncoding(wxFontEncoding encoding,
                                    wxNativeEncodingInfo *info);

// test for the existence of the font described by this facename/encoding,
// return true if such font(s) exist, false otherwise
extern bool wxTestFontEncoding(const wxNativeEncodingInfo& info);

// ----------------------------------------------------------------------------
// font-related functions (X and GTK)
// ----------------------------------------------------------------------------

#ifdef _WX_X_FONTLIKE
    #include "wx/unix/fontutil.h"
#endif // X || GDK

#endif // _WX_FONTUTIL_H_
