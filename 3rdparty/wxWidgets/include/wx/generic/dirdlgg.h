/////////////////////////////////////////////////////////////////////////////
// Name:        wx/generic/dirdlgg.h
// Purpose:     wxGenericDirCtrl class
//              Builds on wxDirCtrl class written by Robert Roebling for the
//              wxFile application, modified by Harm van der Heijden.
//              Further modified for Windows.
// Author:      Robert Roebling, Harm van der Heijden, Julian Smart et al
// Modified by:
// Created:     21/3/2000
// RCS-ID:      $Id: dirdlgg.h 39402 2006-05-28 23:32:12Z VZ $
// Copyright:   (c) Robert Roebling, Harm van der Heijden, Julian Smart
// Licence:     wxWindows licence
/////////////////////////////////////////////////////////////////////////////

#ifndef _WX_DIRDLGG_H_
#define _WX_DIRDLGG_H_

class WXDLLEXPORT wxGenericDirCtrl;
class WXDLLEXPORT wxTextCtrl;
class WXDLLEXPORT wxTreeEvent;

// we may be included directly as well as from wx/dirdlg.h (FIXME)
extern WXDLLEXPORT_DATA(const wxChar) wxDirDialogNameStr[];
extern WXDLLEXPORT_DATA(const wxChar) wxDirSelectorPromptStr[];

#ifndef wxDD_DEFAULT_STYLE
#ifdef __WXWINCE__
    #define wxDD_DEFAULT_STYLE      wxDEFAULT_DIALOG_STYLE
#else
    #define wxDD_DEFAULT_STYLE      (wxDEFAULT_DIALOG_STYLE|wxRESIZE_BORDER)
#endif
#endif

#include "wx/dialog.h"

//-----------------------------------------------------------------------------
// wxGenericDirDialog
//-----------------------------------------------------------------------------

class WXDLLEXPORT wxGenericDirDialog : public wxDirDialogBase
{
public:
    wxGenericDirDialog() : wxDirDialogBase() { }

    wxGenericDirDialog(wxWindow* parent,
                       const wxString& title = wxDirSelectorPromptStr,
                       const wxString& defaultPath = wxEmptyString,
                       long style = wxDD_DEFAULT_STYLE,
                       const wxPoint& pos = wxDefaultPosition,
                       const wxSize& sz = wxDefaultSize,//Size(450, 550),
                       const wxString& name = wxDirDialogNameStr);

    bool Create(wxWindow* parent,
                const wxString& title = wxDirSelectorPromptStr,
                const wxString& defaultPath = wxEmptyString,
                long style = wxDD_DEFAULT_STYLE,
                const wxPoint& pos = wxDefaultPosition,
                const wxSize& sz = wxDefaultSize,//Size(450, 550),
                       const wxString& name = wxDirDialogNameStr);

    //// Accessors
    void SetPath(const wxString& path);
    wxString GetPath() const;

    //// Overrides
    virtual int ShowModal();
    virtual void EndModal(int retCode);

    // this one is specific to wxGenericDirDialog
    wxTextCtrl* GetInputCtrl() const { return m_input; }

protected:
    //// Event handlers
    void OnCloseWindow(wxCloseEvent& event);
    void OnOK(wxCommandEvent& event);
    void OnTreeSelected(wxTreeEvent &event);
    void OnTreeKeyDown(wxTreeEvent &event);
    void OnNew(wxCommandEvent& event);
    void OnGoHome(wxCommandEvent& event);
    void OnShowHidden(wxCommandEvent& event);

    wxGenericDirCtrl* m_dirCtrl;
    wxTextCtrl*       m_input;

    DECLARE_EVENT_TABLE()
    DECLARE_DYNAMIC_CLASS(wxGenericDirDialog)
};

#endif // _WX_DIRDLGG_H_
