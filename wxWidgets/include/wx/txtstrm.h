/////////////////////////////////////////////////////////////////////////////
// Name:        wx/txtstrm.h
// Purpose:     Text stream classes
// Author:      Guilhem Lavaux
// Modified by:
// Created:     28/06/1998
// RCS-ID:      $Id: txtstrm.h 53135 2008-04-12 02:31:04Z VZ $
// Copyright:   (c) Guilhem Lavaux
// Licence:     wxWindows licence
/////////////////////////////////////////////////////////////////////////////

#ifndef _WX_TXTSTREAM_H_
#define _WX_TXTSTREAM_H_

#include "wx/stream.h"
#include "wx/convauto.h"

#if wxUSE_STREAMS

class WXDLLIMPEXP_FWD_BASE wxTextInputStream;
class WXDLLIMPEXP_FWD_BASE wxTextOutputStream;

typedef wxTextInputStream& (*__wxTextInputManip)(wxTextInputStream&);
typedef wxTextOutputStream& (*__wxTextOutputManip)(wxTextOutputStream&);

WXDLLIMPEXP_BASE wxTextOutputStream &endl( wxTextOutputStream &stream );


#define wxEOT wxT('\4') // the End-Of-Text control code (used only inside wxTextInputStream)

// If you're scanning through a file using wxTextInputStream, you should check for EOF _before_
// reading the next item (word / number), because otherwise the last item may get lost.
// You should however be prepared to receive an empty item (empty string / zero number) at the
// end of file, especially on Windows systems. This is unavoidable because most (but not all) files end
// with whitespace (i.e. usually a newline).
class WXDLLIMPEXP_BASE wxTextInputStream
{
public:
#if wxUSE_UNICODE
    wxTextInputStream(wxInputStream& s,
                      const wxString &sep=wxT(" \t"),
                      const wxMBConv& conv = wxConvAuto());
#else
    wxTextInputStream(wxInputStream& s, const wxString &sep=wxT(" \t"));
#endif
    ~wxTextInputStream();

    wxUint32 Read32(int base = 10); // base may be between 2 and 36, inclusive, or the special 0 (= C format)
    wxUint16 Read16(int base = 10);
    wxUint8  Read8(int base = 10);
    wxInt32  Read32S(int base = 10);
    wxInt16  Read16S(int base = 10);
    wxInt8   Read8S(int base = 10);
    double   ReadDouble();
    wxString ReadLine();
    wxString ReadWord();
    wxChar   GetChar() { wxChar c = NextChar(); return (wxChar)(c != wxEOT ? c : 0); }

    wxString GetStringSeparators() const { return m_separators; }
    void SetStringSeparators(const wxString &c) { m_separators = c; }

    // Operators
    wxTextInputStream& operator>>(wxString& word);
    wxTextInputStream& operator>>(char& c);
#if wxUSE_UNICODE && wxWCHAR_T_IS_REAL_TYPE
    wxTextInputStream& operator>>(wchar_t& wc);
#endif // wxUSE_UNICODE
    wxTextInputStream& operator>>(wxInt16& i);
    wxTextInputStream& operator>>(wxInt32& i);
    wxTextInputStream& operator>>(wxUint16& i);
    wxTextInputStream& operator>>(wxUint32& i);
    wxTextInputStream& operator>>(double& i);
    wxTextInputStream& operator>>(float& f);

    wxTextInputStream& operator>>( __wxTextInputManip func) { return func(*this); }

#if WXWIN_COMPATIBILITY_2_6
    wxDEPRECATED( wxString ReadString() );  // use ReadLine or ReadWord instead
#endif // WXWIN_COMPATIBILITY_2_6

protected:
    wxInputStream &m_input;
    wxString m_separators;
    char m_lastBytes[10]; // stores the bytes that were read for the last character

#if wxUSE_UNICODE
    wxMBConv *m_conv;
#endif

    bool   EatEOL(const wxChar &c);
    void   UngetLast(); // should be used instead of wxInputStream::Ungetch() because of Unicode issues
    // returns EOT (\4) if there is a stream error, or end of file
    wxChar NextChar();   // this should be used instead of GetC() because of Unicode issues
    wxChar NextNonSeparators();

    DECLARE_NO_COPY_CLASS(wxTextInputStream)
};

typedef enum
{
  wxEOL_NATIVE,
  wxEOL_UNIX,
  wxEOL_MAC,
  wxEOL_DOS
} wxEOL;

class WXDLLIMPEXP_BASE wxTextOutputStream
{
public:
#if wxUSE_UNICODE
    wxTextOutputStream(wxOutputStream& s,
                       wxEOL mode = wxEOL_NATIVE,
                       const wxMBConv& conv = wxConvAuto());
#else
    wxTextOutputStream(wxOutputStream& s, wxEOL mode = wxEOL_NATIVE);
#endif
    virtual ~wxTextOutputStream();

    void SetMode( wxEOL mode = wxEOL_NATIVE );
    wxEOL GetMode() { return m_mode; }

    void Write32(wxUint32 i);
    void Write16(wxUint16 i);
    void Write8(wxUint8 i);
    virtual void WriteDouble(double d);
    virtual void WriteString(const wxString& string);

    wxTextOutputStream& PutChar(wxChar c);

    wxTextOutputStream& operator<<(const wxChar *string);
    wxTextOutputStream& operator<<(const wxString& string);
    wxTextOutputStream& operator<<(char c);
#if wxUSE_UNICODE && wxWCHAR_T_IS_REAL_TYPE
    wxTextOutputStream& operator<<(wchar_t wc);
#endif // wxUSE_UNICODE
    wxTextOutputStream& operator<<(wxInt16 c);
    wxTextOutputStream& operator<<(wxInt32 c);
    wxTextOutputStream& operator<<(wxUint16 c);
    wxTextOutputStream& operator<<(wxUint32 c);
    wxTextOutputStream& operator<<(double f);
    wxTextOutputStream& operator<<(float f);

    wxTextOutputStream& operator<<( __wxTextOutputManip func) { return func(*this); }

protected:
    wxOutputStream &m_output;
    wxEOL           m_mode;

#if wxUSE_UNICODE
    wxMBConv *m_conv;
#endif

    DECLARE_NO_COPY_CLASS(wxTextOutputStream)
};

#endif
  // wxUSE_STREAMS

#endif
    // _WX_DATSTREAM_H_
