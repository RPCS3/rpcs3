///////////////////////////////////////////////////////////////////////////////
// Name:        src/msw/checklst.cpp
// Purpose:     implementation of wxCheckListBox class
// Author:      Vadim Zeitlin
// Modified by:
// Created:     16.11.97
// RCS-ID:      $Id: checklst.cpp 62511 2009-10-30 14:11:03Z JMS $
// Copyright:   (c) 1998 Vadim Zeitlin <zeitlin@dptmaths.ens-cachan.fr>
// Licence:     wxWindows licence
///////////////////////////////////////////////////////////////////////////////

// ============================================================================
// declarations
// ============================================================================

// ----------------------------------------------------------------------------
// headers
// ----------------------------------------------------------------------------

// For compilers that support precompilation, includes "wx.h".
#include "wx/wxprec.h"

#ifdef __BORLANDC__
    #pragma hdrstop
#endif

#if wxUSE_CHECKLISTBOX && wxUSE_OWNER_DRAWN

#include "wx/checklst.h"

#ifndef WX_PRECOMP
    #include "wx/msw/wrapwin.h"
    #include "wx/object.h"
    #include "wx/colour.h"
    #include "wx/font.h"
    #include "wx/bitmap.h"
    #include "wx/window.h"
    #include "wx/listbox.h"
    #include "wx/dcmemory.h"
    #include "wx/settings.h"
    #include "wx/log.h"
#endif

#include "wx/ownerdrw.h"

#include <windowsx.h>

#include "wx/msw/private.h"

// ----------------------------------------------------------------------------
// private functions
// ----------------------------------------------------------------------------

// get item (converted to right type)
#define GetItem(n)    ((wxCheckListBoxItem *)(GetItem(n)))

// ============================================================================
// implementation
// ============================================================================


#if wxUSE_EXTENDED_RTTI
WX_DEFINE_FLAGS( wxCheckListBoxStyle )

wxBEGIN_FLAGS( wxCheckListBoxStyle )
    // new style border flags, we put them first to
    // use them for streaming out
    wxFLAGS_MEMBER(wxBORDER_SIMPLE)
    wxFLAGS_MEMBER(wxBORDER_SUNKEN)
    wxFLAGS_MEMBER(wxBORDER_DOUBLE)
    wxFLAGS_MEMBER(wxBORDER_RAISED)
    wxFLAGS_MEMBER(wxBORDER_STATIC)
    wxFLAGS_MEMBER(wxBORDER_NONE)

    // old style border flags
    wxFLAGS_MEMBER(wxSIMPLE_BORDER)
    wxFLAGS_MEMBER(wxSUNKEN_BORDER)
    wxFLAGS_MEMBER(wxDOUBLE_BORDER)
    wxFLAGS_MEMBER(wxRAISED_BORDER)
    wxFLAGS_MEMBER(wxSTATIC_BORDER)
    wxFLAGS_MEMBER(wxBORDER)

    // standard window styles
    wxFLAGS_MEMBER(wxTAB_TRAVERSAL)
    wxFLAGS_MEMBER(wxCLIP_CHILDREN)
    wxFLAGS_MEMBER(wxTRANSPARENT_WINDOW)
    wxFLAGS_MEMBER(wxWANTS_CHARS)
    wxFLAGS_MEMBER(wxFULL_REPAINT_ON_RESIZE)
    wxFLAGS_MEMBER(wxALWAYS_SHOW_SB )
    wxFLAGS_MEMBER(wxVSCROLL)
    wxFLAGS_MEMBER(wxHSCROLL)

    wxFLAGS_MEMBER(wxLB_SINGLE)
    wxFLAGS_MEMBER(wxLB_MULTIPLE)
    wxFLAGS_MEMBER(wxLB_EXTENDED)
    wxFLAGS_MEMBER(wxLB_HSCROLL)
    wxFLAGS_MEMBER(wxLB_ALWAYS_SB)
    wxFLAGS_MEMBER(wxLB_NEEDED_SB)
    wxFLAGS_MEMBER(wxLB_SORT)
    wxFLAGS_MEMBER(wxLB_OWNERDRAW)

wxEND_FLAGS( wxCheckListBoxStyle )

IMPLEMENT_DYNAMIC_CLASS_XTI(wxCheckListBox, wxListBox,"wx/checklst.h")

wxBEGIN_PROPERTIES_TABLE(wxCheckListBox)
    wxEVENT_PROPERTY( Toggle , wxEVT_COMMAND_CHECKLISTBOX_TOGGLED , wxCommandEvent )
    wxPROPERTY_FLAGS( WindowStyle , wxCheckListBoxStyle , long , SetWindowStyleFlag , GetWindowStyleFlag , EMPTY_MACROVALUE , wxLB_OWNERDRAW /*flags*/ , wxT("Helpstring") , wxT("group")) // style
wxEND_PROPERTIES_TABLE()

wxBEGIN_HANDLERS_TABLE(wxCheckListBox)
wxEND_HANDLERS_TABLE()

wxCONSTRUCTOR_4( wxCheckListBox , wxWindow* , Parent , wxWindowID , Id , wxPoint , Position , wxSize , Size )

#else
IMPLEMENT_DYNAMIC_CLASS(wxCheckListBox, wxListBox)
#endif

// ----------------------------------------------------------------------------
// declaration and implementation of wxCheckListBoxItem class
// ----------------------------------------------------------------------------

class wxCheckListBoxItem : public wxOwnerDrawn
{
friend class WXDLLIMPEXP_FWD_CORE wxCheckListBox;
public:
    // ctor
    wxCheckListBoxItem(wxCheckListBox *pParent, size_t nIndex);

    // drawing functions
    virtual bool OnDrawItem(wxDC& dc, const wxRect& rc, wxODAction act, wxODStatus stat);

    // simple accessors and operations
    bool IsChecked() const { return m_bChecked; }

    void Check(bool bCheck);
    void Toggle() { Check(!IsChecked()); }

    void SendEvent();

private:
    bool            m_bChecked;
    wxCheckListBox *m_pParent;
    size_t    m_nIndex;

    DECLARE_NO_COPY_CLASS(wxCheckListBoxItem)
};

wxCheckListBoxItem::wxCheckListBoxItem(wxCheckListBox *pParent, size_t nIndex)
                  : wxOwnerDrawn(wxEmptyString, true)   // checkable
{
    m_bChecked = false;
    m_pParent  = pParent;
    m_nIndex   = nIndex;

    // we don't initialize m_nCheckHeight/Width vars because it's
    // done in OnMeasure while they are used only in OnDraw and we
    // know that there will always be OnMeasure before OnDraw

    // fix appearance for check list boxes: they don't look quite the same as
    // menu icons
    SetOwnMarginWidth(::GetSystemMetrics(SM_CXMENUCHECK) -
                      2*wxSystemSettings::GetMetric(wxSYS_EDGE_X) + 1);

    SetBackgroundColour(pParent->GetBackgroundColour());
}

bool wxCheckListBoxItem::OnDrawItem(wxDC& dc, const wxRect& rc,
                                    wxODAction act, wxODStatus stat)
{
    // first draw the label
    if ( IsChecked() )
        stat = (wxOwnerDrawn::wxODStatus)(stat | wxOwnerDrawn::wxODChecked);

    if ( !wxOwnerDrawn::OnDrawItem(dc, rc, act, stat) )
        return false;


    // now draw the check mark part
    size_t nCheckWidth  = GetDefaultMarginWidth(),
           nCheckHeight = m_pParent->GetItemHeight();

    int x = rc.GetX(),
        y = rc.GetY();

    HDC hdc = (HDC)dc.GetHDC();

    // create pens, brushes &c
    COLORREF colBg = wxColourToRGB(GetBackgroundColour());
    AutoHPEN hpenBack(colBg),
             hpenGray(RGB(0xc0, 0xc0, 0xc0));

    SelectInHDC selPen(hdc, (HGDIOBJ)hpenBack);
    AutoHBRUSH hbrBack(colBg);
    SelectInHDC selBrush(hdc, hbrBack);

    // erase the background: it could have been filled with the selected colour
    Rectangle(hdc, x, y, x + nCheckWidth + 1, rc.GetBottom() + 1);

    // shift check mark 1 pixel to the right, looks better like this
    x++;

    if ( IsChecked() )
    {
        // first create a monochrome bitmap in a memory DC
        MemoryHDC hdcMem(hdc);
        MonoBitmap hbmpCheck(nCheckWidth, nCheckHeight);
        SelectInHDC selBmp(hdcMem, hbmpCheck);

        // then draw a check mark into it
        RECT rect = { 0, 0, nCheckWidth, nCheckHeight };
        ::DrawFrameControl(hdcMem, &rect,
#ifdef __WXWINCE__
                           DFC_BUTTON, DFCS_BUTTONCHECK
#else
                           DFC_MENU, DFCS_MENUCHECK
#endif
                           );

        // finally copy it to screen DC
        ::BitBlt(hdc, x, y, nCheckWidth, nCheckHeight, hdcMem, 0, 0, SRCCOPY);
    }

    // now we draw the smaller rectangle
    y++;
    nCheckWidth  -= 2;
    nCheckHeight -= 2;

    // draw hollow gray rectangle
    (void)::SelectObject(hdc, (HGDIOBJ)hpenGray);

    SelectInHDC selBrush2(hdc, ::GetStockObject(NULL_BRUSH));
    Rectangle(hdc, x, y, x + nCheckWidth, y + nCheckHeight);

    if (stat & wxODHasFocus)
    {
        RECT rect;
        wxCopyRectToRECT(rc, rect);
        DrawFocusRect(hdc, &rect);
    }

    return true;
}

// change the state of the item and redraw it
void wxCheckListBoxItem::Check(bool check)
{
    m_bChecked = check;

    // index may be changed because new items were added/deleted
    if ( m_pParent->GetItemIndex(this) != (int)m_nIndex )
    {
        // update it
        int index = m_pParent->GetItemIndex(this);

        wxASSERT_MSG( index != wxNOT_FOUND, wxT("what does this item do here?") );

        m_nIndex = (size_t)index;
    }

    HWND hwndListbox = (HWND)m_pParent->GetHWND();

    RECT rcUpdate;

    if ( ::SendMessage(hwndListbox, LB_GETITEMRECT,
                       m_nIndex, (LPARAM)&rcUpdate) == LB_ERR )
    {
        wxLogDebug(wxT("LB_GETITEMRECT failed"));
    }

    ::InvalidateRect(hwndListbox, &rcUpdate, FALSE);
}

// send an "item checked" event
void wxCheckListBoxItem::SendEvent()
{
    wxCommandEvent event(wxEVT_COMMAND_CHECKLISTBOX_TOGGLED, m_pParent->GetId());
    event.SetInt(m_nIndex);
    event.SetEventObject(m_pParent);
    m_pParent->ProcessCommand(event);
}

// ----------------------------------------------------------------------------
// implementation of wxCheckListBox class
// ----------------------------------------------------------------------------

// define event table
// ------------------
BEGIN_EVENT_TABLE(wxCheckListBox, wxListBox)
  EVT_KEY_DOWN(wxCheckListBox::OnKeyDown)
  EVT_LEFT_DOWN(wxCheckListBox::OnLeftClick)
END_EVENT_TABLE()

// control creation
// ----------------

// def ctor: use Create() to really create the control
wxCheckListBox::wxCheckListBox()
{
}

// ctor which creates the associated control
wxCheckListBox::wxCheckListBox(wxWindow *parent, wxWindowID id,
                               const wxPoint& pos, const wxSize& size,
                               int nStrings, const wxString choices[],
                               long style, const wxValidator& val,
                               const wxString& name)
{
    Create(parent, id, pos, size, nStrings, choices, style, val, name);
}

wxCheckListBox::wxCheckListBox(wxWindow *parent, wxWindowID id,
                               const wxPoint& pos, const wxSize& size,
                               const wxArrayString& choices,
                               long style, const wxValidator& val,
                               const wxString& name)
{
    Create(parent, id, pos, size, choices, style, val, name);
}

bool wxCheckListBox::Create(wxWindow *parent, wxWindowID id,
                            const wxPoint& pos, const wxSize& size,
                            int n, const wxString choices[],
                            long style,
                            const wxValidator& validator, const wxString& name)
{
    return wxListBox::Create(parent, id, pos, size, n, choices,
                             style | wxLB_OWNERDRAW, validator, name);
}

bool wxCheckListBox::Create(wxWindow *parent, wxWindowID id,
                            const wxPoint& pos, const wxSize& size,
                            const wxArrayString& choices,
                            long style,
                            const wxValidator& validator, const wxString& name)
{
    return wxListBox::Create(parent, id, pos, size, choices,
                             style | wxLB_OWNERDRAW, validator, name);
}

// misc overloaded methods
// -----------------------

void wxCheckListBox::Delete(unsigned int n)
{
    wxCHECK_RET( IsValid(n),
                 wxT("invalid index in wxListBox::Delete") );

    wxListBox::Delete(n);

    // free memory
    delete m_aItems[n];

    m_aItems.RemoveAt(n);
}

bool wxCheckListBox::SetFont( const wxFont &font )
{
    unsigned int i;
    for ( i = 0; i < m_aItems.GetCount(); i++ )
        m_aItems[i]->SetFont(font);

    wxListBox::SetFont(font);

    return true;
}

// create/retrieve item
// --------------------

// create a check list box item
wxOwnerDrawn *wxCheckListBox::CreateLboxItem(size_t nIndex)
{
  wxCheckListBoxItem *pItem = new wxCheckListBoxItem(this, nIndex);
  return pItem;
}

// return item size
// ----------------
bool wxCheckListBox::MSWOnMeasure(WXMEASUREITEMSTRUCT *item)
{
  if ( wxListBox::MSWOnMeasure(item) ) {
    MEASUREITEMSTRUCT *pStruct = (MEASUREITEMSTRUCT *)item;

    // save item height
    m_nItemHeight = pStruct->itemHeight;

    // add place for the check mark
    pStruct->itemWidth += wxOwnerDrawn::GetDefaultMarginWidth();

    return true;
  }

  return false;
}

// check items
// -----------

bool wxCheckListBox::IsChecked(unsigned int uiIndex) const
{
    wxCHECK_MSG( IsValid(uiIndex), false, _T("bad wxCheckListBox index") );

    return GetItem(uiIndex)->IsChecked();
}

void wxCheckListBox::Check(unsigned int uiIndex, bool bCheck)
{
    wxCHECK_RET( IsValid(uiIndex), _T("bad wxCheckListBox index") );

    GetItem(uiIndex)->Check(bCheck);
}

// process events
// --------------

void wxCheckListBox::OnKeyDown(wxKeyEvent& event)
{
    // what do we do?
    enum
    {
        None,
        Toggle,
        Set,
        Clear
    } oper;

    switch ( event.GetKeyCode() )
    {
        case WXK_SPACE:
            oper = Toggle;
            break;

        case WXK_NUMPAD_ADD:
        case '+':
            oper = Set;
            break;

        case WXK_NUMPAD_SUBTRACT:
        case '-':
            oper = Clear;
            break;

        default:
            oper = None;
    }

    if ( oper != None )
    {
        wxArrayInt selections;
        int count = 0;
        if ( HasMultipleSelection() )
        {
            count = GetSelections(selections);
        }
        else
        {
            int sel = GetSelection();
            if (sel != -1)
            {
                count = 1;
                selections.Add(sel);
            }
        }

        for ( int i = 0; i < count; i++ )
        {
            wxCheckListBoxItem *item = GetItem(selections[i]);
            if ( !item )
            {
                wxFAIL_MSG( _T("no wxCheckListBoxItem?") );
                continue;
            }

            switch ( oper )
            {
                case Toggle:
                    item->Toggle();
                    break;

                case Set:
                case Clear:
                    item->Check( oper == Set );
                    break;

                default:
                    wxFAIL_MSG( _T("what should this key do?") );
            }

            // we should send an event as this has been done by the user and
            // not by the program
            item->SendEvent();
        }
    }
    else // nothing to do
    {
        event.Skip();
    }
}

void wxCheckListBox::OnLeftClick(wxMouseEvent& event)
{
    // clicking on the item selects it, clicking on the checkmark toggles
    if ( event.GetX() <= wxOwnerDrawn::GetDefaultMarginWidth() )
    {
        int nItem = HitTest(event.GetX(), event.GetY());

        if ( nItem != wxNOT_FOUND )
        {
            wxCheckListBoxItem *item = GetItem(nItem);
            item->Toggle();
            item->SendEvent();
        }
        //else: it's not an error, just click outside of client zone
    }
    else
    {
        // implement default behaviour: clicking on the item selects it
        event.Skip();
    }
}

int wxCheckListBox::DoHitTestItem(wxCoord x, wxCoord y) const
{
    int nItem = (int)::SendMessage
                             (
                              (HWND)GetHWND(),
                              LB_ITEMFROMPOINT,
                              0,
                              MAKELPARAM(x, y)
                             );

    return nItem >= (int)m_noItems ? wxNOT_FOUND : nItem;
}


wxSize wxCheckListBox::DoGetBestSize() const
{
    wxSize best = wxListBox::DoGetBestSize();
    best.x += wxOwnerDrawn::GetDefaultMarginWidth();  // add room for the checkbox
    CacheBestSize(best);
    return best;
}

#endif
