/////////////////////////////////////////////////////////////////////////////
// Name:        htmlprep.h
// Purpose:     HTML processor
// Author:      Vaclav Slavik
// RCS-ID:      $Id: htmlproc.h 35650 2005-09-23 12:56:45Z MR $
// Copyright:   (c) 2001 Vaclav Slavik
// Licence:     wxWindows licence
/////////////////////////////////////////////////////////////////////////////

#ifndef _WX_HTMLPREP_H_
#define _WX_HTMLPREP_H_

#include "wx/defs.h"

#if wxUSE_HTML

#include "wx/string.h"

// Priority of preprocessor in the chain. The higher, the earlier it is used
enum
{
    wxHTML_PRIORITY_DONTCARE = 128, // if the order doesn't matter, use this
                                    // priority
    wxHTML_PRIORITY_SYSTEM   = 256  // >=256 is only for wxHTML's internals
};

// Classes derived from this class serve as simple text processors for
// wxHtmlWindow. wxHtmlWindow runs HTML markup through all registered
// processors before displaying it, thus allowing for on-the-fly
// modifications of the markup.

class WXDLLIMPEXP_HTML wxHtmlProcessor : public wxObject
{
    DECLARE_ABSTRACT_CLASS(wxHtmlProcessor)

public:
    wxHtmlProcessor() : wxObject(), m_enabled(true) {}
    virtual ~wxHtmlProcessor() {}

    // Process input text and return processed result
    virtual wxString Process(const wxString& text) const = 0;

    // Return priority value of this processor. The higher, the sooner
    // is the processor applied to the text.
    virtual int GetPriority() const { return wxHTML_PRIORITY_DONTCARE; }

    // Enable/disable the processor. wxHtmlWindow won't use a disabled
    // processor even if it is in its processors queue.
    virtual void Enable(bool enable = true) { m_enabled = enable; }
    bool IsEnabled() const { return m_enabled; }

protected:
    bool m_enabled;
};

#endif // wxUSE_HTML

#endif // _WX_HTMLPROC_H_
