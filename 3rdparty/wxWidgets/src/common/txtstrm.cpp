///////////////////////////////////////////////////////////////////////////////
// Name:        src/common/txtstrm.cpp
// Purpose:     Text stream classes
// Author:      Guilhem Lavaux
// Modified by:
// Created:     28/06/98
// RCS-ID:      $Id: txtstrm.cpp 48920 2007-09-24 13:11:36Z JS $
// Copyright:   (c) Guilhem Lavaux
// Licence:     wxWindows licence
/////////////////////////////////////////////////////////////////////////////

// For compilers that support precompilation, includes "wx.h".
#include "wx/wxprec.h"

#ifdef __BORLANDC__
  #pragma hdrstop
#endif

#if wxUSE_STREAMS

#include "wx/txtstrm.h"
#include <ctype.h>


// ----------------------------------------------------------------------------
// constants
// ----------------------------------------------------------------------------

// Unix: "\n"
// Dos:  "\r\n"
// Mac:  "\r"

// ----------------------------------------------------------------------------
// wxTextInputStream
// ----------------------------------------------------------------------------

#if wxUSE_UNICODE
wxTextInputStream::wxTextInputStream(wxInputStream &s,
                                     const wxString &sep,
                                     const wxMBConv& conv)
  : m_input(s), m_separators(sep), m_conv(conv.Clone())
{
    memset((void*)m_lastBytes, 0, 10);
}
#else
wxTextInputStream::wxTextInputStream(wxInputStream &s, const wxString &sep)
  : m_input(s), m_separators(sep)
{
    memset((void*)m_lastBytes, 0, 10);
}
#endif

wxTextInputStream::~wxTextInputStream()
{
#if wxUSE_UNICODE
    delete m_conv;
#endif // wxUSE_UNICODE
}

void wxTextInputStream::UngetLast()
{
    size_t byteCount = 0;
    while(m_lastBytes[byteCount]) // pseudo ANSI strlen (even for Unicode!)
        byteCount++;
    m_input.Ungetch(m_lastBytes, byteCount);
    memset((void*)m_lastBytes, 0, 10);
}

wxChar wxTextInputStream::NextChar()
{
#if wxUSE_UNICODE
    wxChar wbuf[2];
    memset((void*)m_lastBytes, 0, 10);
    for(size_t inlen = 0; inlen < 9; inlen++)
    {
        // actually read the next character
        m_lastBytes[inlen] = m_input.GetC();

        if(m_input.LastRead() <= 0)
            return wxEOT;

        if ( m_conv->ToWChar(wbuf, WXSIZEOF(wbuf), m_lastBytes, inlen + 1)
                != wxCONV_FAILED )
            return wbuf[0];
    }
    // there should be no encoding which requires more than nine bytes for one character...
    return wxEOT;
#else
    m_lastBytes[0] = m_input.GetC();

    if(m_input.LastRead() <= 0)
        return wxEOT;

    return m_lastBytes[0];
#endif

}

wxChar wxTextInputStream::NextNonSeparators()
{
    for (;;)
    {
        wxChar c = NextChar();
        if (c == wxEOT) return (wxChar) 0;

        if (c != wxT('\n') &&
            c != wxT('\r') &&
            !m_separators.Contains(c))
          return c;
    }

}

bool wxTextInputStream::EatEOL(const wxChar &c)
{
    if (c == wxT('\n')) return true; // eat on UNIX

    if (c == wxT('\r')) // eat on both Mac and DOS
    {
        wxChar c2 = NextChar();
        if(c2 == wxEOT) return true; // end of stream reached, had enough :-)

        if (c2 != wxT('\n')) UngetLast(); // Don't eat on Mac
        return true;
    }

    return false;
}

wxUint32 wxTextInputStream::Read32(int base)
{
    wxASSERT_MSG( !base || (base > 1 && base <= 36), _T("invalid base") );
    if(!m_input) return 0;

    wxString word = ReadWord();
    if(word.empty())
        return 0;
    return wxStrtoul(word.c_str(), 0, base);
}

wxUint16 wxTextInputStream::Read16(int base)
{
    return (wxUint16)Read32(base);
}

wxUint8 wxTextInputStream::Read8(int base)
{
    return (wxUint8)Read32(base);
}

wxInt32 wxTextInputStream::Read32S(int base)
{
    wxASSERT_MSG( !base || (base > 1 && base <= 36), _T("invalid base") );
    if(!m_input) return 0;

    wxString word = ReadWord();
    if(word.empty())
        return 0;
    return wxStrtol(word.c_str(), 0, base);
}

wxInt16 wxTextInputStream::Read16S(int base)
{
    return (wxInt16)Read32S(base);
}

wxInt8 wxTextInputStream::Read8S(int base)
{
    return (wxInt8)Read32S(base);
}

double wxTextInputStream::ReadDouble()
{
    if(!m_input) return 0;
    wxString word = ReadWord();
    if(word.empty())
        return 0;
    return wxStrtod(word.c_str(), 0);
}

#if WXWIN_COMPATIBILITY_2_6

wxString wxTextInputStream::ReadString()
{
    return ReadLine();
}

#endif // WXWIN_COMPATIBILITY_2_6

wxString wxTextInputStream::ReadLine()
{
    wxString line;

    while ( !m_input.Eof() )
    {
        wxChar c = NextChar();
        if(c == wxEOT)
            break;

        if (EatEOL(c))
            break;

        line += c;
    }

    return line;
}

wxString wxTextInputStream::ReadWord()
{
    wxString word;

    if ( !m_input )
        return word;

    wxChar c = NextNonSeparators();
    if ( !c )
        return word;

    word += c;

    while ( !m_input.Eof() )
    {
        c = NextChar();
        if(c == wxEOT)
            break;

        if (m_separators.Contains(c))
            break;

        if (EatEOL(c))
            break;

        word += c;
    }

    return word;
}

wxTextInputStream& wxTextInputStream::operator>>(wxString& word)
{
    word = ReadWord();
    return *this;
}

wxTextInputStream& wxTextInputStream::operator>>(char& c)
{
    c = m_input.GetC();
    if(m_input.LastRead() <= 0) c = 0;

    if (EatEOL(c))
        c = '\n';

    return *this;
}

#if wxUSE_UNICODE && wxWCHAR_T_IS_REAL_TYPE

wxTextInputStream& wxTextInputStream::operator>>(wchar_t& wc)
{
    wc = GetChar();

    return *this;
}

#endif // wxUSE_UNICODE

wxTextInputStream& wxTextInputStream::operator>>(wxInt16& i)
{
    i = (wxInt16)Read16();
    return *this;
}

wxTextInputStream& wxTextInputStream::operator>>(wxInt32& i)
{
    i = (wxInt32)Read32();
    return *this;
}

wxTextInputStream& wxTextInputStream::operator>>(wxUint16& i)
{
    i = Read16();
    return *this;
}

wxTextInputStream& wxTextInputStream::operator>>(wxUint32& i)
{
    i = Read32();
    return *this;
}

wxTextInputStream& wxTextInputStream::operator>>(double& i)
{
    i = ReadDouble();
    return *this;
}

wxTextInputStream& wxTextInputStream::operator>>(float& f)
{
    f = (float)ReadDouble();
    return *this;
}



#if wxUSE_UNICODE
wxTextOutputStream::wxTextOutputStream(wxOutputStream& s,
                                       wxEOL mode,
                                       const wxMBConv& conv)
  : m_output(s), m_conv(conv.Clone())
#else
wxTextOutputStream::wxTextOutputStream(wxOutputStream& s, wxEOL mode)
  : m_output(s)
#endif
{
    m_mode = mode;
    if (m_mode == wxEOL_NATIVE)
    {
#if defined(__WXMSW__) || defined(__WXPM__)
        m_mode = wxEOL_DOS;
#elif defined(__WXMAC__) && !defined(__DARWIN__)
        m_mode = wxEOL_MAC;
#else
        m_mode = wxEOL_UNIX;
#endif
    }
}

wxTextOutputStream::~wxTextOutputStream()
{
#if wxUSE_UNICODE
    delete m_conv;
#endif // wxUSE_UNICODE
}

void wxTextOutputStream::SetMode(wxEOL mode)
{
    m_mode = mode;
    if (m_mode == wxEOL_NATIVE)
    {
#if defined(__WXMSW__) || defined(__WXPM__)
        m_mode = wxEOL_DOS;
#elif defined(__WXMAC__) && !defined(__DARWIN__)
        m_mode = wxEOL_MAC;
#else
        m_mode = wxEOL_UNIX;
#endif
    }
}

void wxTextOutputStream::Write32(wxUint32 i)
{
    wxString str;
    str.Printf(wxT("%u"), i);

    WriteString(str);
}

void wxTextOutputStream::Write16(wxUint16 i)
{
    wxString str;
    str.Printf(wxT("%u"), (unsigned)i);

    WriteString(str);
}

void wxTextOutputStream::Write8(wxUint8 i)
{
    wxString str;
    str.Printf(wxT("%u"), (unsigned)i);

    WriteString(str);
}

void wxTextOutputStream::WriteDouble(double d)
{
    wxString str;

    str.Printf(wxT("%f"), d);
    WriteString(str);
}

void wxTextOutputStream::WriteString(const wxString& string)
{
    size_t len = string.length();

    wxString out;
    out.reserve(len);

    for ( size_t i = 0; i < len; i++ )
    {
        const wxChar c = string[i];
        if ( c == wxT('\n') )
        {
            switch ( m_mode )
            {
                case wxEOL_DOS:
                    out << _T("\r\n");
                    continue;

                case wxEOL_MAC:
                    out << _T('\r');
                    continue;

                default:
                    wxFAIL_MSG( _T("unknown EOL mode in wxTextOutputStream") );
                    // fall through

                case wxEOL_UNIX:
                    // don't treat '\n' specially
                    ;
            }
        }

        out << c;
    }

#if wxUSE_UNICODE
    wxCharBuffer buffer = m_conv->cWC2MB(out, out.length(), &len);
    m_output.Write(buffer, len);
#else
    m_output.Write(out.c_str(), out.length() );
#endif
}

wxTextOutputStream& wxTextOutputStream::PutChar(wxChar c)
{
#if wxUSE_UNICODE
    WriteString( wxString(&c, *m_conv, 1) );
#else
    WriteString( wxString(&c, wxConvLocal, 1) );
#endif
    return *this;
}

wxTextOutputStream& wxTextOutputStream::operator<<(const wxChar *string)
{
    WriteString( wxString(string) );
    return *this;
}

wxTextOutputStream& wxTextOutputStream::operator<<(const wxString& string)
{
    WriteString( string );
    return *this;
}

wxTextOutputStream& wxTextOutputStream::operator<<(char c)
{
    WriteString( wxString::FromAscii(c) );

    return *this;
}

#if wxUSE_UNICODE && wxWCHAR_T_IS_REAL_TYPE

wxTextOutputStream& wxTextOutputStream::operator<<(wchar_t wc)
{
    WriteString( wxString(&wc, *m_conv, 1) );

    return *this;
}

#endif // wxUSE_UNICODE

wxTextOutputStream& wxTextOutputStream::operator<<(wxInt16 c)
{
    wxString str;
    str.Printf(wxT("%d"), (signed int)c);
    WriteString(str);

    return *this;
}

wxTextOutputStream& wxTextOutputStream::operator<<(wxInt32 c)
{
    wxString str;
    str.Printf(wxT("%ld"), (signed long)c);
    WriteString(str);

    return *this;
}

wxTextOutputStream& wxTextOutputStream::operator<<(wxUint16 c)
{
    wxString str;
    str.Printf(wxT("%u"), (unsigned int)c);
    WriteString(str);

    return *this;
}

wxTextOutputStream& wxTextOutputStream::operator<<(wxUint32 c)
{
    wxString str;
    str.Printf(wxT("%lu"), (unsigned long)c);
    WriteString(str);

    return *this;
}

wxTextOutputStream &wxTextOutputStream::operator<<(double f)
{
    WriteDouble(f);
    return *this;
}

wxTextOutputStream& wxTextOutputStream::operator<<(float f)
{
    WriteDouble((double)f);
    return *this;
}

wxTextOutputStream &endl( wxTextOutputStream &stream )
{
    return stream.PutChar(wxT('\n'));
}

#endif
  // wxUSE_STREAMS
