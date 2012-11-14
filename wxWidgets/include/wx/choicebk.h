///////////////////////////////////////////////////////////////////////////////
// Name:        wx/choicebk.h
// Purpose:     wxChoicebook: wxChoice and wxNotebook combination
// Author:      Vadim Zeitlin
// Modified by: Wlodzimierz ABX Skiba from wx/listbook.h
// Created:     15.09.04
// RCS-ID:      $Id: choicebk.h 59616 2009-03-18 21:58:15Z VZ $
// Copyright:   (c) Vadim Zeitlin, Wlodzimierz Skiba
// Licence:     wxWindows licence
///////////////////////////////////////////////////////////////////////////////

#ifndef _WX_CHOICEBOOK_H_
#define _WX_CHOICEBOOK_H_

#include "wx/defs.h"

#if wxUSE_CHOICEBOOK

#include "wx/bookctrl.h"
#include "wx/choice.h"

class WXDLLIMPEXP_FWD_CORE wxChoice;

extern WXDLLIMPEXP_CORE const wxEventType wxEVT_COMMAND_CHOICEBOOK_PAGE_CHANGED;
extern WXDLLIMPEXP_CORE const wxEventType wxEVT_COMMAND_CHOICEBOOK_PAGE_CHANGING;

// wxChoicebook flags
#define wxCHB_DEFAULT          wxBK_DEFAULT
#define wxCHB_TOP              wxBK_TOP
#define wxCHB_BOTTOM           wxBK_BOTTOM
#define wxCHB_LEFT             wxBK_LEFT
#define wxCHB_RIGHT            wxBK_RIGHT
#define wxCHB_ALIGN_MASK       wxBK_ALIGN_MASK

// ----------------------------------------------------------------------------
// wxChoicebook
// ----------------------------------------------------------------------------

class WXDLLEXPORT wxChoicebook : public wxBookCtrlBase
{
public:
    wxChoicebook()
    {
        Init();
    }

    wxChoicebook(wxWindow *parent,
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

    // returns the choice control
    wxChoice* GetChoiceCtrl() const { return (wxChoice*)m_bookctrl; }

protected:
    virtual wxWindow *DoRemovePage(size_t page);

    // get the size which the choice control should have
    virtual wxSize GetControllerSize() const;

    void UpdateSelectedPage(size_t newsel)
    {
        m_selection = wx_static_cast(int, newsel);
        GetChoiceCtrl()->Select(m_selection);
    }

    wxBookCtrlBaseEvent* CreatePageChangingEvent() const;
    void MakeChangedEvent(wxBookCtrlBaseEvent &event);

    // event handlers
    void OnChoiceSelected(wxCommandEvent& event);

    // the currently selected page or wxNOT_FOUND if none
    int m_selection;

private:
    // common part of all constructors
    void Init();

    DECLARE_EVENT_TABLE()
    DECLARE_DYNAMIC_CLASS_NO_COPY(wxChoicebook)
};

// ----------------------------------------------------------------------------
// choicebook event class and related stuff
// ----------------------------------------------------------------------------

class WXDLLEXPORT wxChoicebookEvent : public wxBookCtrlBaseEvent
{
public:
    wxChoicebookEvent(wxEventType commandType = wxEVT_NULL, int id = 0,
                      int nSel = -1, int nOldSel = -1)
        : wxBookCtrlBaseEvent(commandType, id, nSel, nOldSel)
    {
    }

    wxChoicebookEvent(const wxChoicebookEvent& event)
        : wxBookCtrlBaseEvent(event)
    {
    }

    virtual wxEvent *Clone() const { return new wxChoicebookEvent(*this); }

private:
    DECLARE_DYNAMIC_CLASS_NO_ASSIGN(wxChoicebookEvent)
};

typedef void (wxEvtHandler::*wxChoicebookEventFunction)(wxChoicebookEvent&);

#define wxChoicebookEventHandler(func) \
    (wxObjectEventFunction)(wxEventFunction)wxStaticCastEvent(wxChoicebookEventFunction, &func)

#define EVT_CHOICEBOOK_PAGE_CHANGED(winid, fn) \
    wx__DECLARE_EVT1(wxEVT_COMMAND_CHOICEBOOK_PAGE_CHANGED, winid, wxChoicebookEventHandler(fn))

#define EVT_CHOICEBOOK_PAGE_CHANGING(winid, fn) \
    wx__DECLARE_EVT1(wxEVT_COMMAND_CHOICEBOOK_PAGE_CHANGING, winid, wxChoicebookEventHandler(fn))

#endif // wxUSE_CHOICEBOOK

#endif // _WX_CHOICEBOOK_H_
