/////////////////////////////////////////////////////////////////////////////
// Name:        wx/msw/fdrepdlg.h
// Purpose:     wxFindReplaceDialog class
// Author:      Markus Greither
// Modified by: 31.07.01: VZ: integrated into wxWidgets
// Created:     23/03/2001
// RCS-ID:
// Copyright:   (c) Markus Greither
// Licence:     wxWindows licence
/////////////////////////////////////////////////////////////////////////////

#ifndef _WX_MSW_FDREPDLG_H_
#define _WX_MSW_FDREPDLG_H_

// ----------------------------------------------------------------------------
// wxFindReplaceDialog: dialog for searching / replacing text
// ----------------------------------------------------------------------------

class WXDLLEXPORT wxFindReplaceDialog : public wxFindReplaceDialogBase
{
public:
    // ctors and such
    wxFindReplaceDialog() { Init(); }
    wxFindReplaceDialog(wxWindow *parent,
                        wxFindReplaceData *data,
                        const wxString &title,
                        int style = 0);

    bool Create(wxWindow *parent,
                wxFindReplaceData *data,
                const wxString &title,
                int style = 0);

    virtual ~wxFindReplaceDialog();

    // implementation only from now on

    wxFindReplaceDialogImpl *GetImpl() const { return m_impl; }

    // override some base class virtuals
    virtual bool Show(bool show = true);
    virtual void SetTitle( const wxString& title);
    virtual wxString GetTitle() const;

protected:
    virtual void DoGetSize(int *width, int *height) const;
    virtual void DoGetClientSize(int *width, int *height) const;
    virtual void DoSetSize(int x, int y,
                           int width, int height,
                           int sizeFlags = wxSIZE_AUTO);

    void Init();

    wxString                m_title;

    wxFindReplaceDialogImpl *m_impl;

    DECLARE_DYNAMIC_CLASS(wxFindReplaceDialog)
    DECLARE_NO_COPY_CLASS(wxFindReplaceDialog)
};

#endif // _WX_MSW_FDREPDLG_H_
