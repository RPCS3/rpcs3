///////////////////////////////////////////////////////////////////////////////
// Name:        src/common/textbuf.cpp
// Purpose:     implementation of wxTextBuffer class
// Created:     14.11.01
// Author:      Morten Hanssen, Vadim Zeitlin
// RCS-ID:      $Id: textbuf.cpp 38570 2006-04-05 14:37:47Z VZ $
// Copyright:   (c) 1998-2001 wxWidgets team
// Licence:     wxWindows licence
///////////////////////////////////////////////////////////////////////////////

// ============================================================================
// headers
// ============================================================================

#include  "wx/wxprec.h"

#ifdef __BORLANDC__
    #pragma hdrstop
#endif  //__BORLANDC__

#ifndef WX_PRECOMP
    #include  "wx/string.h"
    #include  "wx/intl.h"
    #include  "wx/log.h"
#endif

#include "wx/textbuf.h"

// ============================================================================
// wxTextBuffer class implementation
// ============================================================================

// ----------------------------------------------------------------------------
// static methods (always compiled in)
// ----------------------------------------------------------------------------

// default type is the native one
// the native type under Mac OS X is:
//   - Unix when compiling with the Apple Developer Tools (__UNIX__)
//   - Mac when compiling with CodeWarrior (__WXMAC__)

const wxTextFileType wxTextBuffer::typeDefault =
#if defined(__WINDOWS__) || defined(__DOS__) || defined(__PALMOS__)
  wxTextFileType_Dos;
#elif defined(__UNIX__)
  wxTextFileType_Unix;
#elif defined(__WXMAC__)
  wxTextFileType_Mac;
#elif defined(__OS2__)
  wxTextFileType_Os2;
#else
  wxTextFileType_None;
  #error  "wxTextBuffer: unsupported platform."
#endif

const wxChar *wxTextBuffer::GetEOL(wxTextFileType type)
{
    switch ( type ) {
        default:
            wxFAIL_MSG(wxT("bad buffer type in wxTextBuffer::GetEOL."));
            // fall through nevertheless - we must return something...

        case wxTextFileType_None: return wxEmptyString;
        case wxTextFileType_Unix: return wxT("\n");
        case wxTextFileType_Dos:  return wxT("\r\n");
        case wxTextFileType_Mac:  return wxT("\r");
    }
}

wxString wxTextBuffer::Translate(const wxString& text, wxTextFileType type)
{
    // don't do anything if there is nothing to do
    if ( type == wxTextFileType_None )
        return text;

    // nor if it is empty
    if ( text.empty() )
        return text;

    wxString eol = GetEOL(type), result;

    // optimization: we know that the length of the new string will be about
    // the same as the length of the old one, so prealloc memory to aviod
    // unnecessary relocations
    result.Alloc(text.Len());

    wxChar chLast = 0;
    for ( const wxChar *pc = text.c_str(); *pc; pc++ )
    {
        wxChar ch = *pc;
        switch ( ch ) {
            case _T('\n'):
                // Dos/Unix line termination
                result += eol;
                chLast = 0;
                break;

            case _T('\r'):
                if ( chLast == _T('\r') ) {
                    // Mac empty line
                    result += eol;
                }
                else {
                    // just remember it: we don't know whether it is just "\r"
                    // or "\r\n" yet
                    chLast = _T('\r');
                }
                break;

            default:
                if ( chLast == _T('\r') ) {
                    // Mac line termination
                    result += eol;

                    // reset chLast to avoid inserting another eol before the
                    // next character
                    chLast = 0;
                }

                // add to the current line
                result += ch;
        }
    }

    if ( chLast ) {
        // trailing '\r'
        result += eol;
    }

    return result;
}

#if wxUSE_TEXTBUFFER

wxString wxTextBuffer::ms_eof;

// ----------------------------------------------------------------------------
// ctors & dtor
// ----------------------------------------------------------------------------

wxTextBuffer::wxTextBuffer(const wxString& strBufferName)
            : m_strBufferName(strBufferName)
{
    m_nCurLine = 0;
    m_isOpened = false;
}

wxTextBuffer::~wxTextBuffer()
{
    // required here for Darwin
}

// ----------------------------------------------------------------------------
// buffer operations
// ----------------------------------------------------------------------------

bool wxTextBuffer::Exists() const
{
    return OnExists();
}

bool wxTextBuffer::Create(const wxString& strBufferName)
{
    m_strBufferName = strBufferName;

    return Create();
}

bool wxTextBuffer::Create()
{
    // buffer name must be either given in ctor or in Create(const wxString&)
    wxASSERT( !m_strBufferName.empty() );

    // if the buffer already exists do nothing
    if ( Exists() ) return false;

    if ( !OnOpen(m_strBufferName, WriteAccess) )
        return false;

    OnClose();
    return true;
}

bool wxTextBuffer::Open(const wxString& strBufferName, const wxMBConv& conv)
{
    m_strBufferName = strBufferName;

    return Open(conv);
}

bool wxTextBuffer::Open(const wxMBConv& conv)
{
    // buffer name must be either given in ctor or in Open(const wxString&)
    wxASSERT( !m_strBufferName.empty() );

    // open buffer in read-only mode
    if ( !OnOpen(m_strBufferName, ReadAccess) )
        return false;

    // read buffer into memory
    m_isOpened = OnRead(conv);

    OnClose();

    return m_isOpened;
}

// analyse some lines of the buffer trying to guess it's type.
// if it fails, it assumes the native type for our platform.
wxTextFileType wxTextBuffer::GuessType() const
{
    wxASSERT( IsOpened() );

    // scan the buffer lines
    size_t nUnix = 0,     // number of '\n's alone
           nDos  = 0,     // number of '\r\n'
           nMac  = 0;     // number of '\r's

    // we take MAX_LINES_SCAN in the beginning, middle and the end of buffer
    #define MAX_LINES_SCAN    (10)
    size_t nCount = m_aLines.Count() / 3,
        nScan =  nCount > 3*MAX_LINES_SCAN ? MAX_LINES_SCAN : nCount / 3;

    #define   AnalyseLine(n)              \
        switch ( m_aTypes[n] ) {            \
            case wxTextFileType_Unix: nUnix++; break;   \
            case wxTextFileType_Dos:  nDos++;  break;   \
            case wxTextFileType_Mac:  nMac++;  break;   \
            default: wxFAIL_MSG(_("unknown line terminator")); \
        }

    size_t n;
    for ( n = 0; n < nScan; n++ )     // the beginning
        AnalyseLine(n);
    for ( n = (nCount - nScan)/2; n < (nCount + nScan)/2; n++ )
        AnalyseLine(n);
    for ( n = nCount - nScan; n < nCount; n++ )
        AnalyseLine(n);

    #undef   AnalyseLine

    // interpret the results (FIXME far from being even 50% fool proof)
    if ( nScan > 0 && nDos + nUnix + nMac == 0 ) {
        // no newlines at all
        wxLogWarning(_("'%s' is probably a binary buffer."), m_strBufferName.c_str());
    }
    else {
        #define   GREATER_OF(t1, t2) n##t1 == n##t2 ? typeDefault               \
                                                : n##t1 > n##t2             \
                                                    ? wxTextFileType_##t1   \
                                                    : wxTextFileType_##t2

#if !defined(__WATCOMC__) || wxCHECK_WATCOM_VERSION(1,4)
        if ( nDos > nUnix )
            return GREATER_OF(Dos, Mac);
        else if ( nDos < nUnix )
            return GREATER_OF(Unix, Mac);
        else {
            // nDos == nUnix
            return nMac > nDos ? wxTextFileType_Mac : typeDefault;
        }
#endif // __WATCOMC__

        #undef    GREATER_OF
    }

    return typeDefault;
}


bool wxTextBuffer::Close()
{
    Clear();
    m_isOpened = false;

    return true;
}

bool wxTextBuffer::Write(wxTextFileType typeNew, const wxMBConv& conv)
{
    return OnWrite(typeNew, conv);
}

#endif // wxUSE_TEXTBUFFER
