/////////////////////////////////////////////////////////////////////////////
// Name:        wx/msw/dialog.h
// Purpose:     wxDialog class
// Author:      Julian Smart
// Modified by:
// Created:     01/02/97
// RCS-ID:      $Id: dialog.h 53135 2008-04-12 02:31:04Z VZ $
// Copyright:   (c) Julian Smart
// Licence:     wxWindows licence
/////////////////////////////////////////////////////////////////////////////

#ifndef _WX_DIALOG_H_
#define _WX_DIALOG_H_

#include "wx/panel.h"

extern WXDLLEXPORT_DATA(const wxChar) wxDialogNameStr[];

class WXDLLIMPEXP_FWD_CORE wxDialogModalData;

#if wxUSE_TOOLBAR && (defined(__SMARTPHONE__) || defined(__POCKETPC__))
class WXDLLIMPEXP_FWD_CORE wxToolBar;
extern WXDLLEXPORT_DATA(const wxChar) wxToolBarNameStr[];
#endif

// Dialog boxes
class WXDLLEXPORT wxDialog : public wxDialogBase
{
public:
    wxDialog() { Init(); }

    // full ctor
    wxDialog(wxWindow *parent, wxWindowID id,
             const wxString& title,
             const wxPoint& pos = wxDefaultPosition,
             const wxSize& size = wxDefaultSize,
             long style = wxDEFAULT_DIALOG_STYLE,
             const wxString& name = wxDialogNameStr)
    {
        Init();

        (void)Create(parent, id, title, pos, size, style, name);
    }

    bool Create(wxWindow *parent, wxWindowID id,
                const wxString& title,
                const wxPoint& pos = wxDefaultPosition,
                const wxSize& size = wxDefaultSize,
                long style = wxDEFAULT_DIALOG_STYLE,
                const wxString& name = wxDialogNameStr);

    virtual ~wxDialog();

    // return true if we're showing the dialog modally
    virtual bool IsModal() const { return m_modalData != NULL; }

    // show the dialog modally and return the value passed to EndModal()
    virtual int ShowModal();

    // may be called to terminate the dialog with the given return code
    virtual void EndModal(int retCode);


    // we treat dialog toolbars specially under Windows CE
#if wxUSE_TOOLBAR && defined(__POCKETPC__)
    // create main toolbar by calling OnCreateToolBar()
    virtual wxToolBar* CreateToolBar(long style = -1,
                                     wxWindowID winid = wxID_ANY,
                                     const wxString& name = wxToolBarNameStr);
    // return a new toolbar
    virtual wxToolBar *OnCreateToolBar(long style,
                                       wxWindowID winid,
                                       const wxString& name );

    // get the main toolbar
    wxToolBar *GetToolBar() const { return m_dialogToolBar; }
#endif // wxUSE_TOOLBAR && __POCKETPC__


    // implementation only from now on
    // -------------------------------

    // override some base class virtuals
    virtual bool Show(bool show = true);

    virtual void Raise();

#ifdef __POCKETPC__
    // Responds to the OK button in a PocketPC titlebar. This
    // can be overridden, or you can change the id used for
    // sending the event with SetAffirmativeId. Returns false
    // if the event was not processed.
    virtual bool DoOK();
#endif

    // Windows callbacks
    WXLRESULT MSWWindowProc(WXUINT message, WXWPARAM wParam, WXLPARAM lParam);

#if WXWIN_COMPATIBILITY_2_6
    // use the other ctor
    wxDEPRECATED( wxDialog(wxWindow *parent,
             const wxString& title, bool modal,
             int x = wxDefaultCoord, int y = wxDefaultCoord, int width = 500, int height = 500,
             long style = wxDEFAULT_DIALOG_STYLE,
             const wxString& name = wxDialogNameStr) );

    // just call Show() or ShowModal()
    wxDEPRECATED( void SetModal(bool flag) );

    // use IsModal()
    wxDEPRECATED( bool IsModalShowing() const );
#endif // WXWIN_COMPATIBILITY_2_6

protected:
    // find the window to use as parent for this dialog if none has been
    // specified explicitly by the user
    //
    // may return NULL
    wxWindow *FindSuitableParent() const;

    // common part of all ctors
    void Init();

private:
    wxWindow*   m_oldFocus;
    bool        m_endModalCalled; // allow for closing within InitDialog

#if wxUSE_TOOLBAR && defined(__POCKETPC__)
    wxToolBar*  m_dialogToolBar;
#endif

    // this pointer is non-NULL only while the modal event loop is running
    wxDialogModalData *m_modalData;

    DECLARE_DYNAMIC_CLASS(wxDialog)
    DECLARE_NO_COPY_CLASS(wxDialog)
};

#endif
    // _WX_DIALOG_H_
