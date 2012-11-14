///////////////////////////////////////////////////////////////////////////////
// Name:        wx/generic/aboutdlgg.h
// Purpose:     generic wxAboutBox() implementation
// Author:      Vadim Zeitlin
// Created:     2006-10-07
// RCS-ID:      $Id: aboutdlgg.h 49804 2007-11-10 01:09:42Z VZ $
// Copyright:   (c) 2006 Vadim Zeitlin <vadim@wxwindows.org>
// Licence:     wxWindows licence
///////////////////////////////////////////////////////////////////////////////

#ifndef _WX_GENERIC_ABOUTDLGG_H_
#define _WX_GENERIC_ABOUTDLGG_H_

#include "wx/defs.h"

#if wxUSE_ABOUTDLG

#include "wx/dialog.h"

class WXDLLIMPEXP_FWD_ADV wxAboutDialogInfo;
class WXDLLIMPEXP_FWD_CORE wxSizer;
class WXDLLIMPEXP_FWD_CORE wxSizerFlags;

// ----------------------------------------------------------------------------
// wxGenericAboutDialog: generic "About" dialog implementation
// ----------------------------------------------------------------------------

class WXDLLIMPEXP_ADV wxGenericAboutDialog : public wxDialog
{
public:
    // constructors and Create() method
    // --------------------------------

    // default ctor, you must use Create() to really initialize the dialog
    wxGenericAboutDialog() { Init(); }

    // ctor which fully initializes the object
    wxGenericAboutDialog(const wxAboutDialogInfo& info)
    {
        Init();

        (void)Create(info);
    }

    // this method must be called if and only if the default ctor was used
    bool Create(const wxAboutDialogInfo& info);

protected:
    // this virtual method may be overridden to add some more controls to the
    // dialog
    //
    // notice that for this to work you must call Create() from the derived
    // class ctor and not use the base class ctor directly as otherwise the
    // virtual function of the derived class wouldn't be called
    virtual void DoAddCustomControls() { }

    // add arbitrary control to the text sizer contents with the specified
    // flags
    void AddControl(wxWindow *win, const wxSizerFlags& flags);

    // add arbitrary control to the text sizer contents and center it
    void AddControl(wxWindow *win);

    // add the text, if it's not empty, to the text sizer contents
    void AddText(const wxString& text);

#if wxUSE_COLLPANE
    // add a wxCollapsiblePane containing the given text
    void AddCollapsiblePane(const wxString& title, const wxString& text);
#endif // wxUSE_COLLPANE

private:
    // common part of all ctors
    void Init() { m_sizerText = NULL; }


    wxSizer *m_sizerText;
};

// unlike wxAboutBox which can show either the native or generic about dialog,
// this function always shows the generic one
WXDLLIMPEXP_ADV void wxGenericAboutBox(const wxAboutDialogInfo& info);

#endif // wxUSE_ABOUTDLG

#endif // _WX_GENERIC_ABOUTDLGG_H_

