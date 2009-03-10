/////////////////////////////////////////////////////////////////////////////
// Name:        src/common/fontmap.cpp
// Purpose:     wxFontMapper class
// Author:      Vadim Zeitlin
// Modified by:
// Created:     04.11.99
// RCS-ID:      $Id: fontmap.cpp 39651 2006-06-09 17:50:46Z ABX $
// Copyright:   (c) 1999-2003 Vadim Zeitlin <vadim@wxwindows.org>
// Licence:     wxWindows licence
/////////////////////////////////////////////////////////////////////////////

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

#include "wx/fontmap.h"

#ifndef WX_PRECOMP
    #include "wx/app.h"
    #include "wx/log.h"
    #include "wx/intl.h"
    #include "wx/msgdlg.h"
    #include "wx/choicdlg.h"
#endif // PCH

#if wxUSE_CONFIG
    #include "wx/config.h"
#endif // wxUSE_CONFIG

#if defined(__WXMSW__)
  #include  "wx/msw/private.h"  // includes windows.h for LOGFONT
  #include  "wx/msw/winundef.h"
#endif

#include "wx/fmappriv.h"
#include "wx/fontutil.h"
#include "wx/fontdlg.h"
#include "wx/encinfo.h"

#include "wx/encconv.h"

#if wxUSE_EXTENDED_RTTI

wxBEGIN_ENUM( wxFontEncoding )
    wxENUM_MEMBER( wxFONTENCODING_SYSTEM )
    wxENUM_MEMBER( wxFONTENCODING_DEFAULT )

    wxENUM_MEMBER( wxFONTENCODING_ISO8859_1 )
    wxENUM_MEMBER( wxFONTENCODING_ISO8859_2 )
    wxENUM_MEMBER( wxFONTENCODING_ISO8859_3 )
    wxENUM_MEMBER( wxFONTENCODING_ISO8859_4 )
    wxENUM_MEMBER( wxFONTENCODING_ISO8859_5 )
    wxENUM_MEMBER( wxFONTENCODING_ISO8859_6 )
    wxENUM_MEMBER( wxFONTENCODING_ISO8859_7 )
    wxENUM_MEMBER( wxFONTENCODING_ISO8859_8 )
    wxENUM_MEMBER( wxFONTENCODING_ISO8859_9 )
    wxENUM_MEMBER( wxFONTENCODING_ISO8859_10 )
    wxENUM_MEMBER( wxFONTENCODING_ISO8859_11 )
    wxENUM_MEMBER( wxFONTENCODING_ISO8859_12 )
    wxENUM_MEMBER( wxFONTENCODING_ISO8859_13 )
    wxENUM_MEMBER( wxFONTENCODING_ISO8859_14 )
    wxENUM_MEMBER( wxFONTENCODING_ISO8859_15 )
    wxENUM_MEMBER( wxFONTENCODING_ISO8859_MAX )
    wxENUM_MEMBER( wxFONTENCODING_KOI8 )
    wxENUM_MEMBER( wxFONTENCODING_KOI8_U )
    wxENUM_MEMBER( wxFONTENCODING_ALTERNATIVE )
    wxENUM_MEMBER( wxFONTENCODING_BULGARIAN )
    wxENUM_MEMBER( wxFONTENCODING_CP437 )
    wxENUM_MEMBER( wxFONTENCODING_CP850 )
    wxENUM_MEMBER( wxFONTENCODING_CP852 )
    wxENUM_MEMBER( wxFONTENCODING_CP855 )
    wxENUM_MEMBER( wxFONTENCODING_CP866 )

    wxENUM_MEMBER( wxFONTENCODING_CP874 )
    wxENUM_MEMBER( wxFONTENCODING_CP932 )
    wxENUM_MEMBER( wxFONTENCODING_CP936 )
    wxENUM_MEMBER( wxFONTENCODING_CP949 )
    wxENUM_MEMBER( wxFONTENCODING_CP950 )
    wxENUM_MEMBER( wxFONTENCODING_CP1250 )
    wxENUM_MEMBER( wxFONTENCODING_CP1251 )
    wxENUM_MEMBER( wxFONTENCODING_CP1252 )
    wxENUM_MEMBER( wxFONTENCODING_CP1253 )
    wxENUM_MEMBER( wxFONTENCODING_CP1254 )
    wxENUM_MEMBER( wxFONTENCODING_CP1255 )
    wxENUM_MEMBER( wxFONTENCODING_CP1256 )
    wxENUM_MEMBER( wxFONTENCODING_CP1257 )
    wxENUM_MEMBER( wxFONTENCODING_CP12_MAX )
    wxENUM_MEMBER( wxFONTENCODING_UTF7 )
    wxENUM_MEMBER( wxFONTENCODING_UTF8 )
    wxENUM_MEMBER( wxFONTENCODING_GB2312 )
    wxENUM_MEMBER( wxFONTENCODING_BIG5 )
    wxENUM_MEMBER( wxFONTENCODING_SHIFT_JIS )
    wxENUM_MEMBER( wxFONTENCODING_EUC_JP )
    wxENUM_MEMBER( wxFONTENCODING_UNICODE )
wxEND_ENUM( wxFontEncoding )
#endif

// ----------------------------------------------------------------------------
// constants
// ----------------------------------------------------------------------------

// the config paths we use
#if wxUSE_CONFIG

static const wxChar* FONTMAPPER_FONT_FROM_ENCODING_PATH = wxT("Encodings");
static const wxChar* FONTMAPPER_FONT_DONT_ASK = wxT("none");

#endif // wxUSE_CONFIG

// ----------------------------------------------------------------------------
// private classes
// ----------------------------------------------------------------------------

// it may happen that while we're showing a dialog asking the user about
// something, another request for an encoding mapping arrives: in this case it
// is best to not do anything because otherwise we risk to enter an infinite
// loop so we create an object of this class on stack to test for this in all
// interactive functions
class ReentrancyBlocker
{
public:
    ReentrancyBlocker(bool& flag) : m_flagOld(flag), m_flag(flag)
        { m_flag = true; }
    ~ReentrancyBlocker() { m_flag = m_flagOld; }

private:
    bool m_flagOld;
    bool& m_flag;

    DECLARE_NO_COPY_CLASS(ReentrancyBlocker)
};

// ============================================================================
// implementation
// ============================================================================

// ----------------------------------------------------------------------------
// ctor and dtor
// ----------------------------------------------------------------------------

wxFontMapper::wxFontMapper()
{
    m_windowParent = NULL;
}

wxFontMapper::~wxFontMapper()
{
}

/* static */
wxFontMapper *wxFontMapper::Get()
{
    wxFontMapperBase *fontmapper = wxFontMapperBase::Get();
    wxASSERT_MSG( !fontmapper->IsDummy(),
                 wxT("GUI code requested a wxFontMapper but we only have a wxFontMapperBase.") );

    // Now return it anyway because there's a chance the GUI code might just
    // only want to call wxFontMapperBase functions and it's better than
    // crashing by returning NULL
    return (wxFontMapper *)fontmapper;
}

wxFontEncoding
wxFontMapper::CharsetToEncoding(const wxString& charset, bool interactive)
{
    // try the ways not needing the users intervention first
    int encoding = wxFontMapperBase::NonInteractiveCharsetToEncoding(charset);

    // if we failed to find the encoding, ask the user -- unless disabled
    if ( encoding == wxFONTENCODING_UNKNOWN )
    {
        // this is the special value which disables asking the user (he had
        // chosen to suppress this the last time)
        encoding = wxFONTENCODING_SYSTEM;
    }
#if wxUSE_CHOICEDLG
    else if ( (encoding == wxFONTENCODING_SYSTEM) && interactive )
    {
        // prepare the dialog data

        // the dialog title
        wxString title(m_titleDialog);
        if ( !title )
            title << wxTheApp->GetAppName() << _(": unknown charset");

        // the message
        wxString msg;
        msg.Printf(_("The charset '%s' is unknown. You may select\nanother charset to replace it with or choose\n[Cancel] if it cannot be replaced"), charset.c_str());

        // the list of choices
        const size_t count = GetSupportedEncodingsCount();

        wxString *encodingNamesTranslated = new wxString[count];

        for ( size_t i = 0; i < count; i++ )
        {
            encodingNamesTranslated[i] = GetEncodingDescription(GetEncoding(i));
        }

        // the parent window
        wxWindow *parent = m_windowParent;
        if ( !parent )
            parent = wxTheApp->GetTopWindow();

        // do ask the user and get back the index in encodings table
        int n = wxGetSingleChoiceIndex(msg, title,
                                       count,
                                       encodingNamesTranslated,
                                       parent);

        delete [] encodingNamesTranslated;

        if ( n != -1 )
        {
            encoding = GetEncoding(n);
        }

#if wxUSE_CONFIG && wxUSE_FILECONFIG
        // save the result in the config now
        wxFontMapperPathChanger path(this, FONTMAPPER_CHARSET_PATH);
        if ( path.IsOk() )
        {
            wxConfigBase *config = GetConfig();

            // remember the alt encoding for this charset -- or remember that
            // we don't know it
            long value = n == -1 ? (long)wxFONTENCODING_UNKNOWN : (long)encoding;
            if ( !config->Write(charset, value) )
            {
                wxLogError(_("Failed to remember the encoding for the charset '%s'."), charset.c_str());
            }
        }
#endif // wxUSE_CONFIG
    }
#else
    wxUnusedVar(interactive);
#endif // wxUSE_CHOICEDLG

    return (wxFontEncoding)encoding;
}

// ----------------------------------------------------------------------------
// support for unknown encodings: we maintain a map between the
// (platform-specific) strings identifying them and our wxFontEncodings they
// correspond to which is used by GetFontForEncoding() function
// ----------------------------------------------------------------------------

bool wxFontMapper::TestAltEncoding(const wxString& configEntry,
                                   wxFontEncoding encReplacement,
                                   wxNativeEncodingInfo *info)
{
    if ( wxGetNativeFontEncoding(encReplacement, info) &&
         wxTestFontEncoding(*info) )
    {
#if wxUSE_CONFIG && wxUSE_FILECONFIG
        // remember the mapping in the config
        wxFontMapperPathChanger path(this, FONTMAPPER_FONT_FROM_ENCODING_PATH);

        if ( path.IsOk() )
        {
            GetConfig()->Write(configEntry, info->ToString());
        }
#else
        wxUnusedVar(configEntry);
#endif // wxUSE_CONFIG
        return true;
    }

    return false;
}

bool wxFontMapper::GetAltForEncoding(wxFontEncoding encoding,
                                     wxNativeEncodingInfo *info,
                                     const wxString& facename,
                                     bool interactive)
{
#if wxUSE_GUI
    // we need a flag to prevent infinite recursion which happens, for
    // example, when GetAltForEncoding() is called from an OnPaint() handler:
    // in this case, wxYield() which is called from wxMessageBox() we use here
    // will lead to another call of OnPaint() and hence to another call of
    // GetAltForEncoding() -- and it is impossible to catch this from the user
    // code because we are called from wxFont ctor implicitly.

    // assume we're always called from the main thread, so that it is safe to
    // use a static var
    static bool s_inGetAltForEncoding = false;

    if ( interactive && s_inGetAltForEncoding )
        return false;

    ReentrancyBlocker blocker(s_inGetAltForEncoding);
#endif // wxUSE_GUI

    wxCHECK_MSG( info, false, wxT("bad pointer in GetAltForEncoding") );

    info->facename = facename;

    if ( encoding == wxFONTENCODING_DEFAULT )
    {
        encoding = wxFont::GetDefaultEncoding();
    }

    // if we failed to load the system default encoding, something is really
    // wrong and we'd better stop now -- otherwise we will go into endless
    // recursion trying to create the font in the msg box with the error
    // message
    if ( encoding == wxFONTENCODING_SYSTEM )
    {
        wxLogFatalError(_("can't load any font, aborting"));

        // wxLogFatalError doesn't return
    }

    wxString configEntry,
             encName = GetEncodingName(encoding);
    if ( !facename.empty() )
    {
        configEntry = facename + _T("_");
    }
    configEntry += encName;

#if wxUSE_CONFIG && wxUSE_FILECONFIG
    // do we have a font spec for this encoding?
    wxString fontinfo;
    wxFontMapperPathChanger path(this, FONTMAPPER_FONT_FROM_ENCODING_PATH);
    if ( path.IsOk() )
    {
        fontinfo = GetConfig()->Read(configEntry);
    }

    // this special value means that we don't know of fonts for this
    // encoding but, moreover, have already asked the user as well and he
    // didn't specify any font neither
    if ( fontinfo == FONTMAPPER_FONT_DONT_ASK )
    {
        interactive = false;
    }
    else // use the info entered the last time
    {
        if ( !fontinfo.empty() && !facename.empty() )
        {
            // we tried to find a match with facename -- now try without it
            fontinfo = GetConfig()->Read(encName);
        }

        if ( !fontinfo.empty() )
        {
            if ( info->FromString(fontinfo) )
            {
                if ( wxTestFontEncoding(*info) )
                {
                    // ok, got something
                    return true;
                }
                //else: no such fonts, look for something else
                //      (should we erase the outdated value?)
            }
            else
            {
                wxLogDebug(wxT("corrupted config data: string '%s' is not a valid font encoding info"),
                           fontinfo.c_str());
            }
        }
        //else: there is no information in config about this encoding
    }
#endif // wxUSE_CONFIG

    // now try to map this encoding to a compatible one which we have on this
    // system
    wxFontEncodingArray equiv = wxEncodingConverter::GetAllEquivalents(encoding);
    size_t count = equiv.GetCount();
    bool foundEquivEncoding = false;
    wxFontEncoding equivEncoding = wxFONTENCODING_SYSTEM;
    if ( count )
    {
        for ( size_t i = 0; i < count && !foundEquivEncoding; i++ )
        {
            // don't test for encoding itself, we already know we don't have it
            if ( equiv[i] == encoding )
                continue;

            if ( TestAltEncoding(configEntry, equiv[i], info) )
            {
                equivEncoding = equiv[i];

                foundEquivEncoding = true;
            }
        }
    }

    // ask the user
#if wxUSE_FONTDLG
    if ( interactive )
    {
        wxString title(m_titleDialog);
        if ( !title )
            title << wxTheApp->GetAppName() << _(": unknown encoding");

        // built the message
        wxString encDesc = GetEncodingDescription(encoding),
                 msg;
        if ( foundEquivEncoding )
        {
            // ask the user if he wants to override found alternative encoding
            msg.Printf(_("No font for displaying text in encoding '%s' found,\nbut an alternative encoding '%s' is available.\nDo you want to use this encoding (otherwise you will have to choose another one)?"),
                       encDesc.c_str(), GetEncodingDescription(equivEncoding).c_str());
        }
        else
        {
            msg.Printf(_("No font for displaying text in encoding '%s' found.\nWould you like to select a font to be used for this encoding\n(otherwise the text in this encoding will not be shown correctly)?"),
                       encDesc.c_str());
        }

        // the question is different in 2 cases so the answer has to be
        // interpreted differently as well
        int answer = foundEquivEncoding ? wxNO : wxYES;

        if ( wxMessageBox(msg, title,
                          wxICON_QUESTION | wxYES_NO,
                          m_windowParent) == answer )
        {
            wxFontData data;
            data.SetEncoding(encoding);
            data.EncodingInfo() = *info;
            wxFontDialog dialog(m_windowParent, data);
            if ( dialog.ShowModal() == wxID_OK )
            {
                wxFontData retData = dialog.GetFontData();

                *info = retData.EncodingInfo();
                info->encoding = retData.GetEncoding();

#if wxUSE_CONFIG && wxUSE_FILECONFIG
                // remember this in the config
                wxFontMapperPathChanger path2(this,
                                              FONTMAPPER_FONT_FROM_ENCODING_PATH);
                if ( path2.IsOk() )
                {
                    GetConfig()->Write(configEntry, info->ToString());
                }
#endif // wxUSE_CONFIG

                return true;
            }
            //else: the user canceled the font selection dialog
        }
        else
        {
            // the user doesn't want to select a font for this encoding
            // or selected to use equivalent encoding
            //
            // remember it to avoid asking the same question again later
#if wxUSE_CONFIG && wxUSE_FILECONFIG
            wxFontMapperPathChanger path2(this,
                                          FONTMAPPER_FONT_FROM_ENCODING_PATH);
            if ( path2.IsOk() )
            {
                GetConfig()->Write
                             (
                                configEntry,
                                foundEquivEncoding ? info->ToString().c_str()
                                                   : FONTMAPPER_FONT_DONT_ASK
                             );
            }
#endif // wxUSE_CONFIG
        }
    }
    //else: we're in non-interactive mode
#else
    wxUnusedVar(equivEncoding);
#endif // wxUSE_FONTDLG

    return foundEquivEncoding;
}

bool wxFontMapper::GetAltForEncoding(wxFontEncoding encoding,
                                     wxFontEncoding *encodingAlt,
                                     const wxString& facename,
                                     bool interactive)
{
    wxCHECK_MSG( encodingAlt, false,
                    _T("wxFontEncoding::GetAltForEncoding(): NULL pointer") );

    wxNativeEncodingInfo info;
    if ( !GetAltForEncoding(encoding, &info, facename, interactive) )
        return false;

    *encodingAlt = info.encoding;

    return true;
}

bool wxFontMapper::IsEncodingAvailable(wxFontEncoding encoding,
                                       const wxString& facename)
{
    wxNativeEncodingInfo info;

    if ( !wxGetNativeFontEncoding(encoding, &info) )
        return false;

    info.facename = facename;
    return wxTestFontEncoding(info);
}

#endif // wxUSE_FONTMAP
