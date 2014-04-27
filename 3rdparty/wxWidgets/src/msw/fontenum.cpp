///////////////////////////////////////////////////////////////////////////////
// Name:        src/msw/fontenum.cpp
// Purpose:     wxFontEnumerator class for Windows
// Author:      Julian Smart
// Modified by: Vadim Zeitlin to add support for font encodings
// Created:     04/01/98
// RCS-ID:      $Id: fontenum.cpp 47549 2007-07-18 15:03:10Z VS $
// Copyright:   (c) Julian Smart
// Licence:     wxWindows licence
///////////////////////////////////////////////////////////////////////////////

// ============================================================================
// declarations
// ============================================================================

// ----------------------------------------------------------------------------
// headers
// ----------------------------------------------------------------------------

// For compilers that support precompilation, includes "wx.h".
#include "wx/wxprec.h"

#ifdef __BORLANDC__
  #pragma hdrstop
#endif

#if wxUSE_FONTMAP

#ifndef WX_PRECOMP
    #include "wx/gdicmn.h"
    #include "wx/font.h"
    #include "wx/encinfo.h"
    #include "wx/dynarray.h"
#endif

#include "wx/msw/private.h"

#include "wx/fontutil.h"
#include "wx/fontenum.h"
#include "wx/fontmap.h"

// ----------------------------------------------------------------------------
// private classes
// ----------------------------------------------------------------------------

// the helper class which calls ::EnumFontFamilies() and whose OnFont() is
// called from the callback passed to this function and, in its turn, calls the
// appropariate wxFontEnumerator method
class wxFontEnumeratorHelper
{
public:
    wxFontEnumeratorHelper(wxFontEnumerator *fontEnum);

    // control what exactly are we enumerating
        // we enumerate fonts with given enocding
    bool SetEncoding(wxFontEncoding encoding);
        // we enumerate fixed-width fonts
    void SetFixedOnly(bool fixedOnly) { m_fixedOnly = fixedOnly; }
        // we enumerate the encodings available in this family
    void SetFamily(const wxString& family);

    // call to start enumeration
    void DoEnumerate();

    // called by our font enumeration proc
    bool OnFont(const LPLOGFONT lf, const LPTEXTMETRIC tm) const;

private:
    // the object we forward calls to OnFont() to
    wxFontEnumerator *m_fontEnum;

    // if != -1, enum only fonts which have this encoding
    int m_charset;

    // if not empty, enum only the fonts with this facename
    wxString m_facename;

    // if not empty, enum only the fonts in this family
    wxString m_family;

    // if true, enum only fixed fonts
    bool m_fixedOnly;

    // if true, we enumerate the encodings, not fonts
    bool m_enumEncodings;

    // the list of charsets we already found while enumerating charsets
    wxArrayInt m_charsets;

    // the list of facenames we already found while enumerating facenames
    wxArrayString m_facenames;

    DECLARE_NO_COPY_CLASS(wxFontEnumeratorHelper)
};

// ----------------------------------------------------------------------------
// private functions
// ----------------------------------------------------------------------------

#ifndef __WXMICROWIN__
int CALLBACK wxFontEnumeratorProc(LPLOGFONT lplf, LPTEXTMETRIC lptm,
                                  DWORD dwStyle, LONG lParam);
#endif

// ============================================================================
// implementation
// ============================================================================

// ----------------------------------------------------------------------------
// wxFontEnumeratorHelper
// ----------------------------------------------------------------------------

wxFontEnumeratorHelper::wxFontEnumeratorHelper(wxFontEnumerator *fontEnum)
{
    m_fontEnum = fontEnum;
    m_charset = DEFAULT_CHARSET;
    m_fixedOnly = false;
    m_enumEncodings = false;
}

void wxFontEnumeratorHelper::SetFamily(const wxString& family)
{
    m_enumEncodings = true;
    m_family = family;
}

bool wxFontEnumeratorHelper::SetEncoding(wxFontEncoding encoding)
{
    if ( encoding != wxFONTENCODING_SYSTEM )
    {
        wxNativeEncodingInfo info;
        if ( !wxGetNativeFontEncoding(encoding, &info) )
        {
#if wxUSE_FONTMAP
            if ( !wxFontMapper::Get()->GetAltForEncoding(encoding, &info) )
#endif // wxUSE_FONTMAP
            {
                // no such encodings at all
                return false;
            }
        }

        m_charset = info.charset;
        m_facename = info.facename;
    }

    return true;
}

#if defined(__GNUWIN32__) && !defined(__CYGWIN10__) && !wxCHECK_W32API_VERSION( 1, 1 ) && !wxUSE_NORLANDER_HEADERS
    #define wxFONTENUMPROC int(*)(ENUMLOGFONTEX *, NEWTEXTMETRICEX*, int, LPARAM)
#else
    #define wxFONTENUMPROC FONTENUMPROC
#endif

void wxFontEnumeratorHelper::DoEnumerate()
{
#ifndef __WXMICROWIN__
    HDC hDC = ::GetDC(NULL);

#ifdef __WXWINCE__
    ::EnumFontFamilies(hDC,
                       m_facename.empty() ? NULL : m_facename.c_str(),
                       (wxFONTENUMPROC)wxFontEnumeratorProc,
                       (LPARAM)this) ;
#else // __WIN32__
    LOGFONT lf;
    lf.lfCharSet = (BYTE)m_charset;
    wxStrncpy(lf.lfFaceName, m_facename, WXSIZEOF(lf.lfFaceName));
    lf.lfPitchAndFamily = 0;
    ::EnumFontFamiliesEx(hDC, &lf, (wxFONTENUMPROC)wxFontEnumeratorProc,
                         (LPARAM)this, 0 /* reserved */) ;
#endif // Win32/CE

    ::ReleaseDC(NULL, hDC);
#endif
}

bool wxFontEnumeratorHelper::OnFont(const LPLOGFONT lf,
                                    const LPTEXTMETRIC tm) const
{
    if ( m_enumEncodings )
    {
        // is this a new charset?
        int cs = lf->lfCharSet;
        if ( m_charsets.Index(cs) == wxNOT_FOUND )
        {
            wxConstCast(this, wxFontEnumeratorHelper)->m_charsets.Add(cs);

            wxFontEncoding enc = wxGetFontEncFromCharSet(cs);
            return m_fontEnum->OnFontEncoding(lf->lfFaceName,
                                              wxFontMapper::GetEncodingName(enc));
        }
        else
        {
            // continue enumeration
            return true;
        }
    }

    if ( m_fixedOnly )
    {
        // check that it's a fixed pitch font (there is *no* error here, the
        // flag name is misleading!)
        if ( tm->tmPitchAndFamily & TMPF_FIXED_PITCH )
        {
            // not a fixed pitch font
            return true;
        }
    }

    if ( m_charset != DEFAULT_CHARSET )
    {
        // check that we have the right encoding
        if ( lf->lfCharSet != m_charset )
        {
            return true;
        }
    }
    else // enumerating fonts in all charsets
    {
        // we can get the same facename twice or more in this case because it
        // may exist in several charsets but we only want to return one copy of
        // it (note that this can't happen for m_charset != DEFAULT_CHARSET)
        if ( m_facenames.Index(lf->lfFaceName) != wxNOT_FOUND )
        {
            // continue enumeration
            return true;
        }

        wxConstCast(this, wxFontEnumeratorHelper)->
            m_facenames.Add(lf->lfFaceName);
    }

    return m_fontEnum->OnFacename(lf->lfFaceName);
}

// ----------------------------------------------------------------------------
// wxFontEnumerator
// ----------------------------------------------------------------------------

bool wxFontEnumerator::EnumerateFacenames(wxFontEncoding encoding,
                                          bool fixedWidthOnly)
{
    wxFontEnumeratorHelper fe(this);
    if ( fe.SetEncoding(encoding) )
    {
        fe.SetFixedOnly(fixedWidthOnly);

        fe.DoEnumerate();
    }
    // else: no such fonts, unknown encoding

    return true;
}

bool wxFontEnumerator::EnumerateEncodings(const wxString& family)
{
    wxFontEnumeratorHelper fe(this);
    fe.SetFamily(family);
    fe.DoEnumerate();

    return true;
}

// ----------------------------------------------------------------------------
// Windows callbacks
// ----------------------------------------------------------------------------

#ifndef __WXMICROWIN__
int CALLBACK wxFontEnumeratorProc(LPLOGFONT lplf, LPTEXTMETRIC lptm,
                                  DWORD WXUNUSED(dwStyle), LONG lParam)
{

    // we used to process TrueType fonts only, but there doesn't seem to be any
    // reasons to restrict ourselves to them here
#if 0
    // Get rid of any fonts that we don't want...
    if ( dwStyle != TRUETYPE_FONTTYPE )
    {
        // continue enumeration
        return TRUE;
    }
#endif // 0

    wxFontEnumeratorHelper *fontEnum = (wxFontEnumeratorHelper *)lParam;

    return fontEnum->OnFont(lplf, lptm);
}
#endif

#endif // wxUSE_FONTMAP
