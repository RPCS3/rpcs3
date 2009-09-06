/////////////////////////////////////////////////////////////////////////////
// Name:        src/msw/radiobox.cpp
// Purpose:     wxRadioBox implementation
// Author:      Julian Smart
// Modified by:
// Created:     04/01/98
// RCS-ID:      $Id: radiobox.cpp 54927 2008-08-02 15:59:13Z VZ $
// Copyright:   (c) Julian Smart
// Licence:     wxWindows licence
/////////////////////////////////////////////////////////////////////////////

// ===========================================================================
// declarations
// ===========================================================================

// ---------------------------------------------------------------------------
// headers
// ---------------------------------------------------------------------------

// For compilers that support precompilation, includes "wx.h".
#include "wx/wxprec.h"

#ifdef __BORLANDC__
    #pragma hdrstop
#endif

#if wxUSE_RADIOBOX

#include "wx/radiobox.h"

#ifndef WX_PRECOMP
    #include "wx/bitmap.h"
    #include "wx/brush.h"
    #include "wx/settings.h"
    #include "wx/log.h"
#endif

#include "wx/msw/subwin.h"

#if wxUSE_TOOLTIPS
    #include "wx/tooltip.h"
#endif // wxUSE_TOOLTIPS

// TODO: wxCONSTRUCTOR
#if 0 // wxUSE_EXTENDED_RTTI
WX_DEFINE_FLAGS( wxRadioBoxStyle )

wxBEGIN_FLAGS( wxRadioBoxStyle )
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

    wxFLAGS_MEMBER(wxRA_SPECIFY_COLS)
    wxFLAGS_MEMBER(wxRA_HORIZONTAL)
    wxFLAGS_MEMBER(wxRA_SPECIFY_ROWS)
    wxFLAGS_MEMBER(wxRA_VERTICAL)

wxEND_FLAGS( wxRadioBoxStyle )

IMPLEMENT_DYNAMIC_CLASS_XTI(wxRadioBox, wxControl,"wx/radiobox.h")

wxBEGIN_PROPERTIES_TABLE(wxRadioBox)
    wxEVENT_PROPERTY( Select , wxEVT_COMMAND_RADIOBOX_SELECTED , wxCommandEvent )
    wxPROPERTY_FLAGS( WindowStyle , wxRadioBoxStyle , long , SetWindowStyleFlag , GetWindowStyleFlag , , 0 /*flags*/ , wxT("Helpstring") , wxT("group")) // style
wxEND_PROPERTIES_TABLE()

#else
IMPLEMENT_DYNAMIC_CLASS(wxRadioBox, wxControl)
#endif

/*
    selection
    content
        label
        dimension
        item
*/

// ---------------------------------------------------------------------------
// private functions
// ---------------------------------------------------------------------------

// wnd proc for radio buttons
LRESULT APIENTRY _EXPORT wxRadioBtnWndProc(HWND hWnd,
                                           UINT message,
                                           WPARAM wParam,
                                           LPARAM lParam);

// ---------------------------------------------------------------------------
// global vars
// ---------------------------------------------------------------------------

// the pointer to standard radio button wnd proc
static WXFARPROC s_wndprocRadioBtn = (WXFARPROC)NULL;

// ===========================================================================
// implementation
// ===========================================================================

// ---------------------------------------------------------------------------
// wxRadioBox creation
// ---------------------------------------------------------------------------

// Radio box item
void wxRadioBox::Init()
{
    m_selectedButton = wxNOT_FOUND;
    m_radioButtons = NULL;
    m_radioWidth = NULL;
    m_radioHeight = NULL;
}

bool wxRadioBox::Create(wxWindow *parent,
                        wxWindowID id,
                        const wxString& title,
                        const wxPoint& pos,
                        const wxSize& size,
                        int n,
                        const wxString choices[],
                        int majorDim,
                        long style,
                        const wxValidator& val,
                        const wxString& name)
{
    // common initialization
    if ( !wxStaticBox::Create(parent, id, title, pos, size, style, name) )
        return false;

    // the code elsewhere in this file supposes that either wxRA_SPECIFY_COLS
    // or wxRA_SPECIFY_ROWS is set, ensure that this is indeed the case
    if ( !(style & (wxRA_SPECIFY_ROWS | wxRA_SPECIFY_COLS)) )
        style |= wxRA_SPECIFY_COLS;

#if wxUSE_VALIDATORS
    SetValidator(val);
#else
    wxUnusedVar(val);
#endif // wxUSE_VALIDATORS/!wxUSE_VALIDATORS

    m_radioButtons = new wxSubwindows(n);
    m_radioWidth = new int[n];
    m_radioHeight = new int[n];

    for ( int i = 0; i < n; i++ )
    {
        m_radioWidth[i] =
        m_radioHeight[i] = wxDefaultCoord;
        long styleBtn = BS_AUTORADIOBUTTON | WS_TABSTOP | WS_CHILD | WS_VISIBLE;
        if ( i == 0 )
            styleBtn |= WS_GROUP;

        long newId = NewControlId();

        HWND hwndBtn = ::CreateWindow(_T("BUTTON"),
                                      choices[i],
                                      styleBtn,
                                      0, 0, 0, 0,   // will be set in SetSize()
                                      GetHwndOf(parent),
                                      (HMENU)newId,
                                      wxGetInstance(),
                                      NULL);

        if ( !hwndBtn )
        {
            wxLogLastError(wxT("CreateWindow(radio btn)"));

            return false;
        }

        (*m_radioButtons)[i] = hwndBtn;

        SubclassRadioButton((WXHWND)hwndBtn);

        m_subControls.Add(newId);
    }

    // Create a dummy radio control to end the group.
    (void)::CreateWindow(_T("BUTTON"),
                         wxEmptyString,
                         WS_GROUP | BS_AUTORADIOBUTTON | WS_CHILD,
                         0, 0, 0, 0, GetHwndOf(parent),
                         (HMENU)NewControlId(), wxGetInstance(), NULL);

    m_radioButtons->SetFont(GetFont());

#ifdef __WXWINCE__
    // Set the z-order correctly
    SetWindowPos(GetHwnd(), HWND_BOTTOM, 0, 0, 0, 0, SWP_NOMOVE|SWP_NOSIZE);
#endif

    SetMajorDim(majorDim == 0 ? n : majorDim, style);
    SetSelection(0);
    SetSize(pos.x, pos.y, size.x, size.y);

    // Now that we have items determine what is the best size and set it.
    SetInitialSize(size);

    return true;
}

bool wxRadioBox::Create(wxWindow *parent,
                        wxWindowID id,
                        const wxString& title,
                        const wxPoint& pos,
                        const wxSize& size,
                        const wxArrayString& choices,
                        int majorDim,
                        long style,
                        const wxValidator& val,
                        const wxString& name)
{
    wxCArrayString chs(choices);
    return Create(parent, id, title, pos, size, chs.GetCount(),
                  chs.GetStrings(), majorDim, style, val, name);
}

wxRadioBox::~wxRadioBox()
{
    m_isBeingDeleted = true;

    delete m_radioButtons;
    delete[] m_radioWidth;
    delete[] m_radioHeight;
}

// NB: if this code is changed, wxGetWindowForHWND() which relies on having the
//     radiobox pointer in GWL_USERDATA for radio buttons must be updated too!
void wxRadioBox::SubclassRadioButton(WXHWND hWndBtn)
{
    HWND hwndBtn = (HWND)hWndBtn;

    if ( !s_wndprocRadioBtn )
        s_wndprocRadioBtn = (WXFARPROC)wxGetWindowProc(hwndBtn);

    wxSetWindowProc(hwndBtn, wxRadioBtnWndProc);
    wxSetWindowUserData(hwndBtn, this);
}

// ----------------------------------------------------------------------------
// events generation
// ----------------------------------------------------------------------------

bool wxRadioBox::MSWCommand(WXUINT cmd, WXWORD id)
{
    if ( cmd == BN_CLICKED )
    {
        if (id == GetId())
            return true;

        int selectedButton = wxNOT_FOUND;

        const unsigned int count = GetCount();
        for ( unsigned int i = 0; i < count; i++ )
        {
            const HWND hwndBtn = (*m_radioButtons)[i];
            if ( id == wxGetWindowId(hwndBtn) )
            {
                // we can get BN_CLICKED for a button which just became focused
                // but it may not be checked, in which case we shouldn't
                // generate a radiobox selection changed event for it
                if ( ::SendMessage(hwndBtn, BM_GETCHECK, 0, 0) == BST_CHECKED )
                    selectedButton = i;

                break;
            }
        }

        if ( selectedButton == wxNOT_FOUND )
        {
            // just ignore it - due to a hack with WM_NCHITTEST handling in our
            // wnd proc, we can receive dummy click messages when we click near
            // the radiobox edge (this is ugly but Julian wouldn't let me get
            // rid of this...)
            return false;
        }

        if ( selectedButton != m_selectedButton )
        {
            m_selectedButton = selectedButton;

            SendNotificationEvent();
        }
        //else: don't generate events when the selection doesn't change

        return true;
    }
    else
        return false;
}

void wxRadioBox::Command(wxCommandEvent & event)
{
    SetSelection (event.GetInt());
    SetFocus();
    ProcessCommand(event);
}

void wxRadioBox::SendNotificationEvent()
{
    wxCommandEvent event(wxEVT_COMMAND_RADIOBOX_SELECTED, m_windowId);
    event.SetInt( m_selectedButton );
    event.SetString(GetString(m_selectedButton));
    event.SetEventObject( this );
    ProcessCommand(event);
}

// ----------------------------------------------------------------------------
// simple accessors
// ----------------------------------------------------------------------------

unsigned int wxRadioBox::GetCount() const
{
    return m_radioButtons ? m_radioButtons->GetCount() : 0u;
}

void wxRadioBox::SetString(unsigned int item, const wxString& label)
{
    wxCHECK_RET( IsValid(item), wxT("invalid radiobox index") );

    m_radioWidth[item] =
    m_radioHeight[item] = wxDefaultCoord;

    ::SetWindowText((*m_radioButtons)[item], label.c_str());

    InvalidateBestSize();
}

void wxRadioBox::SetSelection(int N)
{
    wxCHECK_RET( IsValid(N), wxT("invalid radiobox index") );

    // unselect the old button
    if ( m_selectedButton != wxNOT_FOUND )
        ::SendMessage((*m_radioButtons)[m_selectedButton], BM_SETCHECK, 0, 0L);

    // and select the new one
    ::SendMessage((*m_radioButtons)[N], BM_SETCHECK, 1, 0L);

    m_selectedButton = N;
}

// Find string for position
wxString wxRadioBox::GetString(unsigned int item) const
{
    wxCHECK_MSG( IsValid(item), wxEmptyString,
                 wxT("invalid radiobox index") );

    return wxGetWindowText((*m_radioButtons)[item]);
}

void wxRadioBox::SetFocus()
{
    if ( GetCount() > 0 )
    {
        ::SetFocus((*m_radioButtons)[m_selectedButton == wxNOT_FOUND
                                        ? 0
                                        : m_selectedButton]);
    }
}

// Enable a specific button
bool wxRadioBox::Enable(unsigned int item, bool enable)
{
    wxCHECK_MSG( IsValid(item), false,
                 wxT("invalid item in wxRadioBox::Enable()") );

    BOOL ret = ::EnableWindow((*m_radioButtons)[item], enable);

    return (ret == 0) != enable;
}

bool wxRadioBox::IsItemEnabled(unsigned int item) const
{
    wxCHECK_MSG( IsValid(item), false,
                 wxT("invalid item in wxRadioBox::IsItemEnabled()") );

    return ::IsWindowEnabled((*m_radioButtons)[item]) != 0;
}

// Show a specific button
bool wxRadioBox::Show(unsigned int item, bool show)
{
    wxCHECK_MSG( IsValid(item), false,
                 wxT("invalid item in wxRadioBox::Show()") );

    BOOL ret = ::ShowWindow((*m_radioButtons)[item], show ? SW_SHOW : SW_HIDE);

    bool changed = (ret != 0) != show;
    if ( changed )
    {
        InvalidateBestSize();
    }

    return changed;
}

bool wxRadioBox::IsItemShown(unsigned int item) const
{
    wxCHECK_MSG( IsValid(item), false,
                 wxT("invalid item in wxRadioBox::IsItemShown()") );

    // don't use IsWindowVisible() here because it would return false if the
    // radiobox itself is hidden while we want to only return false if this
    // button specifically is hidden
    return (::GetWindowLong((*m_radioButtons)[item],
                            GWL_STYLE) & WS_VISIBLE) != 0;
}

#if wxUSE_TOOLTIPS

bool wxRadioBox::HasToolTips() const
{
    return wxStaticBox::HasToolTips() || wxRadioBoxBase::HasItemToolTips();
}

void wxRadioBox::DoSetItemToolTip(unsigned int item, wxToolTip *tooltip)
{
    // we have already checked for the item to be valid in wxRadioBoxBase
    const HWND hwndRbtn = (*m_radioButtons)[item];
    if ( tooltip != NULL )
        tooltip->Add(hwndRbtn);
    else // unset the tooltip
        wxToolTip::Remove(hwndRbtn);
}

#endif // wxUSE_TOOLTIPS

bool wxRadioBox::Reparent(wxWindowBase *newParent)
{
    if ( !wxStaticBox::Reparent(newParent) )
    {
        return false;
    }

    HWND hwndParent = GetHwndOf(GetParent());
    for ( size_t item = 0; item < m_radioButtons->GetCount(); item++ )
    {
        ::SetParent((*m_radioButtons)[item], hwndParent);
    }
#ifdef __WXWINCE__
    // put static box under the buttons in the Z-order
    SetWindowPos(GetHwnd(), HWND_BOTTOM, 0, 0, 0, 0, SWP_NOMOVE|SWP_NOSIZE);
#endif
    return true;
}

WX_FORWARD_STD_METHODS_TO_SUBWINDOWS(wxRadioBox, wxStaticBox, m_radioButtons)

// ----------------------------------------------------------------------------
// size calculations
// ----------------------------------------------------------------------------

wxSize wxRadioBox::GetMaxButtonSize() const
{
    // calculate the max button size
    int widthMax = 0,
        heightMax = 0;
    const unsigned int count = GetCount();
    for ( unsigned int i = 0 ; i < count; i++ )
    {
        int width, height;
        if ( m_radioWidth[i] < 0 )
        {
            GetTextExtent(wxGetWindowText((*m_radioButtons)[i]), &width, &height);

            // adjust the size to take into account the radio box itself
            // FIXME this is totally bogus!
            width += RADIO_SIZE;
            height *= 3;
            height /= 2;
        }
        else
        {
            width = m_radioWidth[i];
            height = m_radioHeight[i];
        }

        if ( widthMax < width )
            widthMax = width;
        if ( heightMax < height )
            heightMax = height;
    }

    return wxSize(widthMax, heightMax);
}

wxSize wxRadioBox::GetTotalButtonSize(const wxSize& sizeBtn) const
{
    // the radiobox should be big enough for its buttons
    int cx1, cy1;
    wxGetCharSize(m_hWnd, &cx1, &cy1, GetFont());

    int extraHeight = cy1;

    int height = GetRowCount() * sizeBtn.y + cy1/2 + extraHeight;
    int width  = GetColumnCount() * (sizeBtn.x + cx1) + cx1;

    // Add extra space under the label, if it exists.
    if (!wxControl::GetLabel().empty())
        height += cy1/2;

    // and also wide enough for its label
    int widthLabel;
    GetTextExtent(GetLabelText(), &widthLabel, NULL);
    widthLabel += RADIO_SIZE; // FIXME this is bogus too
    if ( widthLabel > width )
        width = widthLabel;

    return wxSize(width, height);
}

wxSize wxRadioBox::DoGetBestSize() const
{
    if ( !m_radioButtons )
    {
        // if we're not fully initialized yet, we can't meaningfully compute
        // our best size, we'll do it later
        return wxSize(1, 1);
    }

    wxSize best = GetTotalButtonSize(GetMaxButtonSize());
    CacheBestSize(best);
    return best;
}

// Restored old code.
void wxRadioBox::DoSetSize(int x, int y, int width, int height, int sizeFlags)
{
    int currentX, currentY;
    GetPosition(&currentX, &currentY);
    int widthOld, heightOld;
    GetSize(&widthOld, &heightOld);

    int xx = x;
    int yy = y;

    if (x == wxDefaultCoord && !(sizeFlags & wxSIZE_ALLOW_MINUS_ONE))
        xx = currentX;
    if (y == wxDefaultCoord && !(sizeFlags & wxSIZE_ALLOW_MINUS_ONE))
        yy = currentY;

    int y_offset = yy;
    int x_offset = xx;

    int cx1, cy1;
    wxGetCharSize(m_hWnd, &cx1, &cy1, GetFont());

    // Attempt to have a look coherent with other platforms: We compute the
    // biggest toggle dim, then we align all items according this value.
    wxSize maxSize = GetMaxButtonSize();
    int maxWidth = maxSize.x,
        maxHeight = maxSize.y;

    wxSize totSize = GetTotalButtonSize(maxSize);
    int totWidth = totSize.x,
        totHeight = totSize.y;

    // only change our width/height if asked for
    if ( width == wxDefaultCoord )
    {
        if ( sizeFlags & wxSIZE_AUTO_WIDTH )
            width = totWidth;
        else
            width = widthOld;
    }

    if ( height == wxDefaultCoord )
    {
        if ( sizeFlags & wxSIZE_AUTO_HEIGHT )
            height = totHeight;
        else
            height = heightOld;
    }

    DoMoveWindow(xx, yy, width, height);

    // Now position all the buttons: the current button will be put at
    // wxPoint(x_offset, y_offset) and the new row/column will start at
    // startX/startY. The size of all buttons will be the same wxSize(maxWidth,
    // maxHeight) except for the buttons in the last column which should extend
    // to the right border of radiobox and thus can be wider than this.

    // Also, remember that wxRA_SPECIFY_COLS means that we arrange buttons in
    // left to right order and GetMajorDim() is the number of columns while
    // wxRA_SPECIFY_ROWS means that the buttons are arranged top to bottom and
    // GetMajorDim() is the number of rows.

    x_offset += cx1;
    y_offset += cy1;

    // Add extra space under the label, if it exists.
    if (!wxControl::GetLabel().empty())
        y_offset += cy1/2;

    int startX = x_offset;
    int startY = y_offset;

    const unsigned int count = GetCount();
    for (unsigned int i = 0; i < count; i++)
    {
        // the last button in the row may be wider than the other ones as the
        // radiobox may be wider than the sum of the button widths (as it
        // happens, for example, when the radiobox label is very long)
        bool isLastInTheRow;
        if ( m_windowStyle & wxRA_SPECIFY_COLS )
        {
            // item is the last in its row if it is a multiple of the number of
            // columns or if it is just the last item
            unsigned int n = i + 1;
            isLastInTheRow = ((n % GetMajorDim()) == 0) || (n == count);
        }
        else // wxRA_SPECIFY_ROWS
        {
            // item is the last in the row if it is in the last columns
            isLastInTheRow = i >= (count/GetMajorDim())*GetMajorDim();
        }

        // is this the start of new row/column?
        if ( i && (i % GetMajorDim() == 0) )
        {
            if ( m_windowStyle & wxRA_SPECIFY_ROWS )
            {
                // start of new column
                y_offset = startY;
                x_offset += maxWidth + cx1;
            }
            else // start of new row
            {
                x_offset = startX;
                y_offset += maxHeight;
                if (m_radioWidth[0]>0)
                    y_offset += cy1/2;
            }
        }

        int widthBtn;
        if ( isLastInTheRow )
        {
            // make the button go to the end of radio box
            widthBtn = startX + width - x_offset - 2*cx1;
            if ( widthBtn < maxWidth )
                widthBtn = maxWidth;
        }
        else
        {
            // normal button, always of the same size
            widthBtn = maxWidth;
        }

        // make all buttons of the same, maximal size - like this they cover
        // the radiobox entirely and the radiobox tooltips are always shown
        // (otherwise they are not when the mouse pointer is in the radiobox
        // part not belonging to any radiobutton)
        DoMoveSibling((*m_radioButtons)[i], x_offset, y_offset, widthBtn, maxHeight);

        // where do we put the next button?
        if ( m_windowStyle & wxRA_SPECIFY_ROWS )
        {
            // below this one
            y_offset += maxHeight;
            if (m_radioWidth[0]>0)
                y_offset += cy1/2;
        }
        else
        {
            // to the right of this one
            x_offset += widthBtn + cx1;
        }
    }
}

int wxRadioBox::GetItemFromPoint(const wxPoint& pt) const
{
    const unsigned int count = GetCount();
    for ( unsigned int i = 0; i < count; i++ )
    {
        RECT rect = wxGetWindowRect((*m_radioButtons)[i]);

        if ( rect.left <= pt.x && pt.x < rect.right &&
                rect.top  <= pt.y && pt.y < rect.bottom )
        {
            return i;
        }
    }

    return wxNOT_FOUND;
}

// ----------------------------------------------------------------------------
// radio box drawing
// ----------------------------------------------------------------------------

#ifndef __WXWINCE__

WXHRGN wxRadioBox::MSWGetRegionWithoutChildren()
{
    RECT rc;
    ::GetWindowRect(GetHwnd(), &rc);
    HRGN hrgn = ::CreateRectRgn(rc.left, rc.top, rc.right + 1, rc.bottom + 1);

    const unsigned int count = GetCount();
    for ( unsigned int i = 0; i < count; ++i )
    {
        // don't clip out hidden children
        if ( !IsItemShown(i) )
            continue;

        ::GetWindowRect((*m_radioButtons)[i], &rc);
        AutoHRGN hrgnchild(::CreateRectRgnIndirect(&rc));
        ::CombineRgn(hrgn, hrgn, hrgnchild, RGN_DIFF);
    }

    return (WXHRGN)hrgn;
}

#endif // __WXWINCE__

// ---------------------------------------------------------------------------
// window proc for radio buttons
// ---------------------------------------------------------------------------

LRESULT APIENTRY _EXPORT wxRadioBtnWndProc(HWND hwnd,
                                           UINT message,
                                           WPARAM wParam,
                                           LPARAM lParam)
{
    switch ( message )
    {
        case WM_GETDLGCODE:
            // we must tell IsDialogMessage()/our kbd processing code that we
            // want to process arrows ourselves because neither of them is
            // smart enough to handle arrows properly for us
            {
                long lDlgCode = ::CallWindowProc(CASTWNDPROC s_wndprocRadioBtn, hwnd,
                                                 message, wParam, lParam);

                return lDlgCode | DLGC_WANTARROWS;
            }

        case WM_KEYDOWN:
            {
                wxRadioBox *radiobox = (wxRadioBox *)wxGetWindowUserData(hwnd);

                wxCHECK_MSG( radiobox, 0, wxT("radio button without radio box?") );

                bool processed = true;

                wxDirection dir;
                switch ( wParam )
                {
                    case VK_UP:
                        dir = wxUP;
                        break;

                    case VK_LEFT:
                        dir = wxLEFT;
                        break;

                    case VK_DOWN:
                        dir = wxDOWN;
                        break;

                    case VK_RIGHT:
                        dir = wxRIGHT;
                        break;

                    default:
                        processed = false;

                        // just to suppress the compiler warning
                        dir = wxALL;
                }

                if ( processed )
                {
                    int selOld = radiobox->GetSelection();
                    int selNew = radiobox->GetNextItem
                                 (
                                  selOld,
                                  dir,
                                  radiobox->GetWindowStyle()
                                 );

                    if ( selNew != selOld )
                    {
                        radiobox->SetSelection(selNew);
                        radiobox->SetFocus();

                        // emulate the button click
                        radiobox->SendNotificationEvent();

                        return 0;
                    }
                }
            }
            break;

        case WM_SETFOCUS:
        case WM_KILLFOCUS:
            {
                wxRadioBox *radiobox = (wxRadioBox *)wxGetWindowUserData(hwnd);

                wxCHECK_MSG( radiobox, 0, wxT("radio button without radio box?") );

                // if we don't do this, no focus events are generated for the
                // radiobox and, besides, we need to notify the parent about
                // the focus change, otherwise the focus handling logic in
                // wxControlContainer doesn't work
                if ( message == WM_SETFOCUS )
                    radiobox->HandleSetFocus((WXHWND)wParam);
                else
                    radiobox->HandleKillFocus((WXHWND)wParam);
            }
            break;

#ifndef __WXWINCE__
        case WM_HELP:
            {
                wxRadioBox *radiobox = (wxRadioBox *)wxGetWindowUserData(hwnd);

                wxCHECK_MSG( radiobox, 0, wxT("radio button without radio box?") );

                bool processed = false;

                wxEvtHandler * const handler = radiobox->GetEventHandler();

                HELPINFO* info = (HELPINFO*) lParam;
                if ( info->iContextType == HELPINFO_WINDOW )
                {
                    for ( wxWindow* subjectOfHelp = radiobox;
                          subjectOfHelp;
                          subjectOfHelp = subjectOfHelp->GetParent() )
                    {
                        wxHelpEvent helpEvent(wxEVT_HELP,
                                              subjectOfHelp->GetId(),
                                              wxPoint(info->MousePos.x,
                                                      info->MousePos.y));
                        helpEvent.SetEventObject(radiobox);
                        if ( handler->ProcessEvent(helpEvent) )
                        {
                            processed = true;
                            break;
                        }
                    }
                }
                else if (info->iContextType == HELPINFO_MENUITEM)
                {
                    wxHelpEvent helpEvent(wxEVT_HELP, info->iCtrlId);
                    helpEvent.SetEventObject(radiobox);
                    processed = handler->ProcessEvent(helpEvent);
                }

                if ( processed )
                    return 0;
            }
            break;
#endif // !__WXWINCE__
    }

    return ::CallWindowProc(CASTWNDPROC s_wndprocRadioBtn, hwnd, message, wParam, lParam);
}

#endif // wxUSE_RADIOBOX
