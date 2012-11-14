/////////////////////////////////////////////////////////////////////////////
// Name:        wx/msw/msgdlg.h
// Purpose:     wxMessageDialog class
// Author:      Julian Smart
// Modified by:
// Created:     01/02/97
// RCS-ID:      $Id: msgdlg.h 37164 2006-01-26 17:20:50Z ABX $
// Copyright:   (c) Julian Smart
// Licence:     wxWindows licence
/////////////////////////////////////////////////////////////////////////////

#ifndef _WX_MSGBOXDLG_H_
#define _WX_MSGBOXDLG_H_

#include "wx/defs.h"
#include "wx/dialog.h"

/*
 * Message box dialog
 */

extern WXDLLEXPORT_DATA(const wxChar) wxMessageBoxCaptionStr[];

class WXDLLEXPORT wxMessageDialog: public wxDialog, public wxMessageDialogBase
{
DECLARE_DYNAMIC_CLASS(wxMessageDialog)
protected:
    wxString    m_caption;
    wxString    m_message;
    wxWindow *  m_parent;
public:
    wxMessageDialog(wxWindow *parent, const wxString& message, const wxString& caption = wxMessageBoxCaptionStr,
        long style = wxOK|wxCENTRE, const wxPoint& pos = wxDefaultPosition);

    int ShowModal(void);

    DECLARE_NO_COPY_CLASS(wxMessageDialog)
};


#endif
    // _WX_MSGBOXDLG_H_
