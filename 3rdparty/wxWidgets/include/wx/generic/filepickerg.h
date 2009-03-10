/////////////////////////////////////////////////////////////////////////////
// Name:        wx/generic/filepickerg.h
// Purpose:     wxGenericFileDirButton, wxGenericFileButton, wxGenericDirButton
// Author:      Francesco Montorsi
// Modified by:
// Created:     14/4/2006
// Copyright:   (c) Francesco Montorsi
// RCS-ID:      $Id: filepickerg.h 40100 2006-07-15 15:13:04Z VS $
// Licence:     wxWindows Licence
/////////////////////////////////////////////////////////////////////////////

#ifndef _WX_FILEDIRPICKER_H_
#define _WX_FILEDIRPICKER_H_

#include "wx/button.h"
#include "wx/filedlg.h"
#include "wx/dirdlg.h"


extern WXDLLEXPORT_DATA(const wxEventType) wxEVT_COMMAND_DIRPICKER_CHANGED;
extern WXDLLEXPORT_DATA(const wxEventType) wxEVT_COMMAND_FILEPICKER_CHANGED;


//-----------------------------------------------------------------------------
// wxGenericFileDirButton: a button which brings up a wx{File|Dir}Dialog
//-----------------------------------------------------------------------------

class WXDLLIMPEXP_CORE wxGenericFileDirButton : public wxButton,
                                                public wxFileDirPickerWidgetBase
{
public:
    wxGenericFileDirButton() { }
    wxGenericFileDirButton(wxWindow *parent,
                           wxWindowID id,
                           const wxString& label = wxFilePickerWidgetLabel,
                           const wxString& path = wxEmptyString,
                           const wxString &message = wxFileSelectorPromptStr,
                           const wxString &wildcard = wxFileSelectorDefaultWildcardStr,
                           const wxPoint& pos = wxDefaultPosition,
                           const wxSize& size = wxDefaultSize,
                           long style = 0,
                           const wxValidator& validator = wxDefaultValidator,
                           const wxString& name = wxFilePickerWidgetNameStr)
    {
        Create(parent, id, label, path, message, wildcard,
               pos, size, style, validator, name);
    }

    virtual ~wxGenericFileDirButton() {}

    virtual wxControl *AsControl() { return this; }

public:     // overrideable

    virtual wxDialog *CreateDialog() = 0;

    virtual wxWindow *GetDialogParent()
        { return GetParent(); }

    virtual wxEventType GetEventType() const = 0;

public:

    bool Create(wxWindow *parent, wxWindowID id,
           const wxString& label = wxFilePickerWidgetLabel,
           const wxString& path = wxEmptyString,
           const wxString &message = wxFileSelectorPromptStr,
           const wxString &wildcard = wxFileSelectorDefaultWildcardStr,
           const wxPoint& pos = wxDefaultPosition,
           const wxSize& size = wxDefaultSize,
           long style = 0,
           const wxValidator& validator = wxDefaultValidator,
           const wxString& name = wxFilePickerWidgetNameStr);

    // event handler for the click
    void OnButtonClick(wxCommandEvent &);

protected:
    wxString m_message, m_wildcard;
};


//-----------------------------------------------------------------------------
// wxGenericFileButton: a button which brings up a wxFileDialog
//-----------------------------------------------------------------------------

#define wxFILEBTN_DEFAULT_STYLE                     (wxFLP_OPEN)

class WXDLLIMPEXP_CORE wxGenericFileButton : public wxGenericFileDirButton
{
public:
    wxGenericFileButton() {}
    wxGenericFileButton(wxWindow *parent,
                        wxWindowID id,
                        const wxString& label = wxFilePickerWidgetLabel,
                        const wxString& path = wxEmptyString,
                        const wxString &message = wxFileSelectorPromptStr,
                        const wxString &wildcard = wxFileSelectorDefaultWildcardStr,
                        const wxPoint& pos = wxDefaultPosition,
                        const wxSize& size = wxDefaultSize,
                        long style = wxFILEBTN_DEFAULT_STYLE,
                        const wxValidator& validator = wxDefaultValidator,
                        const wxString& name = wxFilePickerWidgetNameStr)
    {
        Create(parent, id, label, path, message, wildcard,
               pos, size, style, validator, name);
    }

public:     // overrideable

    virtual long GetDialogStyle() const
    {
        long filedlgstyle = 0;

        if (this->HasFlag(wxFLP_OPEN))
            filedlgstyle |= wxFD_OPEN;
        if (this->HasFlag(wxFLP_SAVE))
            filedlgstyle |= wxFD_SAVE;
        if (this->HasFlag(wxFLP_OVERWRITE_PROMPT))
            filedlgstyle |= wxFD_OVERWRITE_PROMPT;
        if (this->HasFlag(wxFLP_FILE_MUST_EXIST))
            filedlgstyle |= wxFD_FILE_MUST_EXIST;
        if (this->HasFlag(wxFLP_CHANGE_DIR))
            filedlgstyle |= wxFD_CHANGE_DIR;

        return filedlgstyle;
    }

    virtual wxDialog *CreateDialog()
    {
        wxFileDialog *p = new wxFileDialog(GetDialogParent(), m_message,
                                    wxEmptyString, wxEmptyString,
                                    m_wildcard, GetDialogStyle());

        // this sets both the default folder and the default file of the dialog
        p->SetPath(m_path);
        return p;
    }

    wxEventType GetEventType() const
        { return wxEVT_COMMAND_FILEPICKER_CHANGED; }

protected:
    void UpdateDialogPath(wxDialog *p)
        { wxStaticCast(p, wxFileDialog)->SetPath(m_path); }
    void UpdatePathFromDialog(wxDialog *p)
        { m_path = wxStaticCast(p, wxFileDialog)->GetPath(); }

private:
    DECLARE_DYNAMIC_CLASS(wxGenericFileButton)
};


//-----------------------------------------------------------------------------
// wxGenericDirButton: a button which brings up a wxDirDialog
//-----------------------------------------------------------------------------

#define wxDIRBTN_DEFAULT_STYLE                     0

class WXDLLIMPEXP_CORE wxGenericDirButton : public wxGenericFileDirButton
{
public:
    wxGenericDirButton() {}
    wxGenericDirButton(wxWindow *parent,
                       wxWindowID id,
                       const wxString& label = wxDirPickerWidgetLabel,
                       const wxString& path = wxEmptyString,
                       const wxString &message = wxDirSelectorPromptStr,
                       const wxPoint& pos = wxDefaultPosition,
                       const wxSize& size = wxDefaultSize,
                       long style = wxDIRBTN_DEFAULT_STYLE,
                       const wxValidator& validator = wxDefaultValidator,
                       const wxString& name = wxDirPickerWidgetNameStr)
    {
        Create(parent, id, label, path, message, wxEmptyString,
               pos, size, style, validator, name);
    }

public:     // overrideable

    virtual long GetDialogStyle() const
    {
        long dirdlgstyle = wxDD_DEFAULT_STYLE;

        if (this->HasFlag(wxDIRP_DIR_MUST_EXIST))
            dirdlgstyle |= wxDD_DIR_MUST_EXIST;
        if (this->HasFlag(wxDIRP_CHANGE_DIR))
            dirdlgstyle |= wxDD_CHANGE_DIR;

        return dirdlgstyle;
    }

    virtual wxDialog *CreateDialog()
    {
        return new wxDirDialog(GetDialogParent(), m_message, m_path,
                                   GetDialogStyle());
    }

    wxEventType GetEventType() const
        { return wxEVT_COMMAND_DIRPICKER_CHANGED; }

protected:
    void UpdateDialogPath(wxDialog *p)
        { wxStaticCast(p, wxDirDialog)->SetPath(m_path); }
    void UpdatePathFromDialog(wxDialog *p)
        { m_path = wxStaticCast(p, wxDirDialog)->GetPath(); }

private:
    DECLARE_DYNAMIC_CLASS(wxGenericDirButton)
};


#endif // _WX_FILEDIRPICKER_H_
