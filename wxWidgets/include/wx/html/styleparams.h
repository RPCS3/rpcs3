/////////////////////////////////////////////////////////////////////////////
// Name:        wx/html/styleparams.h
// Purpose:     wxHtml helper code for extracting style parameters
// Author:      Nigel Paton
// Copyright:   wxWidgets team
// Licence:     wxWindows licence
/////////////////////////////////////////////////////////////////////////////

#ifndef _WX_HTML_STYLEPARAMS_H_
#define _WX_HTML_STYLEPARAMS_H_

#include "wx/defs.h"

#if wxUSE_HTML

#include "wx/arrstr.h"

class WXDLLIMPEXP_FWD_HTML wxHtmlTag;

// This is a private class used by wxHTML to parse "style" attributes of HTML
// elements. Currently both parsing and support for the parsed values is pretty
// trivial.
class WXDLLIMPEXP_HTML wxHtmlStyleParams
{
public:
    // Construct a style parameters object corresponding to the style attribute
    // of the given HTML tag.
    wxHtmlStyleParams(const wxHtmlTag& tag);

    // Check whether the named parameter is present or not.
    bool HasParam(const wxString& par) const
    {
        return m_names.Index(par, false /* ignore case */) != wxNOT_FOUND;
    }

    // Get the value of the named parameter, return empty string if none.
    wxString GetParam(const wxString& par) const
    {
        int index = m_names.Index(par, false);
        return index == wxNOT_FOUND ? wxString() : m_values[index];
    }

private:
    // Arrays if names and values of the parameters
    wxArrayString
        m_names,
        m_values;
};

#endif // wxUSE_HTML

#endif // _WX_HTML_STYLEPARAMS_H_
