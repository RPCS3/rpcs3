///////////////////////////////////////////////////////////////////////////////
// Name:        wx/toolbook.h
// Purpose:     wxToolbook: wxToolBar and wxNotebook combination
// Author:      Julian Smart
// Modified by:
// Created:     2006-01-29
// RCS-ID:      $Id: toolbook.h 49804 2007-11-10 01:09:42Z VZ $
// Copyright:   (c) 2006 Julian Smart
// Licence:     wxWindows licence
///////////////////////////////////////////////////////////////////////////////

#ifndef _WX_TOOLBOOK_H_
#define _WX_TOOLBOOK_H_

#include "wx/defs.h"

#if wxUSE_TOOLBOOK

#include "wx/bookctrl.h"

class WXDLLIMPEXP_FWD_CORE wxToolBarBase;
class WXDLLIMPEXP_FWD_CORE wxCommandEvent;

extern WXDLLIMPEXP_CORE const wxEventType wxEVT_COMMAND_TOOLBOOK_PAGE_CHANGED;
extern WXDLLIMPEXP_CORE const wxEventType wxEVT_COMMAND_TOOLBOOK_PAGE_CHANGING;


// Use wxButtonToolBar
#define wxBK_BUTTONBAR            0x0100

// ----------------------------------------------------------------------------
// wxToolbook
// ----------------------------------------------------------------------------

class WXDLLEXPORT wxToolbook : public wxBookCtrlBase
{
public:
    wxToolbook()
    {
        Init();
    }

    wxToolbook(wxWindow *parent,
               wxWindowID id,
               const wxPoint& pos = wxDefaultPosition,
               const wxSize& size = wxDefaultSize,
               long style = 0,
               const wxString& name = wxEmptyString)
    {
        Init();

        (void)Create(parent, id, pos, size, style, name);
    }

    // quasi ctor
    bool Create(wxWindow *parent,
                wxWindowID id,
                const wxPoint& pos = wxDefaultPosition,
                const wxSize& size = wxDefaultSize,
                long style = 0,
                const wxString& name = wxEmptyString);


    // implement base class virtuals
    virtual int GetSelection() const;
    virtual bool SetPageText(size_t n, const wxString& strText);
    virtual wxString GetPageText(size_t n) const;
    virtual int GetPageImage(size_t n) const;
    virtual bool SetPageImage(size_t n, int imageId);
    virtual wxSize CalcSizeFromPage(const wxSize& sizePage) const;
    virtual bool InsertPage(size_t n,
                            wxWindow *page,
                            const wxString& text,
                            bool bSelect = false,
                            int imageId = -1);
    virtual int SetSelection(size_t n) { return DoSetSelection(n, SetSelection_SendEvent); }
    virtual int ChangeSelection(size_t n) { return DoSetSelection(n); }
    virtual void SetImageList(wxImageList *imageList);

    virtual bool DeleteAllPages();
    virtual int HitTest(const wxPoint& pt, long *flags = NULL) const;


    // methods which are not part of base wxBookctrl API

    // get the underlying toolbar
    wxToolBarBase* GetToolBar() const { return (wxToolBarBase*)m_bookctrl; }

    // must be called in OnIdle or by application to realize the toolbar and
    // select the initial page.
    void Realize();

protected:
    virtual wxWindow *DoRemovePage(size_t page);

    // get the size which the list control should have
    virtual wxSize GetControllerSize() const;

    // event handlers
    void OnToolSelected(wxCommandEvent& event);
    void OnSize(wxSizeEvent& event);
    void OnIdle(wxIdleEvent& event);

    void UpdateSelectedPage(size_t newsel);

    wxBookCtrlBaseEvent* CreatePageChangingEvent() const;
    void MakeChangedEvent(wxBookCtrlBaseEvent &event);

    // the currently selected page or wxNOT_FOUND if none
    int m_selection;

    // whether the toolbar needs to be realized
    bool m_needsRealizing;

    // maximum bitmap size
    wxSize m_maxBitmapSize;

private:
    // common part of all constructors
    void Init();

    DECLARE_EVENT_TABLE()
    DECLARE_DYNAMIC_CLASS_NO_COPY(wxToolbook)
};

// ----------------------------------------------------------------------------
// listbook event class and related stuff
// ----------------------------------------------------------------------------

class WXDLLEXPORT wxToolbookEvent : public wxBookCtrlBaseEvent
{
public:
    wxToolbookEvent(wxEventType commandType = wxEVT_NULL, int id = 0,
                    int nSel = wxNOT_FOUND, int nOldSel = wxNOT_FOUND)
        : wxBookCtrlBaseEvent(commandType, id, nSel, nOldSel)
    {
    }

    wxToolbookEvent(const wxToolbookEvent& event)
        : wxBookCtrlBaseEvent(event)
    {
    }

    virtual wxEvent *Clone() const { return new wxToolbookEvent(*this); }

private:
    DECLARE_DYNAMIC_CLASS_NO_ASSIGN(wxToolbookEvent)
};

typedef void (wxEvtHandler::*wxToolbookEventFunction)(wxToolbookEvent&);

#define wxToolbookEventHandler(func) \
    (wxObjectEventFunction)(wxEventFunction)wxStaticCastEvent(wxToolbookEventFunction, &func)

#define EVT_TOOLBOOK_PAGE_CHANGED(winid, fn) \
    wx__DECLARE_EVT1(wxEVT_COMMAND_TOOLBOOK_PAGE_CHANGED, winid, wxToolbookEventHandler(fn))

#define EVT_TOOLBOOK_PAGE_CHANGING(winid, fn) \
    wx__DECLARE_EVT1(wxEVT_COMMAND_TOOLBOOK_PAGE_CHANGING, winid, wxToolbookEventHandler(fn))

#endif // wxUSE_TOOLBOOK

#endif // _WX_TOOLBOOK_H_
