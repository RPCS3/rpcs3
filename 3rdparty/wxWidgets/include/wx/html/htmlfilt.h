/////////////////////////////////////////////////////////////////////////////
// Name:        htmlfilt.h
// Purpose:     filters
// Author:      Vaclav Slavik
// RCS-ID:      $Id: htmlfilt.h 35650 2005-09-23 12:56:45Z MR $
// Copyright:   (c) 1999 Vaclav Slavik
// Licence:     wxWindows licence
/////////////////////////////////////////////////////////////////////////////

#ifndef _WX_HTMLFILT_H_
#define _WX_HTMLFILT_H_

#include "wx/defs.h"

#if wxUSE_HTML

#include "wx/filesys.h"


//--------------------------------------------------------------------------------
// wxHtmlFilter
//                  This class is input filter. It can "translate" files
//                  in non-HTML format to HTML format
//                  interface to access certain
//                  kinds of files (HTPP, FTP, local, tar.gz etc..)
//--------------------------------------------------------------------------------

class WXDLLIMPEXP_HTML wxHtmlFilter : public wxObject
{
    DECLARE_ABSTRACT_CLASS(wxHtmlFilter)

public:
    wxHtmlFilter() : wxObject() {}
    virtual ~wxHtmlFilter() {}

    // returns true if this filter is able to open&read given file
    virtual bool CanRead(const wxFSFile& file) const = 0;

    // Reads given file and returns HTML document.
    // Returns empty string if opening failed
    virtual wxString ReadFile(const wxFSFile& file) const = 0;
};



//--------------------------------------------------------------------------------
// wxHtmlFilterPlainText
//                  This filter is used as default filter if no other can
//                  be used (= uknown type of file). It is used by
//                  wxHtmlWindow itself.
//--------------------------------------------------------------------------------


class WXDLLIMPEXP_HTML wxHtmlFilterPlainText : public wxHtmlFilter
{
    DECLARE_DYNAMIC_CLASS(wxHtmlFilterPlainText)

public:
    virtual bool CanRead(const wxFSFile& file) const;
    virtual wxString ReadFile(const wxFSFile& file) const;
};

//--------------------------------------------------------------------------------
// wxHtmlFilterHTML
//          filter for text/html
//--------------------------------------------------------------------------------

class wxHtmlFilterHTML : public wxHtmlFilter
{
    DECLARE_DYNAMIC_CLASS(wxHtmlFilterHTML)

    public:
        virtual bool CanRead(const wxFSFile& file) const;
        virtual wxString ReadFile(const wxFSFile& file) const;
};



#endif // wxUSE_HTML

#endif // _WX_HTMLFILT_H_

