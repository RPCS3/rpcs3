///////////////////////////////////////////////////////////////////////////////
// Name:        src/common/textfile.cpp
// Purpose:     implementation of wxTextFile class
// Author:      Vadim Zeitlin
// Modified by:
// Created:     03.04.98
// RCS-ID:      $Id: textfile.cpp 49298 2007-10-21 18:05:49Z SC $
// Copyright:   (c) 1998 Vadim Zeitlin <zeitlin@dptmaths.ens-cachan.fr>
// Licence:     wxWindows licence
///////////////////////////////////////////////////////////////////////////////

// ============================================================================
// headers
// ============================================================================

#include  "wx/wxprec.h"

#ifdef    __BORLANDC__
    #pragma hdrstop
#endif  //__BORLANDC__

#if !wxUSE_FILE || !wxUSE_TEXTBUFFER
    #undef wxUSE_TEXTFILE
    #define wxUSE_TEXTFILE 0
#endif // wxUSE_FILE

#if wxUSE_TEXTFILE

#ifndef WX_PRECOMP
    #include "wx/string.h"
    #include "wx/intl.h"
    #include "wx/file.h"
    #include "wx/log.h"
#endif

#include "wx/textfile.h"
#include "wx/filename.h"
#include "wx/buffer.h"

// ============================================================================
// wxTextFile class implementation
// ============================================================================

wxTextFile::wxTextFile(const wxString& strFileName)
          : wxTextBuffer(strFileName)
{
}


// ----------------------------------------------------------------------------
// file operations
// ----------------------------------------------------------------------------

bool wxTextFile::OnExists() const
{
    return wxFile::Exists(m_strBufferName);
}


bool wxTextFile::OnOpen(const wxString &strBufferName, wxTextBufferOpenMode OpenMode)
{
    wxFile::OpenMode FileOpenMode;

    switch ( OpenMode )
    {
        default:
            wxFAIL_MSG( _T("unknown open mode in wxTextFile::Open") );
            // fall through

        case ReadAccess :
            FileOpenMode = wxFile::read;
            break;

        case WriteAccess :
            FileOpenMode = wxFile::write;
            break;
    }

    return m_file.Open(strBufferName.c_str(), FileOpenMode);
}


bool wxTextFile::OnClose()
{
    return m_file.Close();
}


bool wxTextFile::OnRead(const wxMBConv& conv)
{
    // file should be opened
    wxASSERT_MSG( m_file.IsOpened(), _T("can't read closed file") );

    // read the entire file in memory: this is not the most efficient thing to
    // do but there is no good way to avoid it in Unicode build because if we
    // read the file block by block we can't convert each block to Unicode
    // separately (the last multibyte char in the block might be only partially
    // read and so the conversion would fail) and, as the file contents is kept
    // in memory by wxTextFile anyhow, it shouldn't be a big problem to read
    // the file entirely
    size_t bufSize = 0,
           bufPos = 0;
    char block[1024];
    wxCharBuffer buf;

    // first determine if the file is seekable or not and so whether we can
    // determine its length in advance
    wxFileOffset fileLength;
    {
        wxLogNull logNull;
        fileLength = m_file.Length();
    }

    // some non-seekable files under /proc under Linux pretend that they're
    // seekable but always return 0; others do return an error
    const bool seekable = fileLength != wxInvalidOffset && fileLength != 0;
    if ( seekable )
    {
        // we know the required length, so set the buffer size in advance
        bufSize = fileLength;
        if ( !buf.extend(bufSize - 1 /* it adds 1 internally */) )
            return false;

        // if the file is seekable, also check that we're at its beginning
        wxASSERT_MSG( m_file.Tell() == 0, _T("should be at start of file") );
    }

    for ( ;; )
    {
        ssize_t nRead = m_file.Read(block, WXSIZEOF(block));

        if ( nRead == wxInvalidOffset )
        {
            // read error (error message already given in wxFile::Read)
            return false;
        }

        if ( nRead == 0 )
        {
            // if no bytes have been read, presumably this is a valid-but-empty file
            if ( bufPos == 0 )
                return true;

            // otherwise we've finished reading the file
            break;
        }

        if ( seekable )
        {
            // this shouldn't happen but don't overwrite the buffer if it does
            wxCHECK_MSG( bufPos + nRead <= bufSize, false,
                         _T("read more than file length?") );
        }
        else // !seekable
        {
            // for non-seekable files we have to allocate more memory on the go
            if ( !buf.extend(bufPos + nRead - 1 /* it adds 1 internally */) )
                return false;
        }

        // append to the buffer
        memcpy(buf.data() + bufPos, block, nRead);
        bufPos += nRead;
    }

    if ( !seekable )
    {
        bufSize = bufPos;
    }

    const wxString str(buf, conv, bufPos);

    // there's no risk of this happening in ANSI build
#if wxUSE_UNICODE
    if ( bufSize > 4 && str.empty() )
    {
        wxLogError(_("Failed to convert file \"%s\" to Unicode."), GetName());
        return false;
    }
#endif // wxUSE_UNICODE

    free(buf.release()); // we don't need this memory any more


    // now break the buffer in lines

    // last processed character, we need to know if it was a CR or not
    wxChar chLast = '\0';

    // the beginning of the current line, changes inside the loop
    wxString::const_iterator lineStart = str.begin();
    const wxString::const_iterator end = str.end();
    for ( wxString::const_iterator p = lineStart; p != end; p++ )
    {
        const wxChar ch = *p;
        switch ( ch )
        {
            case '\n':
                // could be a DOS or Unix EOL
                if ( chLast == '\r' )
                {
                    if ( p - 1 >= lineStart )
                    {
                        AddLine(wxString(lineStart, p - 1), wxTextFileType_Dos);
                    }
                    else
                    {
                        // there were two line endings, so add an empty line:
                        AddLine(wxEmptyString, wxTextFileType_Dos);
                    }
                }
                else // bare '\n', Unix style
                {
                    AddLine(wxString(lineStart, p), wxTextFileType_Unix);
                }

                lineStart = p + 1;
                break;

            case '\r':
                if ( chLast == '\r' )
                {
                    if ( p - 1 >= lineStart )
                    {
                        AddLine(wxString(lineStart, p - 1), wxTextFileType_Mac);
                    }
                    // Mac empty line
                    AddLine(wxEmptyString, wxTextFileType_Mac);
                    lineStart = p + 1;
                }
                //else: we don't know what this is yet -- could be a Mac EOL or
                //      start of DOS EOL so wait for next char
                break;

            default:
                if ( chLast == '\r' )
                {
                    // Mac line termination
                    if ( p - 1 >= lineStart )
                    {
                        AddLine(wxString(lineStart, p - 1), wxTextFileType_Mac);
                    }
                    else
                    {
                        // there were two line endings, so add an empty line:
                        AddLine(wxEmptyString, wxTextFileType_Mac);
                    }
                    lineStart = p;
                }
        }

        chLast = ch;
    }

    // anything in the last line?
    if ( lineStart != end )
    {
        // add unterminated last line
        AddLine(wxString(lineStart, end), wxTextFileType_None);
    }

    return true;
}


bool wxTextFile::OnWrite(wxTextFileType typeNew, const wxMBConv& conv)
{
    wxFileName fn = m_strBufferName;

    // We do NOT want wxPATH_NORM_CASE here, or the case will not
    // be preserved.
    if ( !fn.IsAbsolute() )
        fn.Normalize(wxPATH_NORM_ENV_VARS | wxPATH_NORM_DOTS | wxPATH_NORM_TILDE |
                     wxPATH_NORM_ABSOLUTE | wxPATH_NORM_LONG);

    wxTempFile fileTmp(fn.GetFullPath());

    if ( !fileTmp.IsOpened() ) {
        wxLogError(_("can't write buffer '%s' to disk."), m_strBufferName.c_str());
        return false;
    }

    size_t nCount = GetLineCount();
    for ( size_t n = 0; n < nCount; n++ ) {
        fileTmp.Write(GetLine(n) +
                      GetEOL(typeNew == wxTextFileType_None ? GetLineType(n)
                                                            : typeNew),
                      conv);
    }

    // replace the old file with this one
    return fileTmp.Commit();
}

#endif // wxUSE_TEXTFILE
