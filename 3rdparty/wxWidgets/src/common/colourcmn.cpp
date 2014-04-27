/////////////////////////////////////////////////////////////////////////////
// Name:        src/common/colourcmn.cpp
// Purpose:     wxColourBase implementation
// Author:      Francesco Montorsi
// Modified by:
// Created:     20/4/2006
// RCS-ID:      $Id: colourcmn.cpp 41538 2006-09-30 20:45:15Z RR $
// Copyright:   (c) Francesco Montorsi
// Licence:     wxWindows licence
/////////////////////////////////////////////////////////////////////////////


// For compilers that support precompilation, includes "wx.h".
#include "wx/wxprec.h"

#ifdef __BORLANDC__
    #pragma hdrstop
#endif

#include "wx/colour.h"

#ifndef WX_PRECOMP
    #include "wx/log.h"
    #include "wx/utils.h"
    #include "wx/gdicmn.h"
#endif

#if wxUSE_VARIANT
IMPLEMENT_VARIANT_OBJECT_EXPORTED(wxColour,WXDLLEXPORT)
#endif

// ============================================================================
// wxString <-> wxColour conversions
// ============================================================================

bool wxColourBase::FromString(const wxChar *str)
{
    if ( str == NULL || str[0] == wxT('\0'))
        return false;       // invalid or empty string

    if ( wxStrncmp(str, wxT("RGB"), 3) == 0 ||
         wxStrncmp(str, wxT("rgb"), 3) == 0 )
    {
        // CSS-like RGB specification
        // according to http://www.w3.org/TR/REC-CSS2/syndata.html#color-units
        // values outside 0-255 range are allowed but should be clipped
        int red, green, blue;
        if (wxSscanf(&str[3], wxT("(%d, %d, %d)"), &red, &green, &blue) != 3)
            return false;

        Set((unsigned char)wxClip(red,0,255),
            (unsigned char)wxClip(green,0,255),
            (unsigned char)wxClip(blue,0,255));
    }
    else if ( str[0] == wxT('#') && wxStrlen(str) == 7 )
    {
        // hexadecimal prefixed with # (HTML syntax)
        unsigned long tmp;
        if (wxSscanf(&str[1], wxT("%lx"), &tmp) != 1)
            return false;

        Set((unsigned char)(tmp >> 16),
            (unsigned char)(tmp >> 8),
            (unsigned char)tmp);
    }
    else if (wxTheColourDatabase) // a colour name ?
    {
        // we can't do
        // *this = wxTheColourDatabase->Find(str)
        // because this place can be called from constructor
        // and 'this' could not be available yet
        wxColour clr = wxTheColourDatabase->Find(str);
        if (clr.Ok())
            Set((unsigned char)clr.Red(),
                (unsigned char)clr.Green(),
                (unsigned char)clr.Blue());
    }

    if (Ok())
        return true;

    wxLogDebug(wxT("wxColour::Set - couldn't set to colour string '%s'"), str);
    return false;
}

wxString wxColourBase::GetAsString(long flags) const
{
    wxString colName;

    if (flags & wxC2S_NAME)
        colName = wxTheColourDatabase->FindName((const wxColour &)(*this)).MakeLower();

    if ( colName.empty() && (flags & wxC2S_CSS_SYNTAX) )
    {
        // no name for this colour; return it in CSS syntax
        colName.Printf(wxT("rgb(%d, %d, %d)"),
                       Red(), Green(), Blue());
    }
    else if ( colName.empty() && (flags & wxC2S_HTML_SYNTAX) )
    {
        // no name for this colour; return it in HTML syntax
        colName.Printf(wxT("#%02X%02X%02X"),
                       Red(), Green(), Blue());
    }

    // this function always returns a non-empty string
    wxASSERT_MSG(!colName.empty(),
                 wxT("Invalid wxColour -> wxString conversion flags"));

    return colName;
}

#if WXWIN_COMPATIBILITY_2_6

// static
wxColour wxColourBase::CreateByName(const wxString& name)
{
    return wxColour(name);
}

void wxColourBase::InitFromName(const wxString& col)
{
    Set(col);
}

#endif // WXWIN_COMPATIBILITY_2_6
