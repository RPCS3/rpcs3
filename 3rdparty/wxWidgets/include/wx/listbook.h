///////////////////////////////////////////////////////////////////////////////
// Name:        wx/listbook.h
// Purpose:     wxListbook: wxListCtrl and wxNotebook combination
// Author:      Vadim Zeitlin
// Modified by:
// Created:     19.08.03
// RCS-ID:      $Id: listbook.h 49563 2007-10-31 20:46:21Z VZ $
// Copyright:   (c) 2003 Vadim Zeitlin <vadim@wxwidgets.org>
// Licence:     wxWindows licence
///////////////////////////////////////////////////////////////////////////////

#ifndef _WX_LISTBOOK_H_
#define _WX_LISTBOOK_H_

#include "wx/defs.h"

#if wxUSE_LISTBOOK

#include "wx/bookctrl.h"

class WXDLLIMPEXP_FWD_CORE wxListView;
class WXDLLIMPEXP_FWD_CORE wxListEvent;

extern WXDLLIMPEXP_CORE const wxEventType wxEVT_COMMAND_LISTBOOK_PAGE_CHANGED;
extern WXDLLIMPEXP_CORE const wxEventType wxEVT_COMMAND_LISTBOOK_PAGE_CHANGING;

// wxListbook flags
#define wxLB_DEFAULT          wxBK_DEFAULT
#define wxLB_TOP              wxBK_TOP
#define wxLB_BOTTOM           wxBK_BOTTOM
#define wxLB_LEFT             wxBK_LEFT
#define wxLB_RIGHT            wxBK_RIGHT
#define wxLB_ALIGN_MASK       wxBK_ALIGN_MASK

// ----------------------------------------------------------------------------
// wxListbook
// ----------------------------------------------------------------------------

class WXDLLEXPORT wxListbook : public wxBookCtrlBase
{
public:
    wxListbook()
    {
        Init();
    }

    wxListbook(wxWindow *parent,
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


    // overridden base class methods
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
    virtual int HitTest(const wxPoint& pt, long *flags = NULL) const;
    virtual void SetImageList(wxImageList *imageList);

    virtual bool DeleteAllPages();

    wxListView* GetListView() const { return (wxListView*)m_bookctrl; }

protected:
    virtual wxWindow *DoRemovePage(size_t page);

    // get the size which the list control should have
    virtual wxSize GetControllerSize() const;

    void UpdateSelectedPage(size_t newsel);

    wxBookCtrlBaseEvent* CreatePageChangingEvent() const;
    void MakeChangedEvent(wxBookCtrlBaseEvent &event);

    // event handlers
    void OnListSelected(wxListEvent& event);
    void OnSize(wxSizeEvent& event);

    // the currently selected page or wxNOT_FOUND if none
    int m_selection;

private:
    // common part of all constructors
    void Init();

    DECLARE_EVENT_TABLE()
    DECLARE_DYNAMIC_CLASS_NO_COPY(wxListbook)
};

// ----------------------------------------------------------------------------
// listbook event class and related stuff
// ----------------------------------------------------------------------------

class WXDLLEXPORT wxListbookEvent : public wxBookCtrlBaseEvent
{
public:
    wxListbookEvent(wxEventType commandType = wxEVT_NULL, int id = 0,
                    int nSel = wxNOT_FOUND, int nOldSel = wxNOT_FOUND)
        : wxBookCtrlBaseEvent(commandType, id, nSel, nOldSel)
    {
    }

    wxListbookEvent(const wxListbookEvent& event)
        : wxBookCtrlBaseEvent(event)
    {
    }

    virtual wxEvent *Clone() const { return new wxListbookEvent(*this); }

private:
    DECLARE_DYNAMIC_CLASS_NO_ASSIGN(wxListbookEvent)
};

typedef void (wxEvtHandler::*wxListbookEventFunction)(wxListbookEvent&);

#define wxListbookEventHandler(func) \
    (wxObjectEventFunction)(wxEventFunction)wxStaticCastEvent(wxListbookEventFunction, &func)

#define EVT_LISTBOOK_PAGE_CHANGED(winid, fn) \
    wx__DECLARE_EVT1(wxEVT_COMMAND_LISTBOOK_PAGE_CHANGED, winid, wxListbookEventHandler(fn))

#define EVT_LISTBOOK_PAGE_CHANGING(winid, fn) \
    wx__DECLARE_EVT1(wxEVT_COMMAND_LISTBOOK_PAGE_CHANGING, winid, wxListbookEventHandler(fn))

#endif // wxUSE_LISTBOOK

#endif // _WX_LISTBOOK_H_
