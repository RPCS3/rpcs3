/////////////////////////////////////////////////////////////////////////////
// Name:        wx/odcombo.h
// Purpose:     wxOwnerDrawnComboBox and wxVListBoxPopup
// Author:      Jaakko Salli
// Modified by:
// Created:     Apr-30-2006
// RCS-ID:      $Id: odcombo.h 64259 2010-05-09 10:48:37Z JMS $
// Copyright:   (c) Jaakko Salli
// Licence:     wxWindows licence
/////////////////////////////////////////////////////////////////////////////

#ifndef _WX_ODCOMBO_H_
#define _WX_ODCOMBO_H_

#include "wx/defs.h"

#if wxUSE_ODCOMBOBOX

#include "wx/combo.h"
#include "wx/ctrlsub.h"
#include "wx/vlbox.h"
#include "wx/timer.h"


//
// New window styles for wxOwnerDrawnComboBox
//
enum
{
    // Double-clicking cycles item if wxCB_READONLY is also used.
    wxODCB_DCLICK_CYCLES            = wxCC_SPECIAL_DCLICK,

    // If used, control itself is not custom paint using callback.
    // Even if this is not used, writable combo is never custom paint
    // until SetCustomPaintWidth is called
    wxODCB_STD_CONTROL_PAINT        = 0x1000
};


//
// Callback flags (see wxOwnerDrawnComboBox::OnDrawItem)
//
enum
{
    // when set, we are painting the selected item in control,
    // not in the popup
    wxODCB_PAINTING_CONTROL         = 0x0001,


    // when set, we are painting an item which should have
    // focus rectangle painted in the background. Text colour
    // and clipping region are then appropriately set in
    // the default OnDrawBackground implementation.
    wxODCB_PAINTING_SELECTED        = 0x0002
};


// ----------------------------------------------------------------------------
// wxVListBoxComboPopup is a wxVListBox customized to act as a popup control.
//
// Notes:
//   wxOwnerDrawnComboBox uses this as its popup. However, it always derives
//   from native wxComboCtrl. If you need to use this popup with
//   wxGenericComboControl, then remember that vast majority of item manipulation
//   functionality is implemented in the wxVListBoxComboPopup class itself.
//
// ----------------------------------------------------------------------------


class WXDLLIMPEXP_ADV wxVListBoxComboPopup : public wxVListBox,
                                             public wxComboPopup
{
    friend class wxOwnerDrawnComboBox;
public:

    // init and dtor
    wxVListBoxComboPopup() : wxVListBox(), wxComboPopup() { }
    virtual ~wxVListBoxComboPopup();

    // required virtuals
    virtual void Init();
    virtual bool Create(wxWindow* parent);
    virtual wxWindow *GetControl() { return this; }
    virtual void SetStringValue( const wxString& value );
    virtual wxString GetStringValue() const;

    // more customization
    virtual void OnPopup();
    virtual wxSize GetAdjustedSize( int minWidth, int prefHeight, int maxHeight );
    virtual void PaintComboControl( wxDC& dc, const wxRect& rect );
    virtual void OnComboKeyEvent( wxKeyEvent& event );
    virtual void OnComboDoubleClick();
    virtual bool LazyCreate();

    // Item management
    void SetSelection( int item );
    void Insert( const wxString& item, int pos );
    int Append(const wxString& item);
    void Clear();
    void Delete( unsigned int item );
    void SetItemClientData(unsigned int n, void* clientData, wxClientDataType clientDataItemsType);
    void *GetItemClientData(unsigned int n) const;
    void SetString( int item, const wxString& str );
    wxString GetString( int item ) const;
    unsigned int GetCount() const;
    int FindString(const wxString& s, bool bCase = false) const;
    int GetSelection() const;

    //void Populate( int n, const wxString choices[] );
    void Populate( const wxArrayString& choices );
    void ClearClientDatas();

    // helpers
    int GetItemAtPosition( const wxPoint& pos ) { return HitTest(pos); }
    wxCoord GetTotalHeight() const { return EstimateTotalHeight(); }
    wxCoord GetLineHeight(int line) const { return OnGetLineHeight(line); }

protected:

    // Called by OnComboDoubleClick and OnComboKeyEvent
    bool HandleKey( int keycode, bool saturate, wxChar unicode = 0 );

    // sends combobox select event from the parent combo control
    void SendComboBoxEvent( int selection );

    // gets value, sends event and dismisses
    void DismissWithEvent();

    // OnMeasureItemWidth will be called on next GetAdjustedSize.
    void ItemWidthChanged(unsigned int item)
    {
        m_widths[item] = -1;
        m_widthsDirty = true;
    }

    // Callbacks for drawing and measuring items. Override in a derived class for
    // owner-drawnness. Font, background and text colour have been prepared according
    // to selection, focus and such.
    //
    // item: item index to be drawn, may be wxNOT_FOUND when painting combo control itself
    //       and there is no valid selection
    // flags: wxODCB_PAINTING_CONTROL is set if painting to combo control instead of list
    // NOTE: If wxVListBoxComboPopup is used with wxComboCtrl class not derived from
    //       wxOwnerDrawnComboBox, this method must be overridden.
    virtual void OnDrawItem( wxDC& dc, const wxRect& rect, int item, int flags ) const;

    // This is same as in wxVListBox
    virtual wxCoord OnMeasureItem( size_t item ) const;

    // Return item width, or -1 for calculating from text extent (default)
    virtual wxCoord OnMeasureItemWidth( size_t item ) const;

    // Draw item and combo control background. Flags are same as with OnDrawItem.
    // NB: Can't use name OnDrawBackground because of virtual function hiding warnings.
    virtual void OnDrawBg(wxDC& dc, const wxRect& rect, int item, int flags) const;

    // Additional wxVListBox implementation (no need to override in derived classes)
    virtual void OnDrawItem(wxDC& dc, const wxRect& rect, size_t n) const;
    void OnDrawBackground(wxDC& dc, const wxRect& rect, size_t n) const;

    // filter mouse move events happening outside the list box
    // move selection with cursor
    void OnMouseMove(wxMouseEvent& event);
    void OnMouseWheel(wxMouseEvent& event);
    void OnKey(wxKeyEvent& event);
    void OnLeftClick(wxMouseEvent& event);

    // Return the widest item width (recalculating it if necessary)
    int GetWidestItemWidth() { CalcWidths(); return m_widestWidth; }

    // Return the index of the widest item (recalculating it if necessary)
    int GetWidestItem() { CalcWidths(); return m_widestItem; }

    // Stop partial completion (when some other event occurs)
    void StopPartialCompletion();

#ifdef __WXMSW__
    // Added to work around a SetFocus() log error. Overriding virtual member
    // function from the primary base class (in this case, wxVListBox) should
    // be ABI compatible.
    virtual void SetFocus();
#endif

    wxArrayString           m_strings;
    wxArrayPtrVoid          m_clientDatas;

    wxFont                  m_useFont;

    //wxString                m_stringValue; // displayed text (may be different than m_strings[m_value])
    int                     m_value; // selection
    int                     m_itemHover; // on which item the cursor is
    int                     m_itemHeight; // default item height (calculate from font size
                                          // and used in the absence of callback)
    wxClientDataType        m_clientDataItemsType;

private:

    // Cached item widths (in pixels).
    wxArrayInt              m_widths;

    // Width of currently widest item.
    int                     m_widestWidth;

    // Index of currently widest item.
    int                     m_widestItem;

    // Measure some items in next GetAdjustedSize?
    bool                    m_widthsDirty;

    // Find widest item in next GetAdjustedSize?
    bool                    m_findWidest;

    // has the mouse been released on this control?
    bool                    m_clicked;

    // Recalculate widths if they are dirty
    void CalcWidths();

    // Partial completion string
    wxString                m_partialCompletionString;

#if wxUSE_TIMER
    // Partial completion timer
    wxTimer                 m_partialCompletionTimer;
#endif // wxUSE_TIMER

    DECLARE_EVENT_TABLE()
};


// ----------------------------------------------------------------------------
// wxOwnerDrawnComboBox: a generic wxComboBox that allows custom paint items
// in addition to many other types of customization already allowed by
// the wxComboCtrl.
// ----------------------------------------------------------------------------

class WXDLLIMPEXP_ADV wxOwnerDrawnComboBox : public wxComboCtrl,
                                             public wxItemContainer
{
    //friend class wxComboPopupWindow;
    friend class wxVListBoxComboPopup;
public:

    // ctors and such
    wxOwnerDrawnComboBox() : wxComboCtrl() { Init(); }

    wxOwnerDrawnComboBox(wxWindow *parent,
                         wxWindowID id,
                         const wxString& value,
                         const wxPoint& pos,
                         const wxSize& size,
                         int n,
                         const wxString choices[],
                         long style = 0,
                         const wxValidator& validator = wxDefaultValidator,
                         const wxString& name = wxComboBoxNameStr)
        : wxComboCtrl()
    {
        Init();

        (void)Create(parent, id, value, pos, size, n,
                     choices, style, validator, name);
    }

    bool Create(wxWindow *parent,
                wxWindowID id,
                const wxString& value = wxEmptyString,
                const wxPoint& pos = wxDefaultPosition,
                const wxSize& size = wxDefaultSize,
                long style = 0,
                const wxValidator& validator = wxDefaultValidator,
                const wxString& name = wxComboBoxNameStr);

    wxOwnerDrawnComboBox(wxWindow *parent,
                         wxWindowID id,
                         const wxString& value,
                         const wxPoint& pos,
                         const wxSize& size,
                         const wxArrayString& choices,
                         long style,
                         const wxValidator& validator = wxDefaultValidator,
                         const wxString& name = wxComboBoxNameStr);

    bool Create(wxWindow *parent,
                wxWindowID id,
                const wxString& value,
                const wxPoint& pos,
                const wxSize& size,
                int n,
                const wxString choices[],
                long style = 0,
                const wxValidator& validator = wxDefaultValidator,
                const wxString& name = wxComboBoxNameStr);

    bool Create(wxWindow *parent,
                wxWindowID id,
                const wxString& value,
                const wxPoint& pos,
                const wxSize& size,
                const wxArrayString& choices,
                long style = 0,
                const wxValidator& validator = wxDefaultValidator,
                const wxString& name = wxComboBoxNameStr);

    virtual ~wxOwnerDrawnComboBox();

    // Prevent app from using wxComboPopup
    void SetPopupControl(wxVListBoxComboPopup* popup)
    {
        DoSetPopupControl(popup);
    }

    // wxControlWithItems methods
    virtual void Clear();
    virtual void Delete(unsigned int n);
    virtual unsigned int GetCount() const;
    virtual wxString GetString(unsigned int n) const;
    virtual void SetString(unsigned int n, const wxString& s);
    virtual int FindString(const wxString& s, bool bCase = false) const;
    virtual void Select(int n);
    virtual int GetSelection() const;
    virtual void SetSelection(int n) { Select(n); }


    // Prevent a method from being hidden
    virtual void SetSelection(long from, long to)
    {
        wxComboCtrl::SetSelection(from,to);
    }

    // Return the widest item width (recalculating it if necessary)
    virtual int GetWidestItemWidth() { EnsurePopupControl(); return GetVListBoxComboPopup()->GetWidestItemWidth(); }

    // Return the index of the widest item (recalculating it if necessary)
    virtual int GetWidestItem() { EnsurePopupControl(); return GetVListBoxComboPopup()->GetWidestItem(); }

    wxCONTROL_ITEMCONTAINER_CLIENTDATAOBJECT_RECAST

protected:

    // Callback for drawing. Font, background and text colour have been
    // prepared according to selection, focus and such.
    // item: item index to be drawn, may be wxNOT_FOUND when painting combo control itself
    //       and there is no valid selection
    // flags: wxODCB_PAINTING_CONTROL is set if painting to combo control instead of list
    virtual void OnDrawItem( wxDC& dc, const wxRect& rect, int item, int flags ) const;

    // Callback for item height, or -1 for default
    virtual wxCoord OnMeasureItem( size_t item ) const;

    // Callback for item width, or -1 for default/undetermined
    virtual wxCoord OnMeasureItemWidth( size_t item ) const;

    // Callback for background drawing. Flags are same as with
    // OnDrawItem.
    virtual void OnDrawBackground( wxDC& dc, const wxRect& rect, int item, int flags ) const;

    // NULL popup can be used to indicate default interface
    virtual void DoSetPopupControl(wxComboPopup* popup);

    // clears all allocated client datas
    void ClearClientDatas();

    wxVListBoxComboPopup* GetVListBoxComboPopup() const
    {
        return (wxVListBoxComboPopup*) m_popupInterface;
    }

    virtual int DoAppend(const wxString& item);
    virtual int DoInsert(const wxString& item, unsigned int pos);
    virtual void DoSetItemClientData(unsigned int n, void* clientData);
    virtual void* DoGetItemClientData(unsigned int n) const;
    virtual void DoSetItemClientObject(unsigned int n, wxClientData* clientData);
    virtual wxClientData* DoGetItemClientObject(unsigned int n) const;

    // temporary storage for the initial choices
    //const wxString*         m_baseChoices;
    //int                     m_baseChoicesCount;
    wxArrayString           m_initChs;

private:
    void Init();

    DECLARE_EVENT_TABLE()

    DECLARE_DYNAMIC_CLASS(wxOwnerDrawnComboBox)
};


#endif // wxUSE_ODCOMBOBOX

#endif
    // _WX_ODCOMBO_H_
