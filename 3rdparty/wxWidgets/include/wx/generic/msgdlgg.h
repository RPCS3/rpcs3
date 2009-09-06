/////////////////////////////////////////////////////////////////////////////
// Name:        wx/generic/msgdlgg.h
// Purpose:     Generic wxMessageDialog
// Author:      Julian Smart
// Modified by:
// Created:     01/02/97
// RCS-ID:      $Id: msgdlgg.h 49765 2007-11-09 18:32:38Z DE $
// Copyright:   (c) Julian Smart
// Licence:     wxWindows licence
/////////////////////////////////////////////////////////////////////////////

#ifndef __MSGDLGH_G__
#define __MSGDLGH_G__

#include "wx/defs.h"
#include "wx/dialog.h"

// type is an 'or' (|) of wxOK, wxCANCEL, wxYES_NO
// Returns wxYES/NO/OK/CANCEL

extern WXDLLEXPORT_DATA(const wxChar) wxMessageBoxCaptionStr[];

class WXDLLEXPORT wxGenericMessageDialog: public wxDialog, public wxMessageDialogBase
{
DECLARE_DYNAMIC_CLASS(wxGenericMessageDialog)

public:
    wxGenericMessageDialog(wxWindow *parent, const wxString& message,
        const wxString& caption = wxMessageBoxCaptionStr,
        long style = wxOK|wxCENTRE, const wxPoint& pos = wxDefaultPosition);

    void OnYes(wxCommandEvent& event);
    void OnNo(wxCommandEvent& event);
    void OnCancel(wxCommandEvent& event);

private:
    DECLARE_EVENT_TABLE()
};

#if (!defined( __WXMSW__ ) && !defined( __WXMAC__) && !defined(__WXPM__) && !defined(__WXCOCOA__)) || defined(__WXUNIVERSAL__)
#define wxMessageDialog wxGenericMessageDialog
#endif

#endif // __MSGDLGH_G__
