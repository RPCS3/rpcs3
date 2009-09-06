/////////////////////////////////////////////////////////////////////////////
// Name:        stattext.h
// Purpose:     wxStaticText base header
// Author:      Julian Smart
// Modified by:
// Created:
// Copyright:   (c) Julian Smart
// RCS-ID:      $Id: stattext.h 37066 2006-01-23 03:27:34Z MR $
// Licence:     wxWindows licence
/////////////////////////////////////////////////////////////////////////////

#ifndef _WX_STATTEXT_H_BASE_
#define _WX_STATTEXT_H_BASE_

#include "wx/defs.h"

#if wxUSE_STATTEXT

#include "wx/control.h"

extern WXDLLEXPORT_DATA(const wxChar) wxStaticTextNameStr[];

class WXDLLEXPORT wxStaticTextBase : public wxControl
{
public:
    wxStaticTextBase() { }

    // in wxGTK wxStaticText doesn't derive from wxStaticTextBase so we have to
    // declare this function directly in gtk header
#if !defined(__WXGTK__) || defined(__WXUNIVERSAL__)
    // wrap the text of the control so that no line is longer than the given
    // width (if possible: this function won't break words)
    //
    // NB: implemented in dlgcmn.cpp for now
    void Wrap(int width);
#endif // ! native __WXGTK__

    // overriden base virtuals
    virtual bool AcceptsFocus() const { return false; }
    virtual bool HasTransparentBackground() { return true; }

private:
    DECLARE_NO_COPY_CLASS(wxStaticTextBase)
};

#if defined(__WXUNIVERSAL__)
    #include "wx/univ/stattext.h"
#elif defined(__WXMSW__)
    #include "wx/msw/stattext.h"
#elif defined(__WXMOTIF__)
    #include "wx/motif/stattext.h"
#elif defined(__WXGTK20__)
    #include "wx/gtk/stattext.h"
#elif defined(__WXGTK__)
    #include "wx/gtk1/stattext.h"
#elif defined(__WXMAC__)
    #include "wx/mac/stattext.h"
#elif defined(__WXCOCOA__)
    #include "wx/cocoa/stattext.h"
#elif defined(__WXPM__)
    #include "wx/os2/stattext.h"
#elif defined(__WXPALMOS__)
    #include "wx/palmos/stattext.h"
#endif

#endif // wxUSE_STATTEXT

#endif
    // _WX_STATTEXT_H_BASE_
