///////////////////////////////////////////////////////////////////////////////
// Name:        wx/textbuf.h
// Purpose:     class wxTextBuffer to work with text buffers of _small_ size
//              (buffer is fully loaded in memory) and which understands CR/LF
//              differences between platforms.
// Created:     14.11.01
// Author:      Morten Hanssen, Vadim Zeitlin
// Copyright:   (c) 1998-2001 Morten Hanssen, Vadim Zeitlin
// Licence:     wxWindows licence
///////////////////////////////////////////////////////////////////////////////

#ifndef _WX_TEXTBUFFER_H
#define _WX_TEXTBUFFER_H

#include "wx/defs.h"
#include "wx/arrstr.h"
#include "wx/convauto.h"

// ----------------------------------------------------------------------------
// constants
// ----------------------------------------------------------------------------

// the line termination type (kept wxTextFileType name for compability)
enum wxTextFileType
{
    wxTextFileType_None,  // incomplete (the last line of the file only)
    wxTextFileType_Unix,  // line is terminated with 'LF' = 0xA = 10 = '\n'
    wxTextFileType_Dos,   //                         'CR' 'LF'
    wxTextFileType_Mac,   //                         'CR' = 0xD = 13 = '\r'
    wxTextFileType_Os2    //                         'CR' 'LF'
};

#include "wx/string.h"

#if wxUSE_TEXTBUFFER

#include "wx/dynarray.h"

// ----------------------------------------------------------------------------
// wxTextBuffer
// ----------------------------------------------------------------------------

WX_DEFINE_USER_EXPORTED_ARRAY_INT(wxTextFileType,
                                  wxArrayLinesType,
                                  class WXDLLIMPEXP_BASE);

#endif // wxUSE_TEXTBUFFER

class WXDLLIMPEXP_BASE wxTextBuffer
{
public:
    // constants and static functions
    // default type for current platform (determined at compile time)
    static const wxTextFileType typeDefault;

    // this function returns a string which is identical to "text" passed in
    // except that the line terminator characters are changed to correspond the
    // given type. Called with the default argument, the function translates
    // the string to the native format (Unix for Unix, DOS for Windows, ...).
    static wxString Translate(const wxString& text,
                            wxTextFileType type = typeDefault);

    // get the buffer termination string
    static const wxChar *GetEOL(wxTextFileType type = typeDefault);

    // the static methods of this class are compiled in even when
    // !wxUSE_TEXTBUFFER because they are used by the library itself, but the
    // rest can be left out
#if wxUSE_TEXTBUFFER

    // buffer operations
    // -----------------

    // buffer exists?
    bool Exists() const;

    // create the buffer if it doesn't already exist
    bool Create();

    // same as Create() but with (another) buffer name
    bool Create(const wxString& strBufferName);

    // Open() also loads buffer in memory on success
    bool Open(const wxMBConv& conv = wxConvAuto());

    // same as Open() but with (another) buffer name
    bool Open(const wxString& strBufferName, const wxMBConv& conv = wxConvAuto());

    // closes the buffer and frees memory, losing all changes
    bool Close();

    // is buffer currently opened?
    bool IsOpened() const { return m_isOpened; }

    // accessors
    // ---------

    // get the number of lines in the buffer
    size_t GetLineCount() const { return m_aLines.size(); }

    // the returned line may be modified (but don't add CR/LF at the end!)
    wxString& GetLine(size_t n)    const { return (wxString&)m_aLines[n]; }
    wxString& operator[](size_t n) const { return (wxString&)m_aLines[n]; }

    // the current line has meaning only when you're using
    // GetFirstLine()/GetNextLine() functions, it doesn't get updated when
    // you're using "direct access" i.e. GetLine()
    size_t GetCurrentLine() const { return m_nCurLine; }
    void GoToLine(size_t n) { m_nCurLine = n; }
    bool Eof() const { return m_nCurLine == m_aLines.size(); }

    // these methods allow more "iterator-like" traversal of the list of
    // lines, i.e. you may write something like:
    //  for ( str = GetFirstLine(); !Eof(); str = GetNextLine() ) { ... }

    // NB: const is commented out because not all compilers understand
    //     'mutable' keyword yet (m_nCurLine should be mutable)
    wxString& GetFirstLine() /* const */
        { return m_aLines.empty() ? ms_eof : m_aLines[m_nCurLine = 0]; }
    wxString& GetNextLine()  /* const */
        { return ++m_nCurLine == m_aLines.size() ? ms_eof
                                                 : m_aLines[m_nCurLine]; }
    wxString& GetPrevLine()  /* const */
        { wxASSERT(m_nCurLine > 0); return m_aLines[--m_nCurLine]; }
    wxString& GetLastLine() /* const */
        { m_nCurLine = m_aLines.size() - 1; return m_aLines.Last(); }

    // get the type of the line (see also GetEOL)
    wxTextFileType GetLineType(size_t n) const { return m_aTypes[n]; }

    // guess the type of buffer
    wxTextFileType GuessType() const;

    // get the name of the buffer
    const wxChar *GetName() const { return m_strBufferName.c_str(); }

    // add/remove lines
    // ----------------

    // add a line to the end
    void AddLine(const wxString& str, wxTextFileType type = typeDefault)
        { m_aLines.push_back(str); m_aTypes.push_back(type); }
    // insert a line before the line number n
    void InsertLine(const wxString& str,
                  size_t n,
                  wxTextFileType type = typeDefault)
    {
        m_aLines.insert(m_aLines.begin() + n, str);
        m_aTypes.insert(m_aTypes.begin()+n, type);
    }

    // delete one line
    void RemoveLine(size_t n)
    {
        m_aLines.erase(m_aLines.begin() + n);
        m_aTypes.erase(m_aTypes.begin() + n);
    }

    // remove all lines
    void Clear() { m_aLines.clear(); m_aTypes.clear(); m_nCurLine = 0; }

    // change the buffer (default argument means "don't change type")
    // possibly in another format
    bool Write(wxTextFileType typeNew = wxTextFileType_None,
               const wxMBConv& conv = wxConvAuto());

    // dtor
    virtual ~wxTextBuffer();

protected:
    // ctors
    // -----

    // default ctor, use Open(string)
    wxTextBuffer() { m_nCurLine = 0; m_isOpened = false; }

    // ctor from filename
    wxTextBuffer(const wxString& strBufferName);

    enum wxTextBufferOpenMode { ReadAccess, WriteAccess };

    // Must implement these in derived classes.
    virtual bool OnExists() const = 0;
    virtual bool OnOpen(const wxString &strBufferName,
                        wxTextBufferOpenMode openmode) = 0;
    virtual bool OnClose() = 0;
    virtual bool OnRead(const wxMBConv& conv) = 0;
    virtual bool OnWrite(wxTextFileType typeNew, const wxMBConv& conv) = 0;

    static wxString ms_eof;     // dummy string returned at EOF
    wxString m_strBufferName;   // name of the buffer

private:
    wxArrayLinesType m_aTypes;   // type of each line
    wxArrayString    m_aLines;   // lines of file

    size_t        m_nCurLine; // number of current line in the buffer

    bool          m_isOpened; // was the buffer successfully opened the last time?
#endif // wxUSE_TEXTBUFFER

    // copy ctor/assignment operator not implemented
    wxTextBuffer(const wxTextBuffer&);
    wxTextBuffer& operator=(const wxTextBuffer&);
};

#endif // _WX_TEXTBUFFER_H

