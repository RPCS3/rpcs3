/////////////////////////////////////////////////////////////////////////////
// Name:        src/msw/combobox.cpp
// Purpose:     wxComboBox class
// Author:      Julian Smart
// Modified by:
// Created:     01/02/97
// RCS-ID:      $Id: combobox.cpp 49562 2007-10-31 18:55:58Z VZ $
// Copyright:   (c) Julian Smart
// Licence:     wxWindows licence
/////////////////////////////////////////////////////////////////////////////

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

#if wxUSE_COMBOBOX

#include "wx/combobox.h"

#ifndef WX_PRECOMP
    #include "wx/msw/wrapcctl.h" // include <commctrl.h> "properly"
    #include "wx/settings.h"
    #include "wx/log.h"
    // for wxEVT_COMMAND_TEXT_ENTER
    #include "wx/textctrl.h"
    #include "wx/app.h"
    #include "wx/brush.h"
#endif

#include "wx/clipbrd.h"
#include "wx/msw/private.h"

#if wxUSE_TOOLTIPS
    #include "wx/tooltip.h"
#endif // wxUSE_TOOLTIPS

// ----------------------------------------------------------------------------
// wxWin macros
// ----------------------------------------------------------------------------

#if wxUSE_EXTENDED_RTTI
WX_DEFINE_FLAGS( wxComboBoxStyle )

wxBEGIN_FLAGS( wxComboBoxStyle )
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

    wxFLAGS_MEMBER(wxCB_SIMPLE)
    wxFLAGS_MEMBER(wxCB_SORT)
    wxFLAGS_MEMBER(wxCB_READONLY)
    wxFLAGS_MEMBER(wxCB_DROPDOWN)

wxEND_FLAGS( wxComboBoxStyle )

IMPLEMENT_DYNAMIC_CLASS_XTI(wxComboBox, wxControl,"wx/combobox.h")

wxBEGIN_PROPERTIES_TABLE(wxComboBox)
    wxEVENT_PROPERTY( Select , wxEVT_COMMAND_COMBOBOX_SELECTED , wxCommandEvent )
    wxEVENT_PROPERTY( TextEnter , wxEVT_COMMAND_TEXT_ENTER , wxCommandEvent )

    // TODO DELEGATES
    wxPROPERTY( Font , wxFont , SetFont , GetFont  , EMPTY_MACROVALUE , 0 /*flags*/ , wxT("Helpstring") , wxT("group"))
    wxPROPERTY_COLLECTION( Choices , wxArrayString , wxString , AppendString , GetStrings , 0 /*flags*/ , wxT("Helpstring") , wxT("group"))
    wxPROPERTY( Value ,wxString, SetValue, GetValue, EMPTY_MACROVALUE , 0 /*flags*/ , wxT("Helpstring") , wxT("group"))
    wxPROPERTY( Selection ,int, SetSelection, GetSelection, EMPTY_MACROVALUE , 0 /*flags*/ , wxT("Helpstring") , wxT("group"))
    wxPROPERTY_FLAGS( WindowStyle , wxComboBoxStyle , long , SetWindowStyleFlag , GetWindowStyleFlag , EMPTY_MACROVALUE , 0 /*flags*/ , wxT("Helpstring") , wxT("group")) // style
wxEND_PROPERTIES_TABLE()

wxBEGIN_HANDLERS_TABLE(wxComboBox)
wxEND_HANDLERS_TABLE()

wxCONSTRUCTOR_5( wxComboBox , wxWindow* , Parent , wxWindowID , Id , wxString , Value , wxPoint , Position , wxSize , Size )

#else

IMPLEMENT_DYNAMIC_CLASS(wxComboBox, wxControl)

#endif

BEGIN_EVENT_TABLE(wxComboBox, wxControl)
    EVT_MENU(wxID_CUT, wxComboBox::OnCut)
    EVT_MENU(wxID_COPY, wxComboBox::OnCopy)
    EVT_MENU(wxID_PASTE, wxComboBox::OnPaste)
    EVT_MENU(wxID_UNDO, wxComboBox::OnUndo)
    EVT_MENU(wxID_REDO, wxComboBox::OnRedo)
    EVT_MENU(wxID_CLEAR, wxComboBox::OnDelete)
    EVT_MENU(wxID_SELECTALL, wxComboBox::OnSelectAll)

    EVT_UPDATE_UI(wxID_CUT, wxComboBox::OnUpdateCut)
    EVT_UPDATE_UI(wxID_COPY, wxComboBox::OnUpdateCopy)
    EVT_UPDATE_UI(wxID_PASTE, wxComboBox::OnUpdatePaste)
    EVT_UPDATE_UI(wxID_UNDO, wxComboBox::OnUpdateUndo)
    EVT_UPDATE_UI(wxID_REDO, wxComboBox::OnUpdateRedo)
    EVT_UPDATE_UI(wxID_CLEAR, wxComboBox::OnUpdateDelete)
    EVT_UPDATE_UI(wxID_SELECTALL, wxComboBox::OnUpdateSelectAll)
END_EVENT_TABLE()

// ----------------------------------------------------------------------------
// function prototypes
// ----------------------------------------------------------------------------

LRESULT APIENTRY _EXPORT wxComboEditWndProc(HWND hWnd,
                                            UINT message,
                                            WPARAM wParam,
                                            LPARAM lParam);

// ---------------------------------------------------------------------------
// global vars
// ---------------------------------------------------------------------------

// the pointer to standard radio button wnd proc
static WNDPROC gs_wndprocEdit = (WNDPROC)NULL;

// ============================================================================
// implementation
// ============================================================================

// ----------------------------------------------------------------------------
// wnd proc for subclassed edit control
// ----------------------------------------------------------------------------

LRESULT APIENTRY _EXPORT wxComboEditWndProc(HWND hWnd,
                                            UINT message,
                                            WPARAM wParam,
                                            LPARAM lParam)
{
    HWND hwndCombo = ::GetParent(hWnd);
    wxWindow *win = wxFindWinFromHandle((WXHWND)hwndCombo);

    switch ( message )
    {
        // forward some messages to the combobox to generate the appropriate
        // wxEvents from them
        case WM_KEYUP:
        case WM_KEYDOWN:
        case WM_CHAR:
        case WM_SYSCHAR:
        case WM_SYSKEYDOWN:
        case WM_SYSKEYUP:
        case WM_SETFOCUS:
        case WM_KILLFOCUS:
            {
                wxComboBox *combo = wxDynamicCast(win, wxComboBox);
                if ( !combo )
                {
                    // we can get WM_KILLFOCUS while our parent is already half
                    // destroyed and hence doesn't look like a combobx any
                    // longer, check for it to avoid bogus assert failures
                    if ( !win->IsBeingDeleted() )
                    {
                        wxFAIL_MSG( _T("should have combo as parent") );
                    }
                }
                else if ( combo->MSWProcessEditMsg(message, wParam, lParam) )
                {
                    // handled by parent
                    return 0;
                }
            }
            break;

        case WM_GETDLGCODE:
            {
                wxCHECK_MSG( win, 0, _T("should have a parent") );

                if ( win->GetWindowStyle() & wxTE_PROCESS_ENTER )
                {
                    // need to return a custom dlg code or we'll never get it
                    return DLGC_WANTMESSAGE;
                }
            }
            break;

        case WM_CUT:
        case WM_COPY:
        case WM_PASTE:
            if( win->HandleClipboardEvent( message ) )
                return 0;
            break;
    }

    return ::CallWindowProc(CASTWNDPROC gs_wndprocEdit, hWnd, message, wParam, lParam);
}

// ----------------------------------------------------------------------------
// wxComboBox callbacks
// ----------------------------------------------------------------------------

WXLRESULT wxComboBox::MSWWindowProc(WXUINT nMsg, WXWPARAM wParam, WXLPARAM lParam)
{
    // TODO: handle WM_CTLCOLOR messages from our EDIT control to be able to
    //       set its colour correctly (to be the same as our own one)

    switch ( nMsg )
    {
        case WM_SIZE:
        // wxStaticBox can generate this message, when modifying the control's style.
        // This causes the content of the combobox to be selected, for some reason.
        case WM_STYLECHANGED:
            {
                // combobox selection sometimes spontaneously changes when its
                // size changes, restore it to the old value if necessary
                long fromOld, toOld;
                GetSelection(&fromOld, &toOld);
                WXLRESULT result = wxChoice::MSWWindowProc(nMsg, wParam, lParam);

                long fromNew, toNew;
                GetSelection(&fromNew, &toNew);

                if ( fromOld != fromNew || toOld != toNew )
                {
                    SetSelection(fromOld, toOld);
                }

                return result;
            }
    }

    return wxChoice::MSWWindowProc(nMsg, wParam, lParam);
}

bool wxComboBox::MSWProcessEditMsg(WXUINT msg, WXWPARAM wParam, WXLPARAM lParam)
{
    switch ( msg )
    {
        case WM_CHAR:
            // for compatibility with wxTextCtrl, generate a special message
            // when Enter is pressed
            if ( wParam == VK_RETURN )
            {
                if (SendMessage(GetHwnd(), CB_GETDROPPEDSTATE, 0, 0))
                    return false;

                wxCommandEvent event(wxEVT_COMMAND_TEXT_ENTER, m_windowId);

                const int sel = GetSelection();
                event.SetInt(sel);
                event.SetString(GetValue());
                InitCommandEventWithItems(event, sel);

                if ( ProcessCommand(event) )
                {
                    // don't let the event through to the native control
                    // because it doesn't need it and may generate an annoying
                    // beep if it gets it
                    return true;
                }
            }
            // fall through

        case WM_SYSCHAR:
            return HandleChar(wParam, lParam, true /* isASCII */);

        case WM_SYSKEYDOWN:
        case WM_KEYDOWN:
            return HandleKeyDown(wParam, lParam);

        case WM_SYSKEYUP:
        case WM_KEYUP:
            return HandleKeyUp(wParam, lParam);

        case WM_SETFOCUS:
            return HandleSetFocus((WXHWND)wParam);

        case WM_KILLFOCUS:
            return HandleKillFocus((WXHWND)wParam);
    }

    return false;
}

bool wxComboBox::MSWCommand(WXUINT param, WXWORD id)
{
    int sel = -1;
    wxString value;

    switch ( param )
    {
        case CBN_SELENDOK:
#ifndef __SMARTPHONE__
            // we need to reset this to prevent the selection from being undone
            // by wxChoice, see wxChoice::MSWCommand() and comments there
            m_lastAcceptedSelection = wxID_NONE;
#endif

            // set these variables so that they could be also fixed in
            // CBN_EDITCHANGE below
            sel = GetSelection();
            value = GetStringSelection();

            // this string is going to become the new combobox value soon but
            // we need it to be done right now, otherwise the event handler
            // could get a wrong value when it calls our GetValue()
            ::SetWindowText(GetHwnd(), value);

            {
                wxCommandEvent event(wxEVT_COMMAND_COMBOBOX_SELECTED, GetId());
                event.SetInt(sel);
                event.SetString(value);
                InitCommandEventWithItems(event, sel);

                ProcessCommand(event);
            }

            // fall through: for compability with wxGTK, also send the text
            // update event when the selection changes (this also seems more
            // logical as the text does change)

        case CBN_EDITCHANGE:
            {
                wxCommandEvent event(wxEVT_COMMAND_TEXT_UPDATED, GetId());

                // if sel != -1, value was already initialized above
                if ( sel == -1 )
                {
                    value = wxGetWindowText(GetHwnd());
                }

                event.SetString(value);
                InitCommandEventWithItems(event, sel);

                ProcessCommand(event);
            }
            break;

        default:
            return wxChoice::MSWCommand(param, id);
    }

    // skip wxChoice version as it would generate its own events for
    // CBN_SELENDOK
    return true;
}

bool wxComboBox::MSWShouldPreProcessMessage(WXMSG *pMsg)
{
    // prevent command accelerators from stealing editing
    // hotkeys when we have the focus
    if (wxIsCtrlDown())
    {
        WPARAM vkey = pMsg->wParam;

        switch (vkey)
        {
            case 'C':
            case 'V':
            case 'X':
            case VK_INSERT:
            case VK_DELETE:
            case VK_HOME:
            case VK_END:
                return false;
        }
    }

    return wxChoice::MSWShouldPreProcessMessage(pMsg);
}

WXHWND wxComboBox::GetEditHWND() const
{
    // this function should not be called for wxCB_READONLY controls, it is
    // the callers responsability to check this
    wxASSERT_MSG( !(GetWindowStyle() & wxCB_READONLY),
                  _T("read-only combobox doesn't have any edit control") );

    POINT pt;
    pt.x = pt.y = 4;
    HWND hwndEdit = ::ChildWindowFromPoint(GetHwnd(), pt);
    if ( !hwndEdit || hwndEdit == GetHwnd() )
    {
        wxFAIL_MSG(_T("not read only combobox without edit control?"));
    }

    return (WXHWND)hwndEdit;
}

// ----------------------------------------------------------------------------
// wxComboBox creation
// ----------------------------------------------------------------------------

bool wxComboBox::Create(wxWindow *parent, wxWindowID id,
                        const wxString& value,
                        const wxPoint& pos,
                        const wxSize& size,
                        int n, const wxString choices[],
                        long style,
                        const wxValidator& validator,
                        const wxString& name)
{
    // pretend that wxComboBox is hidden while it is positioned and resized and
    // show it only right before leaving this method because otherwise there is
    // some noticeable flicker while the control rearranges itself
    m_isShown = false;

    if ( !CreateAndInit(parent, id, pos, size, n, choices, style,
                        validator, name) )
        return false;

    // we shouldn't call SetValue() for an empty string because this would
    // (correctly) result in an assert with a read only combobox and is useless
    // for the other ones anyhow
    if ( !value.empty() )
        SetValue(value);

    // a (not read only) combobox is, in fact, 2 controls: the combobox itself
    // and an edit control inside it and if we want to catch events from this
    // edit control, we must subclass it as well
    if ( !(style & wxCB_READONLY) )
    {
        gs_wndprocEdit = wxSetWindowProc((HWND)GetEditHWND(), wxComboEditWndProc);
    }

    // and finally, show the control
    Show(true);

    return true;
}

bool wxComboBox::Create(wxWindow *parent, wxWindowID id,
                        const wxString& value,
                        const wxPoint& pos,
                        const wxSize& size,
                        const wxArrayString& choices,
                        long style,
                        const wxValidator& validator,
                        const wxString& name)
{
    wxCArrayString chs(choices);
    return Create(parent, id, value, pos, size, chs.GetCount(),
                  chs.GetStrings(), style, validator, name);
}

WXDWORD wxComboBox::MSWGetStyle(long style, WXDWORD *exstyle) const
{
    // we never have an external border
    WXDWORD msStyle = wxChoice::MSWGetStyle
                      (
                        (style & ~wxBORDER_MASK) | wxBORDER_NONE, exstyle
                      );

    // usually WS_TABSTOP is added by wxControl::MSWGetStyle() but as we're
    // created hidden (see Create() above), it is not done for us but we still
    // want to have this style
    msStyle |= WS_TABSTOP;

    // remove the style always added by wxChoice
    msStyle &= ~CBS_DROPDOWNLIST;

    if ( style & wxCB_READONLY )
        msStyle |= CBS_DROPDOWNLIST;
#ifndef __WXWINCE__
    else if ( style & wxCB_SIMPLE )
        msStyle |= CBS_SIMPLE; // A list (shown always) and edit control
#endif
    else
        msStyle |= CBS_DROPDOWN;

    // there is no reason to not always use CBS_AUTOHSCROLL, so do use it
    msStyle |= CBS_AUTOHSCROLL;

    // NB: we used to also add CBS_NOINTEGRALHEIGHT here but why?

    return msStyle;
}

// ----------------------------------------------------------------------------
// wxComboBox text control-like methods
// ----------------------------------------------------------------------------

wxString wxComboBox::GetValue() const
{
    return wxGetWindowText(m_hWnd);
}

void wxComboBox::SetValue(const wxString& value)
{
    if ( HasFlag(wxCB_READONLY) )
        SetStringSelection(value);
    else
        SetWindowText(GetHwnd(), value.c_str());
}

// Clipboard operations
void wxComboBox::Copy()
{
    SendMessage(GetHwnd(), WM_COPY, 0, 0L);
}

void wxComboBox::Cut()
{
    SendMessage(GetHwnd(), WM_CUT, 0, 0L);
}

void wxComboBox::Paste()
{
    SendMessage(GetHwnd(), WM_PASTE, 0, 0L);
}

void wxComboBox::Undo()
{
    if (CanUndo())
    {
        HWND hEditWnd = (HWND) GetEditHWND();
        if ( hEditWnd )
            ::SendMessage(hEditWnd, EM_UNDO, 0, 0);
    }
}

void wxComboBox::Redo()
{
    if (CanUndo())
    {
        // Same as Undo, since Undo undoes the undo, i.e. a redo.
        HWND hEditWnd = (HWND) GetEditHWND();
        if ( hEditWnd )
            ::SendMessage(hEditWnd, EM_UNDO, 0, 0);
    }
}

void wxComboBox::SelectAll()
{
    SetSelection(0, GetLastPosition());
}

bool wxComboBox::CanUndo() const
{
    if (!IsEditable())
        return false;

    HWND hEditWnd = (HWND) GetEditHWND();
    if ( hEditWnd )
        return ::SendMessage(hEditWnd, EM_CANUNDO, 0, 0) != 0;
    else
        return false;
}

bool wxComboBox::CanRedo() const
{
    if (!IsEditable())
        return false;

    HWND hEditWnd = (HWND) GetEditHWND();
    if ( hEditWnd )
        return ::SendMessage(hEditWnd, EM_CANUNDO, 0, 0) != 0;
    else
        return false;
}

bool wxComboBox::HasSelection() const
{
    long from, to;
    GetSelection(&from, &to);
    return from != to;
}

bool wxComboBox::CanCopy() const
{
    // Can copy if there's a selection
    return HasSelection();
}

bool wxComboBox::CanCut() const
{
    return IsEditable() && CanCopy();
}

bool wxComboBox::CanPaste() const
{
    if ( !IsEditable() )
        return false;

    // Standard edit control: check for straight text on clipboard
    if ( !::OpenClipboard(GetHwndOf(wxTheApp->GetTopWindow())) )
        return false;

    bool isTextAvailable = ::IsClipboardFormatAvailable(CF_TEXT) != 0;
    ::CloseClipboard();

    return isTextAvailable;
}

bool wxComboBox::IsEditable() const
{
    return !HasFlag(wxCB_READONLY);
}

void wxComboBox::SetEditable(bool editable)
{
    HWND hWnd = (HWND)GetEditHWND();
    if ( !::SendMessage(hWnd, EM_SETREADONLY, !editable, 0) )
    {
        wxLogLastError(_T("SendMessage(EM_SETREADONLY)"));
    }
}

void wxComboBox::SetInsertionPoint(long pos)
{
    if ( GetWindowStyle() & wxCB_READONLY )
        return;

    HWND hWnd = GetHwnd();
    ::SendMessage(hWnd, CB_SETEDITSEL, 0, MAKELPARAM(pos, pos));
    HWND hEditWnd = (HWND) GetEditHWND();
    if ( hEditWnd )
    {
        // Scroll insertion point into view
        SendMessage(hEditWnd, EM_SCROLLCARET, (WPARAM)0, (LPARAM)0);
    }
}

void wxComboBox::SetInsertionPointEnd()
{
    // setting insertion point doesn't make sense for read only comboboxes
    if ( !(GetWindowStyle() & wxCB_READONLY) )
    {
        wxTextPos pos = GetLastPosition();
        SetInsertionPoint(pos);
    }
}

long wxComboBox::GetInsertionPoint() const
{
    // CB_GETEDITSEL returns the index of the first character of the selection in
    // its low-order word
    DWORD pos= (DWORD)::SendMessage(GetHwnd(), CB_GETEDITSEL, 0, 0L);
    return LOWORD(pos);
}

wxTextPos wxComboBox::GetLastPosition() const
{
    HWND hEditWnd = (HWND) GetEditHWND();

    // Get number of characters in the last (only) line. We'll add this to the character
    // index for the last line, 1st position.
    wxTextPos lineLength = (wxTextPos)SendMessage(hEditWnd, EM_LINELENGTH, (WPARAM) 0, (LPARAM)0L);

    return lineLength;
}

void wxComboBox::Replace(long from, long to, const wxString& value)
{
#if wxUSE_CLIPBOARD
    Remove(from, to);

    // Now replace with 'value', by pasting.
    wxSetClipboardData(wxDF_TEXT, (wxObject *)(const wxChar *)value, 0, 0);

    // Paste into edit control
    SendMessage(GetHwnd(), WM_PASTE, (WPARAM)0, (LPARAM)0L);
#else
    wxUnusedVar(from);
    wxUnusedVar(to);
    wxUnusedVar(value);
#endif
}

void wxComboBox::Remove(long from, long to)
{
    // Set selection and remove it
    SetSelection(from, to);
    SendMessage(GetHwnd(), WM_CUT, (WPARAM)0, (LPARAM)0);
}

void wxComboBox::SetSelection(long from, long to)
{
    // if from and to are both -1, it means (in wxWidgets) that all text should
    // be selected, translate this into Windows convention
    if ( (from == -1) && (to == -1) )
    {
        from = 0;
    }

    if ( SendMessage(GetHwnd(), CB_SETEDITSEL,
                     0, (LPARAM)MAKELONG(from, to)) == CB_ERR )
    {
        wxLogDebug(_T("CB_SETEDITSEL failed"));
    }
}

void wxComboBox::GetSelection(long* from, long* to) const
{
    DWORD dwStart, dwEnd;
    if ( ::SendMessage(GetHwnd(), CB_GETEDITSEL,
                       (WPARAM)&dwStart, (LPARAM)&dwEnd) == CB_ERR )
    {
        *from =
        *to = 0;
    }
    else
    {
        *from = dwStart;
        *to = dwEnd;
    }
}

// ----------------------------------------------------------------------------
// standard event handling
// ----------------------------------------------------------------------------

void wxComboBox::OnCut(wxCommandEvent& WXUNUSED(event))
{
    Cut();
}

void wxComboBox::OnCopy(wxCommandEvent& WXUNUSED(event))
{
    Copy();
}

void wxComboBox::OnPaste(wxCommandEvent& WXUNUSED(event))
{
    Paste();
}

void wxComboBox::OnUndo(wxCommandEvent& WXUNUSED(event))
{
    Undo();
}

void wxComboBox::OnRedo(wxCommandEvent& WXUNUSED(event))
{
    Redo();
}

void wxComboBox::OnDelete(wxCommandEvent& WXUNUSED(event))
{
    long from, to;
    GetSelection(& from, & to);
    if (from != -1 && to != -1)
        Remove(from, to);
}

void wxComboBox::OnSelectAll(wxCommandEvent& WXUNUSED(event))
{
    SetSelection(-1, -1);
}

void wxComboBox::OnUpdateCut(wxUpdateUIEvent& event)
{
    event.Enable( CanCut() );
}

void wxComboBox::OnUpdateCopy(wxUpdateUIEvent& event)
{
    event.Enable( CanCopy() );
}

void wxComboBox::OnUpdatePaste(wxUpdateUIEvent& event)
{
    event.Enable( CanPaste() );
}

void wxComboBox::OnUpdateUndo(wxUpdateUIEvent& event)
{
    event.Enable( CanUndo() );
}

void wxComboBox::OnUpdateRedo(wxUpdateUIEvent& event)
{
    event.Enable( CanRedo() );
}

void wxComboBox::OnUpdateDelete(wxUpdateUIEvent& event)
{
    event.Enable(IsEditable() && HasSelection());
}

void wxComboBox::OnUpdateSelectAll(wxUpdateUIEvent& event)
{
    event.Enable(IsEditable() && GetLastPosition() > 0);
}

#endif // wxUSE_COMBOBOX
