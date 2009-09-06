/////////////////////////////////////////////////////////////////////////////
// Name:        tabctrl.h
// Purpose:     wxTabCtrl class
// Author:      Julian Smart
// Modified by:
// Created:     01/02/97
// RCS-ID:      $Id: tabctrl.h 41020 2006-09-05 20:47:48Z VZ $
// Copyright:   (c) Julian Smart
// Licence:     wxWindows licence
/////////////////////////////////////////////////////////////////////////////

#ifndef _WX_TABCTRL_H_
#define _WX_TABCTRL_H_

class WXDLLIMPEXP_CORE wxImageList;

// extern WXDLLEXPORT_DATA(const wxChar) wxToolBarNameStr[];

/*
 * Flags returned by HitTest
 */

#define wxTAB_HITTEST_NOWHERE           1
#define wxTAB_HITTEST_ONICON            2
#define wxTAB_HITTEST_ONLABEL           4
#define wxTAB_HITTEST_ONITEM            6

class WXDLLEXPORT wxTabCtrl: public wxControl
{
  DECLARE_DYNAMIC_CLASS(wxTabCtrl)
 public:
   /*
    * Public interface
    */

    wxTabCtrl();

    inline wxTabCtrl(wxWindow *parent, wxWindowID id, const wxPoint& pos = wxDefaultPosition, const wxSize& size = wxDefaultSize,
            long style = 0, const wxString& name = wxT("tabCtrl"))
    {
        Create(parent, id, pos, size, style, name);
    }
    virtual ~wxTabCtrl();

// Accessors

    // Get the selection
    int GetSelection() const;

    // Get the tab with the current keyboard focus
    int GetCurFocus() const;

    // Get the associated image list
    wxImageList* GetImageList() const;

    // Get the number of items
    int GetItemCount() const;

    // Get the rect corresponding to the tab
    bool GetItemRect(int item, wxRect& rect) const;

    // Get the number of rows
    int GetRowCount() const;

    // Get the item text
    wxString GetItemText(int item) const ;

    // Get the item image
    int GetItemImage(int item) const;

    // Get the item data
    void* GetItemData(int item) const;

    // Set the selection
    int SetSelection(int item);

    // Set the image list
    void SetImageList(wxImageList* imageList);

    // Set the text for an item
    bool SetItemText(int item, const wxString& text);

    // Set the image for an item
    bool SetItemImage(int item, int image);

    // Set the data for an item
    bool SetItemData(int item, void* data);

    // Set the size for a fixed-width tab control
    void SetItemSize(const wxSize& size);

    // Set the padding between tabs
    void SetPadding(const wxSize& padding);

// Operations

    bool Create(wxWindow *parent, wxWindowID id, const wxPoint& pos = wxDefaultPosition, const wxSize& size = wxDefaultSize,
                long style = 0, const wxString& name = wxT("tabCtrl"));

    // Delete all items
    bool DeleteAllItems();

    // Delete an item
    bool DeleteItem(int item);

    // Hit test
    int HitTest(const wxPoint& pt, long& flags);

    // Insert an item
    bool InsertItem(int item, const wxString& text, int imageId = -1, void* data = NULL);

    // Implementation

    virtual bool MSWOnNotify(int idCtrl, WXLPARAM lParam, WXLPARAM *result);

    // Responds to colour changes
    void OnSysColourChanged(wxSysColourChangedEvent& event);

protected:
    wxImageList*    m_imageList;

DECLARE_EVENT_TABLE()
    DECLARE_NO_COPY_CLASS(wxTabCtrl)
};

class WXDLLEXPORT wxTabEvent : public wxNotifyEvent
{
public:
    wxTabEvent(wxEventType commandType = wxEVT_NULL, int id = 0,
                    int nSel = -1, int nOldSel = -1)
        : wxNotifyEvent(commandType, id)
        {
            m_nSel = nSel;
            m_nOldSel = nOldSel;
        }

    // accessors
        // the currently selected page (-1 if none)
    int GetSelection() const { return m_nSel; }
    void SetSelection(int nSel) { m_nSel = nSel; }
        // the page that was selected before the change (-1 if none)
    int GetOldSelection() const { return m_nOldSel; }
    void SetOldSelection(int nOldSel) { m_nOldSel = nOldSel; }

private:
    int m_nSel,     // currently selected page
        m_nOldSel;  // previously selected page

    DECLARE_DYNAMIC_CLASS_NO_COPY(wxTabEvent)
};

typedef void (wxEvtHandler::*wxTabEventFunction)(wxTabEvent&);

#define EVT_TAB_SEL_CHANGED(id, fn) DECLARE_EVENT_TABLE_ENTRY(wxEVT_COMMAND_TAB_SEL_CHANGED, \
  id, wxID_ANY, (wxObjectEventFunction) (wxEventFunction)  wxStaticCastEvent( wxTabEventFunction, & fn ), NULL),
#define EVT_TAB_SEL_CHANGING(id, fn) DECLARE_EVENT_TABLE_ENTRY(wxEVT_COMMAND_TAB_SEL_CHANGING, \
  id, wxID_ANY, (wxObjectEventFunction) (wxEventFunction)  wxStaticCastEvent( wxTabEventFunction, & fn ), NULL),

#endif
    // _WX_TABCTRL_H_
