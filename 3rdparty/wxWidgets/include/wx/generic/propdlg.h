/////////////////////////////////////////////////////////////////////////////
// Name:        wx/generic/propdlg.h
// Purpose:     wxPropertySheetDialog
// Author:      Julian Smart
// Modified by:
// Created:     2005-03-12
// RCS-ID:      $Id: propdlg.h 49804 2007-11-10 01:09:42Z VZ $
// Copyright:   (c) Julian Smart
// Licence:     wxWindows licence
/////////////////////////////////////////////////////////////////////////////

#ifndef _WX_PROPDLG_H_
#define _WX_PROPDLG_H_

#include "wx/defs.h"

#if wxUSE_BOOKCTRL

#include "wx/dialog.h"

class WXDLLIMPEXP_FWD_CORE wxBookCtrlBase;

//-----------------------------------------------------------------------------
// wxPropertySheetDialog
// A platform-independent properties dialog.
//
//   * on PocketPC, a flat-look 'property sheet' notebook will be used, with
//     no OK/Cancel/Help buttons
//   * on other platforms, a normal notebook will be used, with standard buttons
//
// To use this class, call Create from your derived class.
// Then create pages and add to the book control. Finally call CreateButtons and
// LayoutDialog.
//
// For example:
//
// MyPropertySheetDialog::Create(...)
// {
//     wxPropertySheetDialog::Create(...);
//
//     // Add page
//     wxPanel* panel = new wxPanel(GetBookCtrl(), ...);
//     GetBookCtrl()->AddPage(panel, wxT("General"));
//
//     CreateButtons();
//     LayoutDialog();
// }
//
// Override CreateBookCtrl and AddBookCtrl to create and add a different
// kind of book control.
//-----------------------------------------------------------------------------

// Use the platform default
#define wxPROPSHEET_DEFAULT         0x0001

// Use a notebook
#define wxPROPSHEET_NOTEBOOK        0x0002

// Use a toolbook
#define wxPROPSHEET_TOOLBOOK        0x0004

// Use a choicebook
#define wxPROPSHEET_CHOICEBOOK      0x0008

// Use a listbook
#define wxPROPSHEET_LISTBOOK        0x0010

// Use a wxButtonToolBar toolbook
#define wxPROPSHEET_BUTTONTOOLBOOK  0x0020

// Use a treebook
#define wxPROPSHEET_TREEBOOK        0x0040

// Shrink dialog to fit current page
#define wxPROPSHEET_SHRINKTOFIT     0x0100

class WXDLLIMPEXP_ADV wxPropertySheetDialog : public wxDialog
{
public:
    wxPropertySheetDialog() : wxDialog() { Init(); }

    wxPropertySheetDialog(wxWindow* parent, wxWindowID id,
                       const wxString& title,
                       const wxPoint& pos = wxDefaultPosition,
                       const wxSize& sz = wxDefaultSize,
                       long style = wxDEFAULT_DIALOG_STYLE,
                       const wxString& name = wxDialogNameStr)
    {
        Init();
        Create(parent, id, title, pos, sz, style, name);
    }

    bool Create(wxWindow* parent, wxWindowID id,
                       const wxString& title,
                       const wxPoint& pos = wxDefaultPosition,
                       const wxSize& sz = wxDefaultSize,
                       long style = wxDEFAULT_DIALOG_STYLE,
                       const wxString& name = wxDialogNameStr);

//// Accessors

    // Set and get the notebook
    void SetBookCtrl(wxBookCtrlBase* book) { m_bookCtrl = book; }
    wxBookCtrlBase* GetBookCtrl() const { return m_bookCtrl; }

    // Set and get the inner sizer
    void SetInnerSize(wxSizer* sizer) { m_innerSizer = sizer; }
    wxSizer* GetInnerSizer() const { return m_innerSizer ; }

    // Set and get the book style
    void SetSheetStyle(long sheetStyle) { m_sheetStyle = sheetStyle; }
    long GetSheetStyle() const { return m_sheetStyle ; }

    // Set and get the border around the whole dialog
    void SetSheetOuterBorder(int border) { m_sheetOuterBorder = border; }
    int GetSheetOuterBorder() const { return m_sheetOuterBorder ; }

    // Set and get the border around the book control only
    void SetSheetInnerBorder(int border) { m_sheetInnerBorder = border; }
    int GetSheetInnerBorder() const { return m_sheetInnerBorder ; }

/// Operations

    // Creates the buttons (none on PocketPC)
    virtual void CreateButtons(int flags = wxOK|wxCANCEL);

    // Lay out the dialog, to be called after pages have been created
    virtual void LayoutDialog(int centreFlags = wxBOTH);

/// Implementation

    // Creates the book control. If you want to use a different kind of
    // control, override.
    virtual wxBookCtrlBase* CreateBookCtrl();

    // Adds the book control to the inner sizer.
    virtual void AddBookCtrl(wxSizer* sizer);

    // Set the focus
    void OnActivate(wxActivateEvent& event);

    // Resize dialog if necessary
    void OnIdle(wxIdleEvent& event);

private:
    void Init();

protected:
    wxBookCtrlBase* m_bookCtrl;
    wxSizer*        m_innerSizer; // sizer for extra space
    long            m_sheetStyle;
    int             m_sheetOuterBorder;
    int             m_sheetInnerBorder;
    int             m_selectedPage;

    DECLARE_DYNAMIC_CLASS(wxPropertySheetDialog)
    DECLARE_EVENT_TABLE()
};

#endif // wxUSE_BOOKCTRL

#endif // _WX_PROPDLG_H_

