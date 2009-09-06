/////////////////////////////////////////////////////////////////////////////
// Name:        src/msw/font.cpp
// Purpose:     wxFont class
// Author:      Julian Smart
// Modified by:
// Created:     01/02/97
// RCS-ID:      $Id: font.cpp 48064 2007-08-14 08:51:44Z JS $
// Copyright:   (c) wxWidgets team
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

#include "wx/font.h"

#ifndef WX_PRECOMP
    #include "wx/list.h"
    #include "wx/utils.h"
    #include "wx/app.h"
    #include "wx/log.h"
    #include "wx/encinfo.h"
#endif // WX_PRECOMP

#include "wx/msw/private.h"

#include "wx/fontutil.h"
#include "wx/fontmap.h"

#ifndef __WXWINCE__
    #include "wx/sysopt.h"
#endif

#include "wx/tokenzr.h"

#if wxUSE_EXTENDED_RTTI

wxBEGIN_ENUM( wxFontFamily )
    wxENUM_MEMBER( wxDEFAULT )
    wxENUM_MEMBER( wxDECORATIVE )
    wxENUM_MEMBER( wxROMAN )
    wxENUM_MEMBER( wxSCRIPT )
    wxENUM_MEMBER( wxSWISS )
    wxENUM_MEMBER( wxMODERN )
    wxENUM_MEMBER( wxTELETYPE )
wxEND_ENUM( wxFontFamily )

wxBEGIN_ENUM( wxFontStyle )
    wxENUM_MEMBER( wxNORMAL )
    wxENUM_MEMBER( wxITALIC )
    wxENUM_MEMBER( wxSLANT )
wxEND_ENUM( wxFontStyle )

wxBEGIN_ENUM( wxFontWeight )
    wxENUM_MEMBER( wxNORMAL )
    wxENUM_MEMBER( wxLIGHT )
    wxENUM_MEMBER( wxBOLD )
wxEND_ENUM( wxFontWeight )

IMPLEMENT_DYNAMIC_CLASS_WITH_COPY_XTI(wxFont, wxGDIObject,"wx/font.h")

wxBEGIN_PROPERTIES_TABLE(wxFont)
    wxPROPERTY( Size,int, SetPointSize, GetPointSize, 12 , 0 /*flags*/ , wxT("Helpstring") , wxT("group"))
    wxPROPERTY( Family, int  , SetFamily, GetFamily, (int)wxDEFAULT , 0 /*flags*/ , wxT("Helpstring") , wxT("group")) // wxFontFamily
    wxPROPERTY( Style, int , SetStyle, GetStyle, (int)wxNORMAL , 0 /*flags*/ , wxT("Helpstring") , wxT("group")) // wxFontStyle
    wxPROPERTY( Weight, int , SetWeight, GetWeight, (int)wxNORMAL , 0 /*flags*/ , wxT("Helpstring") , wxT("group")) // wxFontWeight
    wxPROPERTY( Underlined, bool , SetUnderlined, GetUnderlined, false , 0 /*flags*/ , wxT("Helpstring") , wxT("group"))
    wxPROPERTY( Face, wxString , SetFaceName, GetFaceName, EMPTY_MACROVALUE , 0 /*flags*/ , wxT("Helpstring") , wxT("group"))
    wxPROPERTY( Encoding, wxFontEncoding , SetEncoding, GetEncoding, wxFONTENCODING_DEFAULT , 0 /*flags*/ , wxT("Helpstring") , wxT("group"))
wxEND_PROPERTIES_TABLE()

wxCONSTRUCTOR_6( wxFont , int , Size , int , Family , int , Style , int , Weight , bool , Underlined , wxString , Face )

wxBEGIN_HANDLERS_TABLE(wxFont)
wxEND_HANDLERS_TABLE()

#else
    IMPLEMENT_DYNAMIC_CLASS(wxFont, wxGDIObject)
#endif


// ----------------------------------------------------------------------------
// constants
// ----------------------------------------------------------------------------

// the mask used to extract the pitch from LOGFONT::lfPitchAndFamily field
static const int PITCH_MASK = FIXED_PITCH | VARIABLE_PITCH;

// ----------------------------------------------------------------------------
// wxFontRefData - the internal description of the font
// ----------------------------------------------------------------------------

class WXDLLEXPORT wxFontRefData: public wxGDIRefData
{
public:
    // constructors
    wxFontRefData()
    {
        Init(-1, wxSize(0,0), false, wxFONTFAMILY_DEFAULT, wxFONTSTYLE_NORMAL,
             wxFONTWEIGHT_NORMAL, false, wxEmptyString,
             wxFONTENCODING_DEFAULT);
    }

    wxFontRefData(int size,
                  const wxSize& pixelSize,
                  bool sizeUsingPixels,
                  int family,
                  int style,
                  int weight,
                  bool underlined,
                  const wxString& faceName,
                  wxFontEncoding encoding)
    {
        Init(size, pixelSize, sizeUsingPixels, family, style, weight,
             underlined, faceName, encoding);
    }

    wxFontRefData(const wxNativeFontInfo& info, WXHFONT hFont = 0)
    {
        Init(info, hFont);
    }

    wxFontRefData(const wxFontRefData& data) : wxGDIRefData()
    {
        if ( data.m_nativeFontInfoOk )
        {
            Init(data.m_nativeFontInfo);
        }
        else
        {
            Init(data.m_pointSize, data.m_pixelSize, data.m_sizeUsingPixels,
                 data.m_family, data.m_style, data.m_weight,
                 data.m_underlined, data.m_faceName, data.m_encoding);
        }
    }

    virtual ~wxFontRefData();

    // operations
    bool Alloc(wxFont *font);

    void Free();

    // all wxFont accessors
    int GetPointSize() const
    {
        return m_nativeFontInfoOk ? m_nativeFontInfo.GetPointSize()
                                  : m_pointSize;
    }

    wxSize GetPixelSize() const
    {
        return m_nativeFontInfoOk ? m_nativeFontInfo.GetPixelSize()
                                  : m_pixelSize;
    }

    bool IsUsingSizeInPixels() const
    {
        return m_nativeFontInfoOk ? true : m_sizeUsingPixels;
    }

    int GetFamily() const
    {
        return m_family;
    }

    int GetStyle() const
    {
        return m_nativeFontInfoOk ? m_nativeFontInfo.GetStyle()
                                  : m_style;
    }

    int GetWeight() const
    {
        return m_nativeFontInfoOk ? m_nativeFontInfo.GetWeight()
                                  : m_weight;
    }

    bool GetUnderlined() const
    {
        return m_nativeFontInfoOk ? m_nativeFontInfo.GetUnderlined()
                                  : m_underlined;
    }

    wxString GetFaceName() const
    {
        wxString s;
        if ( m_nativeFontInfoOk )
            s = m_nativeFontInfo.GetFaceName();
        else
            s = m_faceName;

        return s;
    }

    wxFontEncoding GetEncoding() const
    {
        return m_nativeFontInfoOk ? m_nativeFontInfo.GetEncoding()
                                  : m_encoding;
    }

    WXHFONT GetHFONT() const { return m_hFont; }

    // ... and setters
    void SetPointSize(int pointSize)
    {
        if ( m_nativeFontInfoOk )
        {
            m_nativeFontInfo.SetPointSize(pointSize);
        }
        else
        {
            m_pointSize = pointSize;
            m_sizeUsingPixels = false;
        }
    }

    void SetPixelSize(const wxSize& pixelSize)
    {
        if ( m_nativeFontInfoOk )
        {
            m_nativeFontInfo.SetPixelSize(pixelSize);
        }
        else
        {
            m_pixelSize = pixelSize;
            m_sizeUsingPixels = true;
        }
    }

    void SetFamily(int family)
    {
        m_family = family;
    }

    void SetStyle(int style)
    {
        if ( m_nativeFontInfoOk )
            m_nativeFontInfo.SetStyle((wxFontStyle)style);
        else
            m_style = style;
    }

    void SetWeight(int weight)
    {
        if ( m_nativeFontInfoOk )
            m_nativeFontInfo.SetWeight((wxFontWeight)weight);
        else
            m_weight = weight;
    }

    bool SetFaceName(const wxString& faceName)
    {
        if ( m_nativeFontInfoOk )
            return m_nativeFontInfo.SetFaceName(faceName);

        m_faceName = faceName;
        return true;
    }

    void SetUnderlined(bool underlined)
    {
        if ( m_nativeFontInfoOk )
            m_nativeFontInfo.SetUnderlined(underlined);
        else
            m_underlined = underlined;
    }

    void SetEncoding(wxFontEncoding encoding)
    {
        if ( m_nativeFontInfoOk )
            m_nativeFontInfo.SetEncoding(encoding);
        else
            m_encoding = encoding;
    }

    // native font info tests
    bool HasNativeFontInfo() const { return m_nativeFontInfoOk; }

    const wxNativeFontInfo& GetNativeFontInfo() const
        { return m_nativeFontInfo; }

protected:
    // common part of all ctors
    void Init(int size,
              const wxSize& pixelSize,
              bool sizeUsingPixels,
              int family,
              int style,
              int weight,
              bool underlined,
              const wxString& faceName,
              wxFontEncoding encoding);

    void Init(const wxNativeFontInfo& info, WXHFONT hFont = 0);

    // font characterstics
    int           m_pointSize;
    wxSize        m_pixelSize;
    bool          m_sizeUsingPixels;
    int           m_family;
    int           m_style;
    int           m_weight;
    bool          m_underlined;
    wxString      m_faceName;
    wxFontEncoding m_encoding;

    // Windows font handle
    WXHFONT       m_hFont;

    // Native font info
    wxNativeFontInfo m_nativeFontInfo;
    bool             m_nativeFontInfoOk;
};

#define M_FONTDATA ((wxFontRefData*)m_refData)

// ============================================================================
// implementation
// ============================================================================

// ----------------------------------------------------------------------------
// wxFontRefData
// ----------------------------------------------------------------------------

void wxFontRefData::Init(int pointSize,
                         const wxSize& pixelSize,
                         bool sizeUsingPixels,
                         int family,
                         int style,
                         int weight,
                         bool underlined,
                         const wxString& faceName,
                         wxFontEncoding encoding)
{
    m_style = style;
    m_pointSize = pointSize == -1 ? wxNORMAL_FONT->GetPointSize() : pointSize;
    m_pixelSize = pixelSize;
    m_sizeUsingPixels = sizeUsingPixels;
    m_family = family;
    m_style = style;
    m_weight = weight;
    m_underlined = underlined;
    m_faceName = faceName;
    m_encoding = encoding;

    m_hFont = 0;

    m_nativeFontInfoOk = false;
}

void wxFontRefData::Init(const wxNativeFontInfo& info, WXHFONT hFont)
{
    // hFont may be zero, or it be passed in case we really want to
    // use the exact font created in the underlying system
    // (for example where we can't guarantee conversion from HFONT
    // to LOGFONT back to HFONT)
    m_hFont = hFont;

    m_nativeFontInfoOk = true;
    m_nativeFontInfo = info;
    // This is the best we can do since we don't have the
    // correct information at this point.
    m_family = wxSWISS;
}

wxFontRefData::~wxFontRefData()
{
    Free();
}

bool wxFontRefData::Alloc(wxFont *font)
{
    if ( !m_nativeFontInfoOk )
    {
        wxFillLogFont(&m_nativeFontInfo.lf, font);
        m_nativeFontInfoOk = true;
    }

    HFONT hfont = ::CreateFontIndirect(&m_nativeFontInfo.lf);
    if ( !hfont )
    {
        wxLogLastError(wxT("CreateFont"));
        return false;
    }

    m_hFont = (WXHFONT)hfont;
    return true;
}

void wxFontRefData::Free()
{
    if ( m_hFont )
    {
        if ( !::DeleteObject((HFONT) m_hFont) )
        {
            wxLogLastError(wxT("DeleteObject(font)"));
        }

        m_hFont = 0;
    }
}

// ----------------------------------------------------------------------------
// wxNativeFontInfo
// ----------------------------------------------------------------------------

void wxNativeFontInfo::Init()
{
    wxZeroMemory(lf);

    // we get better font quality if we use PROOF_QUALITY instead of
    // DEFAULT_QUALITY but some fonts (e.g. "Terminal 6pt") are not available
    // then so we allow to set a global option to choose between quality and
    // wider font selection
#ifdef __WXWINCE__
    lf.lfQuality = CLEARTYPE_QUALITY;
#else
    lf.lfQuality = wxSystemOptions::GetOptionInt(_T("msw.font.no-proof-quality"))
                    ? DEFAULT_QUALITY
                    : PROOF_QUALITY;
#endif
}

int wxNativeFontInfo::GetPointSize() const
{
    // FIXME: using the screen here results in incorrect font size calculation
    //        for printing!
    const int ppInch = ::GetDeviceCaps(ScreenHDC(), LOGPIXELSY);

    return (int) (((72.0*(double)abs((int) lf.lfHeight)) / (double) ppInch) + 0.5);
}

wxSize wxNativeFontInfo::GetPixelSize() const
{
    wxSize ret;
    ret.SetHeight(lf.lfHeight);
    ret.SetWidth(lf.lfWidth);
    return ret;
}

wxFontStyle wxNativeFontInfo::GetStyle() const
{
    return lf.lfItalic ? wxFONTSTYLE_ITALIC : wxFONTSTYLE_NORMAL;
}

wxFontWeight wxNativeFontInfo::GetWeight() const
{
    if ( lf.lfWeight <= 300 )
        return wxFONTWEIGHT_LIGHT;

    if ( lf.lfWeight >= 600 )
        return wxFONTWEIGHT_BOLD;

    return wxFONTWEIGHT_NORMAL;
}

bool wxNativeFontInfo::GetUnderlined() const
{
    return lf.lfUnderline != 0;
}

wxString wxNativeFontInfo::GetFaceName() const
{
    return lf.lfFaceName;
}

wxFontFamily wxNativeFontInfo::GetFamily() const
{
    wxFontFamily family;

    // extract family from pitch-and-family
    switch ( lf.lfPitchAndFamily & ~PITCH_MASK )
    {
        case FF_ROMAN:
            family = wxFONTFAMILY_ROMAN;
            break;

        default:
            wxFAIL_MSG( _T("unknown LOGFONT::lfFamily value") );
            // fall through

        case FF_SWISS:
            family = wxFONTFAMILY_SWISS;
            break;

        case FF_SCRIPT:
            family = wxFONTFAMILY_SCRIPT;
            break;

        case FF_MODERN:
            family = wxFONTFAMILY_MODERN;
            break;

        case FF_DECORATIVE:
            family = wxFONTFAMILY_DECORATIVE;
            break;
    }

    return family;
}

wxFontEncoding wxNativeFontInfo::GetEncoding() const
{
    return wxGetFontEncFromCharSet(lf.lfCharSet);
}

void wxNativeFontInfo::SetPointSize(int pointsize)
{
    // FIXME: using the screen here results in incorrect font size calculation
    //        for printing!
    const int ppInch = ::GetDeviceCaps(ScreenHDC(), LOGPIXELSY);

    lf.lfHeight = -(int)((pointsize*((double)ppInch)/72.0) + 0.5);
}

void wxNativeFontInfo::SetPixelSize(const wxSize& pixelSize)
{
    lf.lfHeight = pixelSize.GetHeight();
    lf.lfWidth = pixelSize.GetWidth();
}


void wxNativeFontInfo::SetStyle(wxFontStyle style)
{
    switch ( style )
    {
        default:
            wxFAIL_MSG( _T("unknown font style") );
            // fall through

        case wxFONTSTYLE_NORMAL:
            lf.lfItalic = FALSE;
            break;

        case wxFONTSTYLE_ITALIC:
        case wxFONTSTYLE_SLANT:
            lf.lfItalic = TRUE;
            break;
    }
}

void wxNativeFontInfo::SetWeight(wxFontWeight weight)
{
    switch ( weight )
    {
        default:
            wxFAIL_MSG( _T("unknown font weight") );
            // fall through

        case wxFONTWEIGHT_NORMAL:
            lf.lfWeight = FW_NORMAL;
            break;

        case wxFONTWEIGHT_LIGHT:
            lf.lfWeight = FW_LIGHT;
            break;

        case wxFONTWEIGHT_BOLD:
            lf.lfWeight = FW_BOLD;
            break;
    }
}

void wxNativeFontInfo::SetUnderlined(bool underlined)
{
    lf.lfUnderline = underlined;
}

bool wxNativeFontInfo::SetFaceName(const wxString& facename)
{
    size_t len = WXSIZEOF(lf.lfFaceName);
    wxStrncpy(lf.lfFaceName, facename, len);
    lf.lfFaceName[len - 1] = '\0';    // truncate the face name
    return true;
}

void wxNativeFontInfo::SetFamily(wxFontFamily family)
{
    BYTE ff_family;
    wxArrayString facename;

    // the list of fonts associated with a family was partially
    // taken from http://www.codestyle.org/css/font-family

    switch ( family )
    {
        case wxSCRIPT:
            ff_family = FF_SCRIPT;
            facename.Add(_T("Script"));
            facename.Add(_T("Brush Script MT"));
            facename.Add(_T("Comic Sans MS"));
            facename.Add(_T("Lucida Handwriting"));
            break;

        case wxDECORATIVE:
            ff_family = FF_DECORATIVE;
            facename.Add(_T("Old English Text MT"));
            facename.Add(_T("Comic Sans MS"));
            facename.Add(_T("Lucida Handwriting"));
            break;

        case wxROMAN:
            ff_family = FF_ROMAN;
            facename.Add(_T("Times New Roman"));
            facename.Add(_T("Georgia"));
            facename.Add(_T("Garamond"));
            facename.Add(_T("Bookman Old Style"));
            facename.Add(_T("Book Antiqua"));
            break;

        case wxTELETYPE:
        case wxMODERN:
            ff_family = FF_MODERN;
            facename.Add(_T("Courier New"));
            facename.Add(_T("Lucida Console"));
            facename.Add(_T("Andale Mono"));
            facename.Add(_T("OCR A Extended"));
            facename.Add(_T("Terminal"));
            break;

        case wxSWISS:
            ff_family = FF_SWISS;
            facename.Add(_T("Arial"));
            facename.Add(_T("Century Gothic"));
            facename.Add(_T("Lucida Sans Unicode"));
            facename.Add(_T("Tahoma"));
            facename.Add(_T("Trebuchet MS"));
            facename.Add(_T("Verdana"));
            break;

        case wxDEFAULT:
        default:
        {
            // We want Windows 2000 or later to have new fonts even MS Shell Dlg
            // is returned as default GUI font for compatibility
            int verMaj;
            ff_family = FF_SWISS;
            if(wxGetOsVersion(&verMaj) == wxOS_WINDOWS_NT && verMaj >= 5)
                facename.Add(_T("MS Shell Dlg 2"));
            else
                facename.Add(_T("MS Shell Dlg"));

            // Quoting the MSDN:
            //     "MS Shell Dlg is a mapping mechanism that enables
            //     U.S. English Microsoft Windows NT, and Microsoft Windows 2000 to
            //     support locales that have characters that are not contained in code
            //     page 1252. It is not a font but a face name for a nonexistent font."
        }
    }

    lf.lfPitchAndFamily = (BYTE)(DEFAULT_PITCH) | ff_family;

    if ( !wxStrlen(lf.lfFaceName) )
    {
        SetFaceName(facename);
    }
}

void wxNativeFontInfo::SetEncoding(wxFontEncoding encoding)
{
    wxNativeEncodingInfo info;
    if ( !wxGetNativeFontEncoding(encoding, &info) )
    {
#if wxUSE_FONTMAP
        if ( wxFontMapper::Get()->GetAltForEncoding(encoding, &info) )
        {
            if ( !info.facename.empty() )
            {
                // if we have this encoding only in some particular facename, use
                // the facename - it is better to show the correct characters in a
                // wrong facename than unreadable text in a correct one
                SetFaceName(info.facename);
            }
        }
        else
#endif // wxUSE_FONTMAP
        {
            // unsupported encoding, replace with the default
            info.charset = DEFAULT_CHARSET;
        }
    }

    lf.lfCharSet = (BYTE)info.charset;
}

bool wxNativeFontInfo::FromString(const wxString& s)
{
    long l;

    wxStringTokenizer tokenizer(s, _T(";"));

    // first the version
    wxString token = tokenizer.GetNextToken();
    if ( token != _T('0') )
        return false;

    token = tokenizer.GetNextToken();
    if ( !token.ToLong(&l) )
        return false;
    lf.lfHeight = l;

    token = tokenizer.GetNextToken();
    if ( !token.ToLong(&l) )
        return false;
    lf.lfWidth = l;

    token = tokenizer.GetNextToken();
    if ( !token.ToLong(&l) )
        return false;
    lf.lfEscapement = l;

    token = tokenizer.GetNextToken();
    if ( !token.ToLong(&l) )
        return false;
    lf.lfOrientation = l;

    token = tokenizer.GetNextToken();
    if ( !token.ToLong(&l) )
        return false;
    lf.lfWeight = l;

    token = tokenizer.GetNextToken();
    if ( !token.ToLong(&l) )
        return false;
    lf.lfItalic = (BYTE)l;

    token = tokenizer.GetNextToken();
    if ( !token.ToLong(&l) )
        return false;
    lf.lfUnderline = (BYTE)l;

    token = tokenizer.GetNextToken();
    if ( !token.ToLong(&l) )
        return false;
    lf.lfStrikeOut = (BYTE)l;

    token = tokenizer.GetNextToken();
    if ( !token.ToLong(&l) )
        return false;
    lf.lfCharSet = (BYTE)l;

    token = tokenizer.GetNextToken();
    if ( !token.ToLong(&l) )
        return false;
    lf.lfOutPrecision = (BYTE)l;

    token = tokenizer.GetNextToken();
    if ( !token.ToLong(&l) )
        return false;
    lf.lfClipPrecision = (BYTE)l;

    token = tokenizer.GetNextToken();
    if ( !token.ToLong(&l) )
        return false;
    lf.lfQuality = (BYTE)l;

    token = tokenizer.GetNextToken();
    if ( !token.ToLong(&l) )
        return false;
    lf.lfPitchAndFamily = (BYTE)l;

    token = tokenizer.GetNextToken();
    if(!token)
        return false;
    wxStrcpy(lf.lfFaceName, token.c_str());

    return true;
}

wxString wxNativeFontInfo::ToString() const
{
    wxString s;

    s.Printf(_T("%d;%ld;%ld;%ld;%ld;%ld;%d;%d;%d;%d;%d;%d;%d;%d;%s"),
             0, // version, in case we want to change the format later
             lf.lfHeight,
             lf.lfWidth,
             lf.lfEscapement,
             lf.lfOrientation,
             lf.lfWeight,
             lf.lfItalic,
             lf.lfUnderline,
             lf.lfStrikeOut,
             lf.lfCharSet,
             lf.lfOutPrecision,
             lf.lfClipPrecision,
             lf.lfQuality,
             lf.lfPitchAndFamily,
             lf.lfFaceName);

    return s;
}

// ----------------------------------------------------------------------------
// wxFont
// ----------------------------------------------------------------------------

bool wxFont::Create(const wxNativeFontInfo& info, WXHFONT hFont)
{
    UnRef();

    m_refData = new wxFontRefData(info, hFont);

    RealizeResource();

    return true;
}

wxFont::wxFont(const wxString& fontdesc)
{
    wxNativeFontInfo info;
    if ( info.FromString(fontdesc) )
        (void)Create(info);
}

/* Constructor for a font. Note that the real construction is done
 * in wxDC::SetFont, when information is available about scaling etc.
 */
bool wxFont::DoCreate(int pointSize,
                      const wxSize& pixelSize,
                      bool sizeUsingPixels,
                      int family,
                      int style,
                      int weight,
                      bool underlined,
                      const wxString& faceName,
                      wxFontEncoding encoding)
{
    UnRef();

    // wxDEFAULT is a valid value for the font size too so we must treat it
    // specially here (otherwise the size would be 70 == wxDEFAULT value)
    if ( pointSize == wxDEFAULT )
    {
        pointSize = wxNORMAL_FONT->GetPointSize();
    }

    m_refData = new wxFontRefData(pointSize, pixelSize, sizeUsingPixels,
                                  family, style, weight,
                                  underlined, faceName, encoding);

    RealizeResource();

    return true;
}

wxFont::~wxFont()
{
}

// ----------------------------------------------------------------------------
// real implementation
// ----------------------------------------------------------------------------

bool wxFont::RealizeResource()
{
    if ( GetResourceHandle() )
    {
        // VZ: the old code returned false in this case, but it doesn't seem
        //     to make sense because the font _was_ created
        return true;
    }

    return M_FONTDATA->Alloc(this);
}

bool wxFont::FreeResource(bool WXUNUSED(force))
{
    if ( GetResourceHandle() )
    {
        M_FONTDATA->Free();

        return true;
    }

    return false;
}

WXHANDLE wxFont::GetResourceHandle() const
{
    return (WXHANDLE)GetHFONT();
}

WXHFONT wxFont::GetHFONT() const
{
    return M_FONTDATA ? M_FONTDATA->GetHFONT() : 0;
}

bool wxFont::IsFree() const
{
    return M_FONTDATA && (M_FONTDATA->GetHFONT() == 0);
}

void wxFont::Unshare()
{
    // Don't change shared data
    if ( !m_refData )
    {
        m_refData = new wxFontRefData();
    }
    else
    {
        wxFontRefData* ref = new wxFontRefData(*M_FONTDATA);
        UnRef();
        m_refData = ref;
    }
}

// ----------------------------------------------------------------------------
// change font attribute: we recreate font when doing it
// ----------------------------------------------------------------------------

void wxFont::SetPointSize(int pointSize)
{
    Unshare();

    M_FONTDATA->SetPointSize(pointSize);

    RealizeResource();
}

void wxFont::SetPixelSize(const wxSize& pixelSize)
{
    Unshare();

    M_FONTDATA->SetPixelSize(pixelSize);

    RealizeResource();
}

void wxFont::SetFamily(int family)
{
    Unshare();

    M_FONTDATA->SetFamily(family);

    RealizeResource();
}

void wxFont::SetStyle(int style)
{
    Unshare();

    M_FONTDATA->SetStyle(style);

    RealizeResource();
}

void wxFont::SetWeight(int weight)
{
    Unshare();

    M_FONTDATA->SetWeight(weight);

    RealizeResource();
}

bool wxFont::SetFaceName(const wxString& faceName)
{
    Unshare();

    bool refdataok = M_FONTDATA->SetFaceName(faceName);

    RealizeResource();

    // NB: using win32's GetObject() API on M_FONTDATA->GetHFONT()
    //     to retrieve a LOGFONT and then compare lf.lfFaceName
    //     with given facename is not reliable at all:
    //     Windows copies the facename given to ::CreateFontIndirect()
    //     without any validity check.
    //     Thus we use wxFontBase::SetFaceName to check if facename
    //     is valid...
    return refdataok && wxFontBase::SetFaceName(faceName);
}

void wxFont::SetUnderlined(bool underlined)
{
    Unshare();

    M_FONTDATA->SetUnderlined(underlined);

    RealizeResource();
}

void wxFont::SetEncoding(wxFontEncoding encoding)
{
    Unshare();

    M_FONTDATA->SetEncoding(encoding);

    RealizeResource();
}

void wxFont::DoSetNativeFontInfo(const wxNativeFontInfo& info)
{
    Unshare();

    FreeResource();

    *M_FONTDATA = wxFontRefData(info);

    RealizeResource();
}

// ----------------------------------------------------------------------------
// accessors
// ----------------------------------------------------------------------------

int wxFont::GetPointSize() const
{
    wxCHECK_MSG( Ok(), 0, wxT("invalid font") );

    return M_FONTDATA->GetPointSize();
}

wxSize wxFont::GetPixelSize() const
{
    wxCHECK_MSG( Ok(), wxDefaultSize, wxT("invalid font") );

    return M_FONTDATA->GetPixelSize();
}

bool wxFont::IsUsingSizeInPixels() const
{
    wxCHECK_MSG( Ok(), 0, wxT("invalid font") );

    return M_FONTDATA->IsUsingSizeInPixels();
}

int wxFont::GetFamily() const
{
    wxCHECK_MSG( Ok(), 0, wxT("invalid font") );

    return M_FONTDATA->GetFamily();
}

int wxFont::GetStyle() const
{
    wxCHECK_MSG( Ok(), 0, wxT("invalid font") );

    return M_FONTDATA->GetStyle();
}

int wxFont::GetWeight() const
{
    wxCHECK_MSG( Ok(), 0, wxT("invalid font") );

    return M_FONTDATA->GetWeight();
}

bool wxFont::GetUnderlined() const
{
    wxCHECK_MSG( Ok(), false, wxT("invalid font") );

    return M_FONTDATA->GetUnderlined();
}

wxString wxFont::GetFaceName() const
{
    wxCHECK_MSG( Ok(), wxEmptyString, wxT("invalid font") );

    return M_FONTDATA->GetFaceName();
}

wxFontEncoding wxFont::GetEncoding() const
{
    wxCHECK_MSG( Ok(), wxFONTENCODING_DEFAULT, wxT("invalid font") );

    return M_FONTDATA->GetEncoding();
}

const wxNativeFontInfo *wxFont::GetNativeFontInfo() const
{
    return Ok() && M_FONTDATA->HasNativeFontInfo() ? &(M_FONTDATA->GetNativeFontInfo())
                                           : NULL;
}

wxString wxFont::GetNativeFontInfoDesc() const
{
    wxCHECK_MSG( Ok(), wxEmptyString, wxT("invalid font") );

    // be sure we have an HFONT associated...
    wxConstCast(this, wxFont)->RealizeResource();
    return wxFontBase::GetNativeFontInfoDesc();
}

wxString wxFont::GetNativeFontInfoUserDesc() const
{
    wxCHECK_MSG( Ok(), wxEmptyString, wxT("invalid font") );

    // be sure we have an HFONT associated...
    wxConstCast(this, wxFont)->RealizeResource();
    return wxFontBase::GetNativeFontInfoUserDesc();
}

bool wxFont::IsFixedWidth() const
{
    if ( M_FONTDATA->HasNativeFontInfo() )
    {
        // the two low-order bits specify the pitch of the font, the rest is
        // family
        BYTE pitch =
            (BYTE)(M_FONTDATA->GetNativeFontInfo().lf.lfPitchAndFamily & PITCH_MASK);

        return pitch == FIXED_PITCH;
    }

    return wxFontBase::IsFixedWidth();
}
