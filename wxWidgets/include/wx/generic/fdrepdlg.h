/////////////////////////////////////////////////////////////////////////////
// Name:        wx/msw/fdrepdlg.h
// Purpose:     wxGenericFindReplaceDialog class
// Author:      Markus Greither
// Modified by:
// Created:     25/05/2001
// RCS-ID:
// Copyright:   (c) wxWidgets team
// Licence:     wxWindows licence
/////////////////////////////////////////////////////////////////////////////

#ifndef _WX_GENERIC_FDREPDLG_H_
#define _WX_GENERIC_FDREPDLG_H_

class WXDLLEXPORT wxCheckBox;
class WXDLLEXPORT wxRadioBox;
class WXDLLEXPORT wxTextCtrl;

// ----------------------------------------------------------------------------
// wxGenericFindReplaceDialog: dialog for searching / replacing text
// ----------------------------------------------------------------------------

class WXDLLEXPORT wxGenericFindReplaceDialog : public wxFindReplaceDialogBase
{
public:
    wxGenericFindReplaceDialog() { Init(); }

    wxGenericFindReplaceDialog(wxWindow *parent,
                               wxFindReplaceData *data,
                               const wxString& title,
                               int style = 0)
    {
        Init();

        (void)Create(parent, data, title, style);
    }

    bool Create(wxWindow *parent,
                wxFindReplaceData *data,
                const wxString& title,
                int style = 0);

protected:
    void Init();

    void SendEvent(const wxEventType& evtType);

    void OnFind(wxCommandEvent& event);
    void OnReplace(wxCommandEvent& event);
    void OnReplaceAll(wxCommandEvent& event);
    void OnCancel(wxCommandEvent& event);

    void OnUpdateFindUI(wxUpdateUIEvent& event);

    void OnCloseWindow(wxCloseEvent& event);

    wxCheckBox *m_chkCase,
               *m_chkWord;

    wxRadioBox *m_radioDir;

    wxTextCtrl *m_textFind,
               *m_textRepl;

private:
    DECLARE_DYNAMIC_CLASS(wxGenericFindReplaceDialog)

    DECLARE_EVENT_TABLE()
};

#endif // _WX_GENERIC_FDREPDLG_H_
