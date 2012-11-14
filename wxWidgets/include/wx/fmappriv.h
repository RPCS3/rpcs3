///////////////////////////////////////////////////////////////////////////////
// Name:        wx/fmappriv.h
// Purpose:     private wxFontMapper stuff, not to be used by the library users
// Author:      Vadim Zeitlin
// Modified by:
// Created:     21.06.2003 (extracted from common/fontmap.cpp)
// RCS-ID:      $Id: fmappriv.h 27454 2004-05-26 10:49:43Z JS $
// Copyright:   (c) 1999-2003 Vadim Zeitlin <vadim@wxwidgets.org>
// Licence:     wxWindows licence
///////////////////////////////////////////////////////////////////////////////

#ifndef _WX_FMAPPRIV_H_
#define _WX_FMAPPRIV_H_

// ----------------------------------------------------------------------------
// constants
// ----------------------------------------------------------------------------

// a special pseudo encoding which means "don't ask me about this charset
// any more" -- we need it to avoid driving the user crazy with asking him
// time after time about the same charset which he [presumably] doesn't
// have the fonts for
enum { wxFONTENCODING_UNKNOWN = -2 };

// the config paths we use
#if wxUSE_CONFIG

#define FONTMAPPER_ROOT_PATH wxT("/wxWindows/FontMapper")
#define FONTMAPPER_CHARSET_PATH wxT("Charsets")
#define FONTMAPPER_CHARSET_ALIAS_PATH wxT("Aliases")

#endif // wxUSE_CONFIG

// ----------------------------------------------------------------------------
// wxFontMapperPathChanger: change the config path during our lifetime
// ----------------------------------------------------------------------------

#if wxUSE_CONFIG && wxUSE_FILECONFIG

class wxFontMapperPathChanger
{
public:
    wxFontMapperPathChanger(wxFontMapperBase *fontMapper, const wxString& path)
    {
        m_fontMapper = fontMapper;
        m_ok = m_fontMapper->ChangePath(path, &m_pathOld);
    }

    bool IsOk() const { return m_ok; }

    ~wxFontMapperPathChanger()
    {
        if ( IsOk() )
            m_fontMapper->RestorePath(m_pathOld);
    }

private:
    // the fontmapper object we're working with
    wxFontMapperBase *m_fontMapper;

    // the old path to be restored if m_ok
    wxString m_pathOld;

    // have we changed the path successfully?
    bool m_ok;


    DECLARE_NO_COPY_CLASS(wxFontMapperPathChanger)
};

#endif // wxUSE_CONFIG

#endif // _WX_FMAPPRIV_H_

