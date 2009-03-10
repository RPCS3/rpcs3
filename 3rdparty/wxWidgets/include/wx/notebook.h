///////////////////////////////////////////////////////////////////////////////
// Name:        wx/notebook.h
// Purpose:     wxNotebook interface
// Author:      Vadim Zeitlin
// Modified by:
// Created:     01.02.01
// RCS-ID:      $Id: notebook.h 42152 2006-10-20 09:16:41Z VZ $
// Copyright:   (c) 1996-2000 Vadim Zeitlin
// Licence:     wxWindows licence
///////////////////////////////////////////////////////////////////////////////

#ifndef _WX_NOTEBOOK_H_BASE_
#define _WX_NOTEBOOK_H_BASE_

// ----------------------------------------------------------------------------
// headers
// ----------------------------------------------------------------------------

#include "wx/defs.h"

#if wxUSE_NOTEBOOK

#include "wx/bookctrl.h"

// ----------------------------------------------------------------------------
// constants
// ----------------------------------------------------------------------------

// wxNotebook hit results, use wxBK_HITTEST so other book controls can share them
// if wxUSE_NOTEBOOK is disabled
enum
{
    wxNB_HITTEST_NOWHERE = wxBK_HITTEST_NOWHERE,
    wxNB_HITTEST_ONICON  = wxBK_HITTEST_ONICON,
    wxNB_HITTEST_ONLABEL = wxBK_HITTEST_ONLABEL,
    wxNB_HITTEST_ONITEM  = wxBK_HITTEST_ONITEM,
    wxNB_HITTEST_ONPAGE  = wxBK_HITTEST_ONPAGE
};

// wxNotebook flags

// use common book wxBK_* flags for describing alignment
#define wxNB_DEFAULT          wxBK_DEFAULT
#define wxNB_TOP              wxBK_TOP
#define wxNB_BOTTOM           wxBK_BOTTOM
#define wxNB_LEFT             wxBK_LEFT
#define wxNB_RIGHT            wxBK_RIGHT

#define wxNB_FIXEDWIDTH       0x0100
#define wxNB_MULTILINE        0x0200
#define wxNB_NOPAGETHEME      0x0400
#define wxNB_FLAT             0x0800


typedef wxWindow wxNotebookPage;  // so far, any window can be a page

extern WXDLLEXPORT_DATA(const wxChar) wxNotebookNameStr[];

#if WXWIN_COMPATIBILITY_2_4
    #define wxNOTEBOOK_NAME wxNotebookNameStr
#endif

// ----------------------------------------------------------------------------
// wxNotebookBase: define wxNotebook interface
// ----------------------------------------------------------------------------

class WXDLLEXPORT wxNotebookBase : public wxBookCtrlBase
{
public:
    // ctors
    // -----

    wxNotebookBase() { }

    wxNotebookBase(wxWindow *parent,
                   wxWindowID winid,
                   const wxPoint& pos = wxDefaultPosition,
                   const wxSize& size = wxDefaultSize,
                   long style = 0,
                   const wxString& name = wxNotebookNameStr) ;

    // wxNotebook-specific additions to wxBookCtrlBase interface
    // ---------------------------------------------------------

    // get the number of rows for a control with wxNB_MULTILINE style (not all
    // versions support it - they will always return 1 then)
    virtual int GetRowCount() const { return 1; }

    // set the padding between tabs (in pixels)
    virtual void SetPadding(const wxSize& padding) = 0;

    // set the size of the tabs for wxNB_FIXEDWIDTH controls
    virtual void SetTabSize(const wxSize& sz) = 0;



    // implement some base class functions
    virtual wxSize CalcSizeFromPage(const wxSize& sizePage) const;

    // On platforms that support it, get the theme page background colour, else invalid colour
    virtual wxColour GetThemeBackgroundColour() const { return wxNullColour; }


    // send wxEVT_COMMAND_NOTEBOOK_PAGE_CHANGING/ED events

    // returns false if the change to nPage is vetoed by the program
    bool SendPageChangingEvent(int nPage);

    // sends the event about page change from old to new (or GetSelection() if
    // new is -1)
    void SendPageChangedEvent(int nPageOld, int nPageNew = -1);


protected:
    DECLARE_NO_COPY_CLASS(wxNotebookBase)
};

// ----------------------------------------------------------------------------
// notebook event class and related stuff
// ----------------------------------------------------------------------------

class WXDLLEXPORT wxNotebookEvent : public wxBookCtrlBaseEvent
{
public:
    wxNotebookEvent(wxEventType commandType = wxEVT_NULL, int winid = 0,
                    int nSel = -1, int nOldSel = -1)
        : wxBookCtrlBaseEvent(commandType, winid, nSel, nOldSel)
    {
    }

    wxNotebookEvent(const wxNotebookEvent& event)
        : wxBookCtrlBaseEvent(event)
    {
    }

    virtual wxEvent *Clone() const { return new wxNotebookEvent(*this); }

private:
    DECLARE_DYNAMIC_CLASS_NO_ASSIGN(wxNotebookEvent)
};

BEGIN_DECLARE_EVENT_TYPES()
    DECLARE_EVENT_TYPE(wxEVT_COMMAND_NOTEBOOK_PAGE_CHANGED, 802)
    DECLARE_EVENT_TYPE(wxEVT_COMMAND_NOTEBOOK_PAGE_CHANGING, 803)
END_DECLARE_EVENT_TYPES()

typedef void (wxEvtHandler::*wxNotebookEventFunction)(wxNotebookEvent&);

#define wxNotebookEventHandler(func) \
    (wxObjectEventFunction)(wxEventFunction)wxStaticCastEvent(wxNotebookEventFunction, &func)

#define EVT_NOTEBOOK_PAGE_CHANGED(winid, fn) \
    wx__DECLARE_EVT1(wxEVT_COMMAND_NOTEBOOK_PAGE_CHANGED, winid, wxNotebookEventHandler(fn))

#define EVT_NOTEBOOK_PAGE_CHANGING(winid, fn) \
    wx__DECLARE_EVT1(wxEVT_COMMAND_NOTEBOOK_PAGE_CHANGING, winid, wxNotebookEventHandler(fn))

// ----------------------------------------------------------------------------
// wxNotebook class itself
// ----------------------------------------------------------------------------

#if defined(__WXUNIVERSAL__)
    #include "wx/univ/notebook.h"
#elif defined(__WXMSW__)
    #include  "wx/msw/notebook.h"
#elif defined(__WXMOTIF__)
    #include  "wx/generic/notebook.h"
#elif defined(__WXGTK20__)
    #include  "wx/gtk/notebook.h"
#elif defined(__WXGTK__)
    #include  "wx/gtk1/notebook.h"
#elif defined(__WXMAC__)
    #include  "wx/mac/notebook.h"
#elif defined(__WXCOCOA__)
    #include  "wx/cocoa/notebook.h"
#elif defined(__WXPM__)
    #include  "wx/os2/notebook.h"
#endif

#endif // wxUSE_NOTEBOOK

#endif
    // _WX_NOTEBOOK_H_BASE_
