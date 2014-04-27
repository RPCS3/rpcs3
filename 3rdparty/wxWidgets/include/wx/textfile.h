///////////////////////////////////////////////////////////////////////////////
// Name:        wx/textfile.h
// Purpose:     class wxTextFile to work with text files of _small_ size
//              (file is fully loaded in memory) and which understands CR/LF
//              differences between platforms.
// Author:      Vadim Zeitlin
// Modified by:
// Created:     03.04.98
// RCS-ID:      $Id: textfile.h 38570 2006-04-05 14:37:47Z VZ $
// Copyright:   (c) 1998 Vadim Zeitlin <zeitlin@dptmaths.ens-cachan.fr>
// Licence:     wxWindows licence
///////////////////////////////////////////////////////////////////////////////

#ifndef _WX_TEXTFILE_H
#define _WX_TEXTFILE_H

#include "wx/defs.h"

#include "wx/textbuf.h"

#if wxUSE_TEXTFILE

#include "wx/file.h"

// ----------------------------------------------------------------------------
// wxTextFile
// ----------------------------------------------------------------------------

class WXDLLIMPEXP_BASE wxTextFile : public wxTextBuffer
{
public:
    // constructors
    wxTextFile() { }
    wxTextFile(const wxString& strFileName);

protected:
    // implement the base class pure virtuals
    virtual bool OnExists() const;
    virtual bool OnOpen(const wxString &strBufferName,
                        wxTextBufferOpenMode OpenMode);
    virtual bool OnClose();
    virtual bool OnRead(const wxMBConv& conv);
    virtual bool OnWrite(wxTextFileType typeNew, const wxMBConv& conv);

private:

    wxFile m_file;

    DECLARE_NO_COPY_CLASS(wxTextFile)
};

#else // !wxUSE_TEXTFILE

// old code relies on the static methods of wxTextFile being always available
// and they still are available in wxTextBuffer (even if !wxUSE_TEXTBUFFER), so
// make it possible to use them in a backwards compatible way
typedef wxTextBuffer wxTextFile;

#endif // wxUSE_TEXTFILE/!wxUSE_TEXTFILE

#endif // _WX_TEXTFILE_H

