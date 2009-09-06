///////////////////////////////////////////////////////////////////////////////
// Name:        src/msw/wince/textctrlce.cpp
// Purpose:     wxTextCtrl implementation for smart phones driven by WinCE
// Author:      Wlodzimierz ABX Skiba
// Modified by:
// Created:     30.08.2004
// RCS-ID:      $Id: textctrlce.cpp 42816 2006-10-31 08:50:17Z RD $
// Copyright:   (c) Wlodzimierz Skiba
// License:     wxWindows licence
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

#if wxUSE_TEXTCTRL && defined(__SMARTPHONE__) && defined(__WXWINCE__)

#include "wx/textctrl.h"

#ifndef WX_PRECOMP
    #include "wx/msw/wrapcctl.h" // include <commctrl.h> "properly"
#endif

#include "wx/spinbutt.h"
#include "wx/textfile.h"

#define GetBuddyHwnd()      (HWND)(m_hwndBuddy)

#define IsVertical(wxStyle) (true)

// ----------------------------------------------------------------------------
// event tables and other macros
// ----------------------------------------------------------------------------

#if wxUSE_EXTENDED_RTTI
// TODO
#else
IMPLEMENT_DYNAMIC_CLASS(wxTextCtrl, wxControl)
#endif

BEGIN_EVENT_TABLE(wxTextCtrl, wxControl)
    EVT_CHAR(wxTextCtrl::OnChar)

    EVT_MENU(wxID_CUT, wxTextCtrl::OnCut)
    EVT_MENU(wxID_COPY, wxTextCtrl::OnCopy)
    EVT_MENU(wxID_PASTE, wxTextCtrl::OnPaste)
    EVT_MENU(wxID_UNDO, wxTextCtrl::OnUndo)
    EVT_MENU(wxID_REDO, wxTextCtrl::OnRedo)
    EVT_MENU(wxID_CLEAR, wxTextCtrl::OnDelete)
    EVT_MENU(wxID_SELECTALL, wxTextCtrl::OnSelectAll)

    EVT_UPDATE_UI(wxID_CUT, wxTextCtrl::OnUpdateCut)
    EVT_UPDATE_UI(wxID_COPY, wxTextCtrl::OnUpdateCopy)
    EVT_UPDATE_UI(wxID_PASTE, wxTextCtrl::OnUpdatePaste)
    EVT_UPDATE_UI(wxID_UNDO, wxTextCtrl::OnUpdateUndo)
    EVT_UPDATE_UI(wxID_REDO, wxTextCtrl::OnUpdateRedo)
    EVT_UPDATE_UI(wxID_CLEAR, wxTextCtrl::OnUpdateDelete)
    EVT_UPDATE_UI(wxID_SELECTALL, wxTextCtrl::OnUpdateSelectAll)

    EVT_SET_FOCUS(wxTextCtrl::OnSetFocus)
END_EVENT_TABLE()

// ----------------------------------------------------------------------------
// constants
// ----------------------------------------------------------------------------

// the margin between the up-down control and its buddy (can be arbitrary,
// choose what you like - or may be decide during run-time depending on the
// font size?)
static const int MARGIN_BETWEEN = 0;

// ============================================================================
// implementation
// ============================================================================

wxArrayTextSpins wxTextCtrl::ms_allTextSpins;

// ----------------------------------------------------------------------------
// wnd proc for the buddy text ctrl
// ----------------------------------------------------------------------------

LRESULT APIENTRY _EXPORT wxBuddyTextCtrlWndProc(HWND hwnd,
                                                UINT message,
                                                WPARAM wParam,
                                                LPARAM lParam)
{
    wxTextCtrl *spin = (wxTextCtrl *)wxGetWindowUserData(hwnd);

    // forward some messages (the key and focus ones only so far) to
    // the spin ctrl
    switch ( message )
    {
        case WM_SETFOCUS:
            // if the focus comes from the spin control itself, don't set it
            // back to it -- we don't want to go into an infinite loop
            if ( (WXHWND)wParam == spin->GetHWND() )
                break;
            //else: fall through

        case WM_KILLFOCUS:
        case WM_CHAR:
        case WM_DEADCHAR:
        case WM_KEYUP:
        case WM_KEYDOWN:
            spin->MSWWindowProc(message, wParam, lParam);

            // The control may have been deleted at this point, so check.
            if ( !::IsWindow(hwnd) || wxGetWindowUserData(hwnd) != spin )
                return 0;
            break;

        case WM_GETDLGCODE:
            // we want to get WXK_RETURN in order to generate the event for it
            return DLGC_WANTCHARS;
    }

    return ::CallWindowProc(CASTWNDPROC spin->GetBuddyWndProc(),
                            hwnd, message, wParam, lParam);
}

// ----------------------------------------------------------------------------
// creation
// ----------------------------------------------------------------------------

void wxTextCtrl::Init()
{
    m_suppressNextUpdate = false;
    m_isNativeCaretShown = true;
}

wxTextCtrl::~wxTextCtrl()
{
}

bool wxTextCtrl::Create(wxWindow *parent, wxWindowID id,
                        const wxString& value,
                        const wxPoint& pos,
                        const wxSize& size,
                        long style,
                        const wxValidator& validator,
                        const wxString& name)
{
    if ( (style & wxBORDER_MASK) == wxBORDER_DEFAULT )
        style |= wxBORDER_SIMPLE;

    SetWindowStyle(style);

    WXDWORD exStyle = 0;
    WXDWORD msStyle = MSWGetStyle(GetWindowStyle(), & exStyle) ;

    wxSize sizeText(size), sizeBtn(size);
    sizeBtn.x = GetBestSpinnerSize(IsVertical(style)).x / 2;

    if ( sizeText.x == wxDefaultCoord )
    {
        // DEFAULT_ITEM_WIDTH is the default width for the text control
        sizeText.x = DEFAULT_ITEM_WIDTH + MARGIN_BETWEEN + sizeBtn.x;
    }

    sizeText.x -= sizeBtn.x + MARGIN_BETWEEN;
    if ( sizeText.x <= 0 )
    {
        wxLogDebug(_T("not enough space for wxSpinCtrl!"));
    }

    wxPoint posBtn(pos);
    posBtn.x += sizeText.x + MARGIN_BETWEEN;

    // we need to turn '\n's into "\r\n"s for the multiline controls
    wxString valueWin;
    if ( m_windowStyle & wxTE_MULTILINE )
    {
        valueWin = wxTextFile::Translate(value, wxTextFileType_Dos);
    }
    else // single line
    {
        valueWin = value;
    }

    // we must create the list control before the spin button for the purpose
    // of the dialog navigation: if there is a static text just before the spin
    // control, activating it by Alt-letter should give focus to the text
    // control, not the spin and the dialog navigation code will give focus to
    // the next control (at Windows level), not the one after it

    // create the text window

    m_hwndBuddy = (WXHWND)::CreateWindowEx
                    (
                     exStyle,                // sunken border
                     _T("EDIT"),             // window class
                     valueWin,               // no window title
                     msStyle,                // style (will be shown later)
                     pos.x, pos.y,           // position
                     0, 0,                   // size (will be set later)
                     GetHwndOf(parent),      // parent
                     (HMENU)-1,              // control id
                     wxGetInstance(),        // app instance
                     NULL                    // unused client data
                    );

    if ( !m_hwndBuddy )
    {
        wxLogLastError(wxT("CreateWindow(buddy text window)"));

        return false;
    }

    // initialize wxControl
    if ( !CreateControl(parent, id, posBtn, sizeBtn, style, validator, name) )
        return false;

    // now create the real HWND
    WXDWORD spiner_style = WS_VISIBLE |
                           UDS_ALIGNRIGHT |
                           UDS_EXPANDABLE |
                           UDS_NOSCROLL;

    if ( !IsVertical(style) )
        spiner_style |= UDS_HORZ;

    if ( style & wxSP_WRAP )
        spiner_style |= UDS_WRAP;

    if ( !MSWCreateControl(UPDOWN_CLASS, spiner_style, posBtn, sizeBtn, _T(""), 0) )
        return false;

    // subclass the text ctrl to be able to intercept some events
    wxSetWindowUserData(GetBuddyHwnd(), this);
    m_wndProcBuddy = (WXFARPROC)wxSetWindowProc(GetBuddyHwnd(),
                                                wxBuddyTextCtrlWndProc);

    // set up fonts and colours  (This is nomally done in MSWCreateControl)
    InheritAttributes();
    if (!m_hasFont)
        SetFont(GetDefaultAttributes().font);

    // set the size of the text window - can do it only now, because we
    // couldn't call DoGetBestSize() before as font wasn't set
    if ( sizeText.y <= 0 )
    {
        int cx, cy;
        wxGetCharSize(GetHWND(), &cx, &cy, GetFont());

        sizeText.y = EDIT_HEIGHT_FROM_CHAR_HEIGHT(cy);
    }

    SetInitialSize(size);

    (void)::ShowWindow(GetBuddyHwnd(), SW_SHOW);

    // associate the list window with the spin button
    (void)::SendMessage(GetHwnd(), UDM_SETBUDDY, (WPARAM)GetBuddyHwnd(), 0);

    // do it after finishing with m_hwndBuddy creation to avoid generating
    // initial wxEVT_COMMAND_TEXT_UPDATED message
    ms_allTextSpins.Add(this);

    return true;
}

// Make sure the window style (etc.) reflects the HWND style (roughly)
void wxTextCtrl::AdoptAttributesFromHWND()
{
    wxWindow::AdoptAttributesFromHWND();

    long style = ::GetWindowLong(GetBuddyHwnd(), GWL_STYLE);

    if (style & ES_MULTILINE)
        m_windowStyle |= wxTE_MULTILINE;
    if (style & ES_PASSWORD)
        m_windowStyle |= wxTE_PASSWORD;
    if (style & ES_READONLY)
        m_windowStyle |= wxTE_READONLY;
    if (style & ES_WANTRETURN)
        m_windowStyle |= wxTE_PROCESS_ENTER;
    if (style & ES_CENTER)
        m_windowStyle |= wxTE_CENTRE;
    if (style & ES_RIGHT)
        m_windowStyle |= wxTE_RIGHT;
}

WXDWORD wxTextCtrl::MSWGetStyle(long style, WXDWORD *exstyle) const
{
    // we never have an external border
    WXDWORD msStyle = wxControl::MSWGetStyle
                      (
                        (style & ~wxBORDER_MASK) | wxBORDER_NONE, exstyle
                      );

    msStyle |= WS_VISIBLE;

    // styles which we alaways add by default
    if ( style & wxTE_MULTILINE )
    {
        wxASSERT_MSG( !(style & wxTE_PROCESS_ENTER),
                      wxT("wxTE_PROCESS_ENTER style is ignored for multiline text controls (they always process it)") );

        msStyle |= ES_MULTILINE | ES_WANTRETURN;
        if ( !(style & wxTE_NO_VSCROLL) )
        {
            // always adjust the vertical scrollbar automatically if we have it
            msStyle |= WS_VSCROLL | ES_AUTOVSCROLL;
        }

        style |= wxTE_PROCESS_ENTER;
    }
    else // !multiline
    {
        // there is really no reason to not have this style for single line
        // text controls
        msStyle |= ES_AUTOHSCROLL;
    }

    // note that wxTE_DONTWRAP is the same as wxHSCROLL so if we have a horz
    // scrollbar, there is no wrapping -- which makes sense
    if ( style & wxTE_DONTWRAP )
    {
        // automatically scroll the control horizontally as necessary
        //
        // NB: ES_AUTOHSCROLL is needed for richedit controls or they don't
        //     show horz scrollbar at all, even in spite of WS_HSCROLL, and as
        //     it doesn't seem to do any harm for plain edit controls, add it
        //     always
        msStyle |= WS_HSCROLL | ES_AUTOHSCROLL;
    }

    if ( style & wxTE_READONLY )
        msStyle |= ES_READONLY;

    if ( style & wxTE_PASSWORD )
        msStyle |= ES_PASSWORD;

    if ( style & wxTE_NOHIDESEL )
        msStyle |= ES_NOHIDESEL;

    // note that we can't do do "& wxTE_LEFT" as wxTE_LEFT == 0
    if ( style & wxTE_CENTRE )
        msStyle |= ES_CENTER;
    else if ( style & wxTE_RIGHT )
        msStyle |= ES_RIGHT;
    else
        msStyle |= ES_LEFT; // ES_LEFT is 0 as well but for consistency...

    return msStyle;
}

// ----------------------------------------------------------------------------
// set/get the controls text
// ----------------------------------------------------------------------------

wxString wxTextCtrl::GetValue() const
{
    // range 0..-1 is special for GetRange() and means to retrieve all text
    return GetRange(0, -1);
}

wxString wxTextCtrl::GetRange(long from, long to) const
{
    wxString str;

    if ( from >= to && to != -1 )
    {
        // nothing to retrieve
        return str;
    }

    // retrieve all text
    str = wxGetWindowText(GetBuddyHwnd());

    // need only a range?
    if ( from < to )
    {
        str = str.Mid(from, to - from);
    }

    // WM_GETTEXT uses standard DOS CR+LF (\r\n) convention - convert to the
    // canonical one (same one as above) for consistency with the other kinds
    // of controls and, more importantly, with the other ports
    str = wxTextFile::Translate(str, wxTextFileType_Unix);

    return str;
}

void wxTextCtrl::DoSetValue(const wxString& value, int flags)
{
    // if the text is long enough, it's faster to just set it instead of first
    // comparing it with the old one (chances are that it will be different
    // anyhow, this comparison is there to avoid flicker for small single-line
    // edit controls mostly)
    if ( (value.length() > 0x400) || (value != GetValue()) )
    {
        DoWriteText(value, flags);

        // for compatibility, don't move the cursor when doing SetValue()
        SetInsertionPoint(0);
    }
    else // same text
    {
        // still send an event for consistency
        if ( flags & SetValue_SendEvent )
            SendUpdateEvent();
    }

    // we should reset the modified flag even if the value didn't really change

    // mark the control as being not dirty - we changed its text, not the
    // user
    DiscardEdits();
}

void wxTextCtrl::WriteText(const wxString& value)
{
    DoWriteText(value);
}

void wxTextCtrl::DoWriteText(const wxString& value, int flags)
{
    bool selectionOnly = (flags & SetValue_SelectionOnly) != 0;
    wxString valueDos;
    if ( m_windowStyle & wxTE_MULTILINE )
        valueDos = wxTextFile::Translate(value, wxTextFileType_Dos);
    else
        valueDos = value;

    // in some cases we get 2 EN_CHANGE notifications after the SendMessage
    // call below which is confusing for the client code and so should be
    // avoided
    //
    if ( selectionOnly && HasSelection() )
    {
        m_suppressNextUpdate = true;
    }

    ::SendMessage(GetBuddyHwnd(), selectionOnly ? EM_REPLACESEL : WM_SETTEXT,
                  0, (LPARAM)valueDos.c_str());

    if ( !selectionOnly && !( flags & SetValue_SendEvent ) )
    {
        // Windows already sends an update event for single-line
        // controls.
        if ( m_windowStyle & wxTE_MULTILINE )
            SendUpdateEvent();
    }

    AdjustSpaceLimit();
}

void wxTextCtrl::AppendText(const wxString& text)
{
    SetInsertionPointEnd();

    WriteText(text);
}

void wxTextCtrl::Clear()
{
    ::SetWindowText(GetBuddyHwnd(), wxEmptyString);

    // Windows already sends an update event for single-line
    // controls.
    if ( m_windowStyle & wxTE_MULTILINE )
        SendUpdateEvent();
}

// ----------------------------------------------------------------------------
// Clipboard operations
// ----------------------------------------------------------------------------

void wxTextCtrl::Copy()
{
    if (CanCopy())
    {
        ::SendMessage(GetBuddyHwnd(), WM_COPY, 0, 0L);
    }
}

void wxTextCtrl::Cut()
{
    if (CanCut())
    {
        ::SendMessage(GetBuddyHwnd(), WM_CUT, 0, 0L);
    }
}

void wxTextCtrl::Paste()
{
    if (CanPaste())
    {
        ::SendMessage(GetBuddyHwnd(), WM_PASTE, 0, 0L);
    }
}

bool wxTextCtrl::HasSelection() const
{
    long from, to;
    GetSelection(&from, &to);
    return from != to;
}

bool wxTextCtrl::CanCopy() const
{
    // Can copy if there's a selection
    return HasSelection();
}

bool wxTextCtrl::CanCut() const
{
    return CanCopy() && IsEditable();
}

bool wxTextCtrl::CanPaste() const
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

// ----------------------------------------------------------------------------
// Accessors
// ----------------------------------------------------------------------------

void wxTextCtrl::SetEditable(bool editable)
{
    ::SendMessage(GetBuddyHwnd(), EM_SETREADONLY, (WPARAM)!editable, (LPARAM)0L);
}

void wxTextCtrl::SetInsertionPoint(long pos)
{
    DoSetSelection(pos, pos);
}

void wxTextCtrl::SetInsertionPointEnd()
{
    if ( GetInsertionPoint() != GetLastPosition() )
        SetInsertionPoint(GetLastPosition());
}

long wxTextCtrl::GetInsertionPoint() const
{
    DWORD Pos = (DWORD)::SendMessage(GetBuddyHwnd(), EM_GETSEL, 0, 0L);
    return Pos & 0xFFFF;
}

wxTextPos wxTextCtrl::GetLastPosition() const
{
    int numLines = GetNumberOfLines();
    long posStartLastLine = XYToPosition(0, numLines - 1);

    long lenLastLine = GetLengthOfLineContainingPos(posStartLastLine);

    return posStartLastLine + lenLastLine;
}

void wxTextCtrl::GetSelection(long* from, long* to) const
{
    DWORD dwStart, dwEnd;
    ::SendMessage(GetBuddyHwnd(), EM_GETSEL, (WPARAM)&dwStart, (LPARAM)&dwEnd);

    *from = dwStart;
    *to = dwEnd;
}

bool wxTextCtrl::IsEditable() const
{
    if ( !GetBuddyHwnd() )
        return true;

    long style = ::GetWindowLong(GetBuddyHwnd(), GWL_STYLE);

    return (style & ES_READONLY) == 0;
}

// ----------------------------------------------------------------------------
// selection
// ----------------------------------------------------------------------------

void wxTextCtrl::SetSelection(long from, long to)
{
    // if from and to are both -1, it means (in wxWidgets) that all text should
    // be selected - translate into Windows convention
    if ( (from == -1) && (to == -1) )
    {
        from = 0;
        to = -1;
    }

    DoSetSelection(from, to);
}

void wxTextCtrl::DoSetSelection(long from, long to, bool scrollCaret)
{
    ::SendMessage(GetBuddyHwnd(), EM_SETSEL, (WPARAM)from, (LPARAM)to);

    if ( scrollCaret )
    {
        ::SendMessage(GetBuddyHwnd(), EM_SCROLLCARET, (WPARAM)0, (LPARAM)0);
    }
}

// ----------------------------------------------------------------------------
// Working with files
// ----------------------------------------------------------------------------

bool wxTextCtrl::LoadFile(const wxString& file)
{
    if ( wxTextCtrlBase::LoadFile(file) )
    {
        // update the size limit if needed
        AdjustSpaceLimit();

        return true;
    }

    return false;
}

// ----------------------------------------------------------------------------
// Editing
// ----------------------------------------------------------------------------

void wxTextCtrl::Replace(long from, long to, const wxString& value)
{
    // Set selection and remove it
    DoSetSelection(from, to, false);

    DoWriteText(value, SetValue_SelectionOnly);
}

void wxTextCtrl::Remove(long from, long to)
{
    Replace(from, to, wxEmptyString);
}

bool wxTextCtrl::IsModified() const
{
    return ::SendMessage(GetBuddyHwnd(), EM_GETMODIFY, 0, 0) != 0;
}

void wxTextCtrl::MarkDirty()
{
    ::SendMessage(GetBuddyHwnd(), EM_SETMODIFY, TRUE, 0L);
}

void wxTextCtrl::DiscardEdits()
{
    ::SendMessage(GetBuddyHwnd(), EM_SETMODIFY, FALSE, 0L);
}

int wxTextCtrl::GetNumberOfLines() const
{
    return (int)::SendMessage(GetBuddyHwnd(), EM_GETLINECOUNT, 0, 0L);
}

// ----------------------------------------------------------------------------
// Positions <-> coords
// ----------------------------------------------------------------------------

long wxTextCtrl::XYToPosition(long x, long y) const
{
    // This gets the char index for the _beginning_ of this line
    long charIndex = ::SendMessage(GetBuddyHwnd(), EM_LINEINDEX, (WPARAM)y, (LPARAM)0);

    return charIndex + x;
}

bool wxTextCtrl::PositionToXY(long pos, long *x, long *y) const
{
    // This gets the line number containing the character
    long lineNo = ::SendMessage(GetBuddyHwnd(), EM_LINEFROMCHAR, (WPARAM)pos, 0);

    if ( lineNo == -1 )
    {
        // no such line
        return false;
    }

    // This gets the char index for the _beginning_ of this line
    long charIndex = ::SendMessage(GetBuddyHwnd(), EM_LINEINDEX, (WPARAM)lineNo, (LPARAM)0);
    if ( charIndex == -1 )
    {
        return false;
    }

    // The X position must therefore be the different between pos and charIndex
    if ( x )
        *x = pos - charIndex;
    if ( y )
        *y = lineNo;

    return true;
}

wxTextCtrlHitTestResult
wxTextCtrl::HitTest(const wxPoint& pt, long *posOut) const
{
    // first get the position from Windows
    // for the plain ones, we are limited to 16 bit positions which are
    // combined in a single 32 bit value
    LPARAM lParam = MAKELPARAM(pt.x, pt.y);

    LRESULT pos = ::SendMessage(GetBuddyHwnd(), EM_CHARFROMPOS, 0, lParam);

    if ( pos == -1 )
    {
        // this seems to indicate an error...
        return wxTE_HT_UNKNOWN;
    }

    // for plain EDIT controls the higher word contains something else
    pos = LOWORD(pos);


    // next determine where it is relatively to our point: EM_CHARFROMPOS
    // always returns the closest character but we need to be more precise, so
    // double check that we really are where it pretends
    POINTL ptReal;

    LRESULT lRc = ::SendMessage(GetBuddyHwnd(), EM_POSFROMCHAR, pos, 0);

    if ( lRc == -1 )
    {
        // this is apparently returned when pos corresponds to the last
        // position
        ptReal.x =
        ptReal.y = 0;
    }
    else
    {
        ptReal.x = LOWORD(lRc);
        ptReal.y = HIWORD(lRc);
    }

    wxTextCtrlHitTestResult rc;

    if ( pt.y > ptReal.y + GetCharHeight() )
        rc = wxTE_HT_BELOW;
    else if ( pt.x > ptReal.x + GetCharWidth() )
        rc = wxTE_HT_BEYOND;
    else
        rc = wxTE_HT_ON_TEXT;

    if ( posOut )
        *posOut = pos;

    return rc;
}

void wxTextCtrl::ShowPosition(long pos)
{
    int currentLineLineNo = (int)::SendMessage(GetBuddyHwnd(), EM_GETFIRSTVISIBLELINE, 0, 0L);

    int specifiedLineLineNo = (int)::SendMessage(GetBuddyHwnd(), EM_LINEFROMCHAR, (WPARAM)pos, 0L);

    int linesToScroll = specifiedLineLineNo - currentLineLineNo;

    if (linesToScroll != 0)
        (void)::SendMessage(GetBuddyHwnd(), EM_LINESCROLL, 0, (LPARAM)linesToScroll);
}

long wxTextCtrl::GetLengthOfLineContainingPos(long pos) const
{
    return ::SendMessage(GetBuddyHwnd(), EM_LINELENGTH, (WPARAM)pos, 0L);
}

int wxTextCtrl::GetLineLength(long lineNo) const
{
    long pos = XYToPosition(0, lineNo);

    return GetLengthOfLineContainingPos(pos);
}

wxString wxTextCtrl::GetLineText(long lineNo) const
{
    size_t len = (size_t)GetLineLength(lineNo) + 1;

    // there must be at least enough place for the length WORD in the
    // buffer
    len += sizeof(WORD);

    wxString str;
    {
        wxStringBufferLength tmp(str, len);
        wxChar *buf = tmp;

        *(WORD *)buf = (WORD)len;
        len = (size_t)::SendMessage(GetBuddyHwnd(), EM_GETLINE, lineNo, (LPARAM)buf);

        // remove the '\n' at the end, if any (this is how this function is
        // supposed to work according to the docs)
        if ( buf[len - 1] == _T('\n') )
        {
            len--;
        }

        buf[len] = 0;
        tmp.SetLength(len);
    }

    return str;
}

void wxTextCtrl::SetMaxLength(unsigned long len)
{
    ::SendMessage(GetBuddyHwnd(), EM_LIMITTEXT, len, 0);
}

// ----------------------------------------------------------------------------
// Undo/redo
// ----------------------------------------------------------------------------

void wxTextCtrl::Undo()
{
    if (CanUndo())
    {
        ::SendMessage(GetBuddyHwnd(), EM_UNDO, 0, 0);
    }
}

void wxTextCtrl::Redo()
{
    if (CanRedo())
    {
        ::SendMessage(GetBuddyHwnd(), EM_UNDO, 0, 0);
    }
}

bool wxTextCtrl::CanUndo() const
{
    return ::SendMessage(GetBuddyHwnd(), EM_CANUNDO, 0, 0) != 0;
}

bool wxTextCtrl::CanRedo() const
{
    return ::SendMessage(GetBuddyHwnd(), EM_CANUNDO, 0, 0) != 0;
}

// ----------------------------------------------------------------------------
// caret handling
// ----------------------------------------------------------------------------

// ----------------------------------------------------------------------------
// implemenation details
// ----------------------------------------------------------------------------

void wxTextCtrl::Command(wxCommandEvent & event)
{
    SetValue(event.GetString());
    ProcessCommand (event);
}

// ----------------------------------------------------------------------------
// kbd input processing
// ----------------------------------------------------------------------------

void wxTextCtrl::OnChar(wxKeyEvent& event)
{
    switch ( event.GetKeyCode() )
    {
        case WXK_RETURN:
            if ( !HasFlag(wxTE_MULTILINE) )
            {
                wxCommandEvent event(wxEVT_COMMAND_TEXT_ENTER, m_windowId);
                InitCommandEvent(event);
                event.SetString(GetValue());
                if ( GetEventHandler()->ProcessEvent(event) )
                    return;
            }
            //else: multiline controls need Enter for themselves

            break;

        case WXK_TAB:
            // ok, so this is getting absolutely ridiculous but I don't see
            // any other way to fix this bug: when a multiline text control is
            // inside a wxFrame, we need to generate the navigation event as
            // otherwise nothing happens at all, but when the same control is
            // created inside a dialog, IsDialogMessage() *does* switch focus
            // all by itself and so if we do it here as well, it is advanced
            // twice and goes to the next control... to prevent this from
            // happening we're doing this ugly check, the logic being that if
            // we don't have focus then it had been already changed to the next
            // control
            //
            // the right thing to do would, of course, be to understand what
            // the hell is IsDialogMessage() doing but this is beyond my feeble
            // forces at the moment unfortunately
            if ( !(m_windowStyle & wxTE_PROCESS_TAB))
            {
                if ( FindFocus() == this )
                {
                    int flags = 0;
                    if (!event.ShiftDown())
                        flags |= wxNavigationKeyEvent::IsForward ;
                    if (event.ControlDown())
                        flags |= wxNavigationKeyEvent::WinChange ;
                    if (Navigate(flags))
                        return;
                }
            }
            else
            {
                // Insert tab since calling the default Windows handler
                // doesn't seem to do it
                WriteText(wxT("\t"));
            }
            break;
    }

    // no, we didn't process it
    event.Skip();
}

WXLRESULT wxTextCtrl::MSWWindowProc(WXUINT nMsg, WXWPARAM wParam, WXLPARAM lParam)
{
    WXLRESULT lRc = wxTextCtrlBase::MSWWindowProc(nMsg, wParam, lParam);

    if ( nMsg == WM_GETDLGCODE )
    {
        // we always want the chars and the arrows: the arrows for navigation
        // and the chars because we want Ctrl-C to work even in a read only
        // control
        long lDlgCode = DLGC_WANTCHARS | DLGC_WANTARROWS;

        if ( IsEditable() )
        {
            // we may have several different cases:
            // 1. normal case: both TAB and ENTER are used for dlg navigation
            // 2. ctrl which wants TAB for itself: ENTER is used to pass to the
            //    next control in the dialog
            // 3. ctrl which wants ENTER for itself: TAB is used for dialog
            //    navigation
            // 4. ctrl which wants both TAB and ENTER: Ctrl-ENTER is used to go
            //    to the next control

            // the multiline edit control should always get <Return> for itself
            if ( HasFlag(wxTE_PROCESS_ENTER) || HasFlag(wxTE_MULTILINE) )
                lDlgCode |= DLGC_WANTMESSAGE;

            if ( HasFlag(wxTE_PROCESS_TAB) )
                lDlgCode |= DLGC_WANTTAB;

            lRc |= lDlgCode;
        }
        else // !editable
        {
            // NB: use "=", not "|=" as the base class version returns the
            //     same flags is this state as usual (i.e. including
            //     DLGC_WANTMESSAGE). This is strange (how does it work in the
            //     native Win32 apps?) but for now live with it.
            lRc = lDlgCode;
        }
    }

    return lRc;
}

// ----------------------------------------------------------------------------
// text control event processing
// ----------------------------------------------------------------------------

bool wxTextCtrl::SendUpdateEvent()
{
    // is event reporting suspended?
    if ( m_suppressNextUpdate )
    {
        // do process the next one
        m_suppressNextUpdate = false;

        return false;
    }

    wxCommandEvent event(wxEVT_COMMAND_TEXT_UPDATED, GetId());
    InitCommandEvent(event);
    event.SetString(GetValue());

    return ProcessCommand(event);
}

bool wxTextCtrl::MSWCommand(WXUINT param, WXWORD WXUNUSED(id))
{
    switch ( param )
    {
        case EN_SETFOCUS:
        case EN_KILLFOCUS:
            {
                wxFocusEvent event(param == EN_KILLFOCUS ? wxEVT_KILL_FOCUS
                                                         : wxEVT_SET_FOCUS,
                                   m_windowId);
                event.SetEventObject(this);
                GetEventHandler()->ProcessEvent(event);
            }
            break;

        case EN_CHANGE:
            SendUpdateEvent();
            break;

        case EN_MAXTEXT:
            // the text size limit has been hit -- try to increase it
            if ( !AdjustSpaceLimit() )
            {
                wxCommandEvent event(wxEVT_COMMAND_TEXT_MAXLEN, m_windowId);
                InitCommandEvent(event);
                event.SetString(GetValue());
                ProcessCommand(event);
            }
            break;

            // the other edit notification messages are not processed
        default:
            return false;
    }

    // processed
    return true;
}

bool wxTextCtrl::AdjustSpaceLimit()
{
    unsigned int limit = ::SendMessage(GetBuddyHwnd(), EM_GETLIMITTEXT, 0, 0);

    // HACK: we try to automatically extend the limit for the amount of text
    //       to allow (interactively) entering more than 64Kb of text under
    //       Win9x but we shouldn't reset the text limit which was previously
    //       set explicitly with SetMaxLength()
    //
    //       we could solve this by storing the limit we set in wxTextCtrl but
    //       to save space we prefer to simply test here the actual limit
    //       value: we consider that SetMaxLength() can only be called for
    //       values < 32Kb
    if ( limit < 0x8000 )
    {
        // we've got more text than limit set by SetMaxLength()
        return false;
    }

    unsigned int len = ::GetWindowTextLength(GetBuddyHwnd());
    if ( len >= limit )
    {
        limit = len + 0x8000;    // 32Kb

        if ( limit > 0xffff )
        {
            // this will set it to a platform-dependent maximum (much more
            // than 64Kb under NT)
            limit = 0;
        }

        ::SendMessage(GetBuddyHwnd(), EM_LIMITTEXT, limit, 0L);
    }

    // we changed the limit
    return true;
}

bool wxTextCtrl::AcceptsFocus() const
{
    // we don't want focus if we can't be edited unless we're a multiline
    // control because then it might be still nice to get focus from keyboard
    // to be able to scroll it without mouse
    return (IsEditable() || IsMultiLine()) && wxControl::AcceptsFocus();
}

void wxTextCtrl::DoMoveWindow(int x, int y, int width, int height)
{
    int widthBtn = GetBestSpinnerSize(IsVertical(GetWindowStyle())).x / 2;
    int widthText = width - widthBtn - MARGIN_BETWEEN;
    if ( widthText <= 0 )
    {
        wxLogDebug(_T("not enough space for wxSpinCtrl!"));
    }

    if ( !::MoveWindow(GetBuddyHwnd(), x, y, widthText, height, TRUE) )
    {
        wxLogLastError(wxT("MoveWindow(buddy)"));
    }

    x += widthText + MARGIN_BETWEEN;
    if ( !::MoveWindow(GetHwnd(), x, y, widthBtn, height, TRUE) )
    {
        wxLogLastError(wxT("MoveWindow"));
    }
}

wxSize wxTextCtrl::DoGetBestSize() const
{
    int cx, cy;
    wxGetCharSize(GetBuddyHwnd(), &cx, &cy, GetFont());

    int wText = DEFAULT_ITEM_WIDTH;

    int hText = cy;
    if ( m_windowStyle & wxTE_MULTILINE )
    {
        hText *= wxMax(GetNumberOfLines(), 5);
    }
    //else: for single line control everything is ok

    // we have to add the adjustments for the control height only once, not
    // once per line, so do it after multiplication above
    hText += EDIT_HEIGHT_FROM_CHAR_HEIGHT(cy) - cy;

    return wxSize(wText, hText);
}

// ----------------------------------------------------------------------------
// standard handlers for standard edit menu events
// ----------------------------------------------------------------------------

void wxTextCtrl::OnCut(wxCommandEvent& WXUNUSED(event))
{
    Cut();
}

void wxTextCtrl::OnCopy(wxCommandEvent& WXUNUSED(event))
{
    Copy();
}

void wxTextCtrl::OnPaste(wxCommandEvent& WXUNUSED(event))
{
    Paste();
}

void wxTextCtrl::OnUndo(wxCommandEvent& WXUNUSED(event))
{
    Undo();
}

void wxTextCtrl::OnRedo(wxCommandEvent& WXUNUSED(event))
{
    Redo();
}

void wxTextCtrl::OnDelete(wxCommandEvent& WXUNUSED(event))
{
    long from, to;
    GetSelection(& from, & to);
    if (from != -1 && to != -1)
        Remove(from, to);
}

void wxTextCtrl::OnSelectAll(wxCommandEvent& WXUNUSED(event))
{
    SetSelection(-1, -1);
}

void wxTextCtrl::OnUpdateCut(wxUpdateUIEvent& event)
{
    event.Enable( CanCut() );
}

void wxTextCtrl::OnUpdateCopy(wxUpdateUIEvent& event)
{
    event.Enable( CanCopy() );
}

void wxTextCtrl::OnUpdatePaste(wxUpdateUIEvent& event)
{
    event.Enable( CanPaste() );
}

void wxTextCtrl::OnUpdateUndo(wxUpdateUIEvent& event)
{
    event.Enable( CanUndo() );
}

void wxTextCtrl::OnUpdateRedo(wxUpdateUIEvent& event)
{
    event.Enable( CanRedo() );
}

void wxTextCtrl::OnUpdateDelete(wxUpdateUIEvent& event)
{
    long from, to;
    GetSelection(& from, & to);
    event.Enable(from != -1 && to != -1 && from != to && IsEditable()) ;
}

void wxTextCtrl::OnUpdateSelectAll(wxUpdateUIEvent& event)
{
    event.Enable(GetLastPosition() > 0);
}

void wxTextCtrl::OnSetFocus(wxFocusEvent& WXUNUSED(event))
{
    // be sure the caret remains invisible if the user had hidden it
    if ( !m_isNativeCaretShown )
    {
        ::HideCaret(GetBuddyHwnd());
    }
}

#endif // wxUSE_TEXTCTRL && __SMARTPHONE__ && __WXWINCE__
