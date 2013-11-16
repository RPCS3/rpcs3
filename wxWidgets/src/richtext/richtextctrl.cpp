/////////////////////////////////////////////////////////////////////////////
// Name:        src/richtext/richeditctrl.cpp
// Purpose:     A rich edit control
// Author:      Julian Smart
// Modified by:
// Created:     2005-09-30
// RCS-ID:      $Id: richtextctrl.cpp 67025 2011-02-25 17:28:13Z JS $
// Copyright:   (c) Julian Smart
// Licence:     wxWindows licence
/////////////////////////////////////////////////////////////////////////////

// For compilers that support precompilation, includes "wx.h".
#include "wx/wxprec.h"

#ifdef __BORLANDC__
    #pragma hdrstop
#endif

#if wxUSE_RICHTEXT

#include "wx/richtext/richtextctrl.h"
#include "wx/richtext/richtextstyles.h"

#ifndef WX_PRECOMP
    #include "wx/wx.h"
    #include "wx/settings.h"
#endif

#include "wx/timer.h"
#include "wx/textfile.h"
#include "wx/ffile.h"
#include "wx/filename.h"
#include "wx/dcbuffer.h"
#include "wx/arrimpl.cpp"
#include "wx/fontenum.h"
#include "wx/accel.h"

// DLL options compatibility check:
#include "wx/app.h"

// Refresh the area affected by a selection change
bool wxRichTextCtrlRefreshForSelectionChange(wxRichTextCtrl& ctrl, const wxRichTextRange& oldSelection, const wxRichTextRange& newSelection);

WX_CHECK_BUILD_OPTIONS("wxRichTextCtrl")

DEFINE_EVENT_TYPE(wxEVT_COMMAND_RICHTEXT_LEFT_CLICK)
DEFINE_EVENT_TYPE(wxEVT_COMMAND_RICHTEXT_MIDDLE_CLICK)
DEFINE_EVENT_TYPE(wxEVT_COMMAND_RICHTEXT_RIGHT_CLICK)
DEFINE_EVENT_TYPE(wxEVT_COMMAND_RICHTEXT_LEFT_DCLICK)
DEFINE_EVENT_TYPE(wxEVT_COMMAND_RICHTEXT_RETURN)
DEFINE_EVENT_TYPE(wxEVT_COMMAND_RICHTEXT_CHARACTER)
DEFINE_EVENT_TYPE(wxEVT_COMMAND_RICHTEXT_DELETE)

DEFINE_EVENT_TYPE(wxEVT_COMMAND_RICHTEXT_STYLESHEET_REPLACING)
DEFINE_EVENT_TYPE(wxEVT_COMMAND_RICHTEXT_STYLESHEET_REPLACED)
DEFINE_EVENT_TYPE(wxEVT_COMMAND_RICHTEXT_STYLESHEET_CHANGING)
DEFINE_EVENT_TYPE(wxEVT_COMMAND_RICHTEXT_STYLESHEET_CHANGED)

DEFINE_EVENT_TYPE(wxEVT_COMMAND_RICHTEXT_CONTENT_INSERTED)
DEFINE_EVENT_TYPE(wxEVT_COMMAND_RICHTEXT_CONTENT_DELETED)
DEFINE_EVENT_TYPE(wxEVT_COMMAND_RICHTEXT_STYLE_CHANGED)
DEFINE_EVENT_TYPE(wxEVT_COMMAND_RICHTEXT_SELECTION_CHANGED)
DEFINE_EVENT_TYPE(wxEVT_COMMAND_RICHTEXT_BUFFER_RESET)

#if wxRICHTEXT_USE_OWN_CARET

class wxRichTextCaret;
class wxRichTextCaretTimer: public wxTimer
{
  public:
    wxRichTextCaretTimer(wxRichTextCaret* caret)
    {
        m_caret = caret;
    }
    virtual void Notify();
    wxRichTextCaret* m_caret;
};

/*!
 * wxRichTextCaret
 *
 * This implements a non-flashing cursor in case there
 * are platform-specific problems with the generic caret.
 * wxRICHTEXT_USE_OWN_CARET is set in richtextbuffer.h.
 */

class wxRichTextCaret: public wxCaret
{
public:
    // ctors
    // -----
        // default - use Create()
    wxRichTextCaret(): m_timer(this)  { Init(); }
        // creates a block caret associated with the given window
    wxRichTextCaret(wxRichTextCtrl *window, int width, int height)
        : wxCaret(window, width, height), m_timer(this) { Init(); m_richTextCtrl = window; }
    wxRichTextCaret(wxRichTextCtrl *window, const wxSize& size)
        : wxCaret(window, size), m_timer(this) { Init(); m_richTextCtrl = window; }

    virtual ~wxRichTextCaret();

    // implementation
    // --------------

    // called by wxWindow (not using the event tables)
    virtual void OnSetFocus();
    virtual void OnKillFocus();

    // draw the caret on the given DC
    void DoDraw(wxDC *dc);

    // get the visible count
    int GetVisibleCount() const { return m_countVisible; }

    // delay repositioning
    bool GetNeedsUpdate() const { return m_needsUpdate; }
    void SetNeedsUpdate(bool needsUpdate = true ) { m_needsUpdate = needsUpdate; }

    void Notify();

protected:
    virtual void DoShow();
    virtual void DoHide();
    virtual void DoMove();
    virtual void DoSize();

    // refresh the caret
    void Refresh();

private:
    void Init();

    int           m_xOld,
                  m_yOld;
    bool          m_hasFocus;       // true => our window has focus
    bool          m_needsUpdate;    // must be repositioned
    bool          m_flashOn;
    wxRichTextCaretTimer m_timer;
    wxRichTextCtrl* m_richTextCtrl;
};
#endif

IMPLEMENT_CLASS( wxRichTextCtrl, wxTextCtrlBase )

IMPLEMENT_CLASS( wxRichTextEvent, wxNotifyEvent )

BEGIN_EVENT_TABLE( wxRichTextCtrl, wxTextCtrlBase )
    EVT_PAINT(wxRichTextCtrl::OnPaint)
    EVT_ERASE_BACKGROUND(wxRichTextCtrl::OnEraseBackground)
    EVT_IDLE(wxRichTextCtrl::OnIdle)
    EVT_SCROLLWIN(wxRichTextCtrl::OnScroll)
    EVT_LEFT_DOWN(wxRichTextCtrl::OnLeftClick)
    EVT_MOTION(wxRichTextCtrl::OnMoveMouse)
    EVT_LEFT_UP(wxRichTextCtrl::OnLeftUp)
    EVT_RIGHT_DOWN(wxRichTextCtrl::OnRightClick)
    EVT_MIDDLE_DOWN(wxRichTextCtrl::OnMiddleClick)
    EVT_LEFT_DCLICK(wxRichTextCtrl::OnLeftDClick)
    EVT_CHAR(wxRichTextCtrl::OnChar)
    EVT_KEY_DOWN(wxRichTextCtrl::OnChar)
    EVT_SIZE(wxRichTextCtrl::OnSize)
    EVT_SET_FOCUS(wxRichTextCtrl::OnSetFocus)
    EVT_KILL_FOCUS(wxRichTextCtrl::OnKillFocus)
    EVT_MOUSE_CAPTURE_LOST(wxRichTextCtrl::OnCaptureLost)
    EVT_CONTEXT_MENU(wxRichTextCtrl::OnContextMenu)

    EVT_MENU(wxID_UNDO, wxRichTextCtrl::OnUndo)
    EVT_UPDATE_UI(wxID_UNDO, wxRichTextCtrl::OnUpdateUndo)

    EVT_MENU(wxID_REDO, wxRichTextCtrl::OnRedo)
    EVT_UPDATE_UI(wxID_REDO, wxRichTextCtrl::OnUpdateRedo)

    EVT_MENU(wxID_COPY, wxRichTextCtrl::OnCopy)
    EVT_UPDATE_UI(wxID_COPY, wxRichTextCtrl::OnUpdateCopy)

    EVT_MENU(wxID_PASTE, wxRichTextCtrl::OnPaste)
    EVT_UPDATE_UI(wxID_PASTE, wxRichTextCtrl::OnUpdatePaste)

    EVT_MENU(wxID_CUT, wxRichTextCtrl::OnCut)
    EVT_UPDATE_UI(wxID_CUT, wxRichTextCtrl::OnUpdateCut)

    EVT_MENU(wxID_CLEAR, wxRichTextCtrl::OnClear)
    EVT_UPDATE_UI(wxID_CLEAR, wxRichTextCtrl::OnUpdateClear)

    EVT_MENU(wxID_SELECTALL, wxRichTextCtrl::OnSelectAll)
    EVT_UPDATE_UI(wxID_SELECTALL, wxRichTextCtrl::OnUpdateSelectAll)
END_EVENT_TABLE()

/*!
 * wxRichTextCtrl
 */

wxArrayString wxRichTextCtrl::sm_availableFontNames;

wxRichTextCtrl::wxRichTextCtrl()
              : wxScrollHelper(this)
{
    Init();
}

wxRichTextCtrl::wxRichTextCtrl(wxWindow* parent,
                               wxWindowID id,
                               const wxString& value,
                               const wxPoint& pos,
                               const wxSize& size,
                               long style,
                               const wxValidator& validator,
                               const wxString& name)
              : wxScrollHelper(this)
{
    Init();
    Create(parent, id, value, pos, size, style, validator, name);
}

/// Creation
bool wxRichTextCtrl::Create( wxWindow* parent, wxWindowID id, const wxString& value, const wxPoint& pos, const wxSize& size, long style,
                             const wxValidator& validator, const wxString& name)
{
    if ((style & wxBORDER_MASK) == wxBORDER_DEFAULT)
#ifdef __WXMSW__
        style |= GetThemedBorderStyle();
#else
        style |= wxBORDER_SUNKEN;
#endif

    if (!wxTextCtrlBase::Create(parent, id, pos, size,
                                style|wxFULL_REPAINT_ON_RESIZE,
                                validator, name))
        return false;

    if (!GetFont().Ok())
    {
        SetFont(wxSystemSettings::GetFont(wxSYS_DEFAULT_GUI_FONT));
    }

    // No physical scrolling, so we can preserve margins
    EnableScrolling(false, false);

    if (style & wxTE_READONLY)
        SetEditable(false);

    // The base attributes must all have default values
    wxTextAttrEx attributes;
    attributes.SetFont(GetFont());
    attributes.SetTextColour(wxSystemSettings::GetColour(wxSYS_COLOUR_WINDOWTEXT));
    attributes.SetAlignment(wxTEXT_ALIGNMENT_LEFT);
    attributes.SetLineSpacing(10);
    attributes.SetParagraphSpacingAfter(10);
    attributes.SetParagraphSpacingBefore(0);

    SetBasicStyle(attributes);

    // The default attributes will be merged with base attributes, so
    // can be empty to begin with
    wxTextAttrEx defaultAttributes;
    SetDefaultStyle(defaultAttributes);

    SetBackgroundColour(wxSystemSettings::GetColour(wxSYS_COLOUR_WINDOW));
    SetBackgroundStyle(wxBG_STYLE_CUSTOM);

    GetBuffer().Reset();
    GetBuffer().SetRichTextCtrl(this);

#if wxRICHTEXT_USE_OWN_CARET
    SetCaret(new wxRichTextCaret(this, wxRICHTEXT_DEFAULT_CARET_WIDTH, 16));
#else
    SetCaret(new wxCaret(this, wxRICHTEXT_DEFAULT_CARET_WIDTH, 16));
#endif

    // Tell the sizers to use the given or best size
    SetInitialSize(size);

#if wxRICHTEXT_BUFFERED_PAINTING
    // Create a buffer
    RecreateBuffer(size);
#endif

    m_textCursor = wxCursor(wxCURSOR_IBEAM);
    m_urlCursor = wxCursor(wxCURSOR_HAND);

    SetCursor(m_textCursor);

    if (!value.IsEmpty())
        SetValue(value);

    GetBuffer().AddEventHandler(this);

    // Accelerators
    wxAcceleratorEntry entries[6];

    entries[0].Set(wxACCEL_CMD,   (int) 'C',       wxID_COPY);
    entries[1].Set(wxACCEL_CMD,   (int) 'X',       wxID_CUT);
    entries[2].Set(wxACCEL_CMD,   (int) 'V',       wxID_PASTE);
    entries[3].Set(wxACCEL_CMD,   (int) 'A',       wxID_SELECTALL);
    entries[4].Set(wxACCEL_CMD,   (int) 'Z',       wxID_UNDO);
    entries[5].Set(wxACCEL_CMD,   (int) 'Y',       wxID_REDO);

    wxAcceleratorTable accel(6, entries);
    SetAcceleratorTable(accel);

    m_contextMenu = new wxMenu;
    m_contextMenu->Append(wxID_UNDO, _("&Undo"));
    m_contextMenu->Append(wxID_REDO, _("&Redo"));
    m_contextMenu->AppendSeparator();
    m_contextMenu->Append(wxID_CUT, _("Cu&t"));
    m_contextMenu->Append(wxID_COPY, _("&Copy"));
    m_contextMenu->Append(wxID_PASTE, _("&Paste"));
    m_contextMenu->Append(wxID_CLEAR, _("&Delete"));
    m_contextMenu->AppendSeparator();
    m_contextMenu->Append(wxID_SELECTALL, _("Select &All"));

    return true;
}

wxRichTextCtrl::~wxRichTextCtrl()
{
    GetBuffer().RemoveEventHandler(this);

    delete m_contextMenu;
}

/// Member initialisation
void wxRichTextCtrl::Init()
{
    m_freezeCount = 0;
    m_contextMenu = NULL;
    m_caret = NULL;
    m_caretPosition = -1;
    m_selectionRange.SetRange(-2, -2);
    m_selectionAnchor = -2;
    m_editable = true;
    m_caretAtLineStart = false;
    m_dragging = false;
    m_fullLayoutRequired = false;
    m_fullLayoutTime = 0;
    m_fullLayoutSavedPosition = 0;
    m_delayedLayoutThreshold = wxRICHTEXT_DEFAULT_DELAYED_LAYOUT_THRESHOLD;
    m_caretPositionForDefaultStyle = -2;
}

/// Call Freeze to prevent refresh
void wxRichTextCtrl::Freeze()
{
    m_freezeCount ++;
}

/// Call Thaw to refresh
void wxRichTextCtrl::Thaw()
{
    m_freezeCount --;

    if (m_freezeCount == 0)
    {
        if (GetBuffer().GetDirty())
            LayoutContent();
        else
            SetupScrollbars();

        Refresh(false);
    }
}

/// Clear all text
void wxRichTextCtrl::Clear()
{
    m_buffer.ResetAndClearCommands();
    m_buffer.SetDirty(true);
    m_caretPosition = -1;
    m_caretPositionForDefaultStyle = -2;
    m_caretAtLineStart = false;
    m_selectionRange.SetRange(-2, -2);

    Scroll(0,0);

    if (m_freezeCount == 0)
    {
        LayoutContent();
        Refresh(false);
    }
    SendTextUpdatedEvent();
}

/// Painting
void wxRichTextCtrl::OnPaint(wxPaintEvent& WXUNUSED(event))
{
#if !wxRICHTEXT_USE_OWN_CARET
    if (GetCaret() && !IsFrozen())
        GetCaret()->Hide();
#endif

    {
#if wxRICHTEXT_BUFFERED_PAINTING
        wxBufferedPaintDC dc(this, m_bufferBitmap);
#else
        wxPaintDC dc(this);
#endif
        PrepareDC(dc);

        if (IsFrozen())
        {
            return;
        }

        dc.SetFont(GetFont());

        // Paint the background
        PaintBackground(dc);

        // wxRect drawingArea(GetLogicalPoint(wxPoint(0, 0)), GetClientSize());

        wxRect drawingArea(GetUpdateRegion().GetBox());
        drawingArea.SetPosition(GetLogicalPoint(drawingArea.GetPosition()));

        wxRect availableSpace(GetClientSize());
        if (GetBuffer().GetDirty())
        {
            GetBuffer().Layout(dc, availableSpace, wxRICHTEXT_FIXED_WIDTH|wxRICHTEXT_VARIABLE_HEIGHT);
            GetBuffer().SetDirty(false);
            SetupScrollbars();
        }

        wxRect clipRect(availableSpace);
        clipRect.x += GetBuffer().GetLeftMargin();
        clipRect.y += GetBuffer().GetTopMargin();
        clipRect.width -= (GetBuffer().GetLeftMargin() + GetBuffer().GetRightMargin());
        clipRect.height -= (GetBuffer().GetTopMargin() + GetBuffer().GetBottomMargin());
        clipRect.SetPosition(GetLogicalPoint(clipRect.GetPosition()));
        dc.SetClippingRegion(clipRect);

        GetBuffer().Draw(dc, GetBuffer().GetRange(), GetInternalSelectionRange(), drawingArea, 0 /* descent */, 0 /* flags */);

        dc.DestroyClippingRegion();

#if wxRICHTEXT_USE_OWN_CARET
        if (GetCaret()->IsVisible())
        {
            ((wxRichTextCaret*) GetCaret())->DoDraw(& dc);
        }
#endif

    }

#if !wxRICHTEXT_USE_OWN_CARET
    if (GetCaret())
        GetCaret()->Show();
    PositionCaret();
#endif
}

// Empty implementation, to prevent flicker
void wxRichTextCtrl::OnEraseBackground(wxEraseEvent& WXUNUSED(event))
{
}

void wxRichTextCtrl::OnSetFocus(wxFocusEvent& WXUNUSED(event))
{
    if (GetCaret())
    {
#if !wxRICHTEXT_USE_OWN_CARET
        PositionCaret();
#endif
        GetCaret()->Show();
    }

#if defined(__WXGTK__) && !wxRICHTEXT_USE_OWN_CARET
    // Work around dropouts when control is focused
    if (!IsFrozen())
    {
        Refresh(false);
    }
#endif
}

void wxRichTextCtrl::OnKillFocus(wxFocusEvent& WXUNUSED(event))
{
    if (GetCaret())
        GetCaret()->Hide();

#if defined(__WXGTK__) && !wxRICHTEXT_USE_OWN_CARET
    // Work around dropouts when control is focused
    if (!IsFrozen())
    {
        Refresh(false);
    }
#endif
}

void wxRichTextCtrl::OnCaptureLost(wxMouseCaptureLostEvent& WXUNUSED(event))
{
    m_dragging = false;
}

/// Left-click
void wxRichTextCtrl::OnLeftClick(wxMouseEvent& event)
{
    SetFocus();

    wxClientDC dc(this);
    PrepareDC(dc);
    dc.SetFont(GetFont());

    long position = 0;
    int hit = GetBuffer().HitTest(dc, event.GetLogicalPosition(dc), position);

    if (hit != wxRICHTEXT_HITTEST_NONE)
    {
        m_dragStart = event.GetLogicalPosition(dc);
        m_dragging = true;
        CaptureMouse();

        bool caretAtLineStart = false;

        if (hit & wxRICHTEXT_HITTEST_BEFORE)
        {
            // If we're at the start of a line (but not first in para)
            // then we should keep the caret showing at the start of the line
            // by showing the m_caretAtLineStart flag.
            wxRichTextParagraph* para = GetBuffer().GetParagraphAtPosition(position);
            wxRichTextLine* line = GetBuffer().GetLineAtPosition(position);

            if (line && para && line->GetAbsoluteRange().GetStart() == position && para->GetRange().GetStart() != position)
                caretAtLineStart = true;
            position --;
        }

        long oldCaretPos = m_caretPosition;

        MoveCaret(position, caretAtLineStart);
        SetDefaultStyleToCursorStyle();

        if (event.ShiftDown())
        {
            if (m_selectionRange.GetStart() == -2)
                ExtendSelection(oldCaretPos, m_caretPosition, wxRICHTEXT_SHIFT_DOWN);
            else
                ExtendSelection(m_caretPosition, m_caretPosition, wxRICHTEXT_SHIFT_DOWN);
        }
        else
            SelectNone();
    }

    event.Skip();
}

/// Left-up
void wxRichTextCtrl::OnLeftUp(wxMouseEvent& event)
{
    if (m_dragging)
    {
        m_dragging = false;
        if (GetCapture() == this)
            ReleaseMouse();

        // See if we clicked on a URL
        wxClientDC dc(this);
        PrepareDC(dc);
        dc.SetFont(GetFont());

        long position = 0;
        wxPoint logicalPt = event.GetLogicalPosition(dc);
        int hit = GetBuffer().HitTest(dc, logicalPt, position);

        if ((hit != wxRICHTEXT_HITTEST_NONE) && !(hit & wxRICHTEXT_HITTEST_OUTSIDE))
        {
            wxTextAttrEx attr;
            if (GetStyle(position, attr))
            {
                if (attr.HasFlag(wxTEXT_ATTR_URL))
                {
                    wxString urlTarget = attr.GetURL();
                    if (!urlTarget.IsEmpty())
                    {
                        wxMouseEvent mouseEvent(event);

                        long startPos = 0, endPos = 0;
                        wxRichTextObject* obj = GetBuffer().GetLeafObjectAtPosition(position);
                        if (obj)
                        {
                            startPos = obj->GetRange().GetStart();
                            endPos = obj->GetRange().GetEnd();
                        }

                        wxTextUrlEvent urlEvent(GetId(), mouseEvent, startPos, endPos);
                        InitCommandEvent(urlEvent);

                        urlEvent.SetString(urlTarget);

                        GetEventHandler()->ProcessEvent(urlEvent);
                    }
                }
            }
        }
    }
}

/// Left-click
void wxRichTextCtrl::OnMoveMouse(wxMouseEvent& event)
{
    wxClientDC dc(this);
    PrepareDC(dc);
    dc.SetFont(GetFont());

    long position = 0;
    wxPoint logicalPt = event.GetLogicalPosition(dc);
    int hit = GetBuffer().HitTest(dc, logicalPt, position);

    // See if we need to change the cursor

    {
        if (hit != wxRICHTEXT_HITTEST_NONE && !(hit & wxRICHTEXT_HITTEST_OUTSIDE))
        {
            wxTextAttrEx attr;
            if (GetStyle(position, attr))
            {
                if (attr.HasFlag(wxTEXT_ATTR_URL))
                {
                    SetCursor(m_urlCursor);
                }
                else if (!attr.HasFlag(wxTEXT_ATTR_URL))
                {
                    SetCursor(m_textCursor);
                }
            }
        }
        else
            SetCursor(m_textCursor);
    }

    if (!event.Dragging())
    {
        event.Skip();
        return;
    }

    if (m_dragging && hit != wxRICHTEXT_HITTEST_NONE)
    {
        // TODO: test closeness

        bool caretAtLineStart = false;

        if (hit & wxRICHTEXT_HITTEST_BEFORE)
        {
            // If we're at the start of a line (but not first in para)
            // then we should keep the caret showing at the start of the line
            // by showing the m_caretAtLineStart flag.
            wxRichTextParagraph* para = GetBuffer().GetParagraphAtPosition(position);
            wxRichTextLine* line = GetBuffer().GetLineAtPosition(position);

            if (line && para && line->GetAbsoluteRange().GetStart() == position && para->GetRange().GetStart() != position)
                caretAtLineStart = true;
            position --;
        }

        if (m_caretPosition != position)
        {
            ExtendSelection(m_caretPosition, position, wxRICHTEXT_SHIFT_DOWN);

            MoveCaret(position, caretAtLineStart);
            SetDefaultStyleToCursorStyle();
        }
    }
}

/// Right-click
void wxRichTextCtrl::OnRightClick(wxMouseEvent& event)
{
    SetFocus();
    event.Skip();
}

/// Left-double-click
void wxRichTextCtrl::OnLeftDClick(wxMouseEvent& event)
{
    SelectWord(GetCaretPosition()+1);
    event.Skip();
}

/// Middle-click
void wxRichTextCtrl::OnMiddleClick(wxMouseEvent& event)
{
    event.Skip();
}

/// Key press
void wxRichTextCtrl::OnChar(wxKeyEvent& event)
{
    int flags = 0;
    if (event.CmdDown())
        flags |= wxRICHTEXT_CTRL_DOWN;
    if (event.ShiftDown())
        flags |= wxRICHTEXT_SHIFT_DOWN;
    if (event.AltDown())
        flags |= wxRICHTEXT_ALT_DOWN;

    if (event.GetEventType() == wxEVT_KEY_DOWN)
    {
        if (event.GetKeyCode() == WXK_LEFT ||
            event.GetKeyCode() == WXK_RIGHT ||
            event.GetKeyCode() == WXK_UP ||
            event.GetKeyCode() == WXK_DOWN ||
            event.GetKeyCode() == WXK_HOME ||
            event.GetKeyCode() == WXK_PAGEUP ||
            event.GetKeyCode() == WXK_PAGEDOWN ||
            event.GetKeyCode() == WXK_END ||

            event.GetKeyCode() == WXK_NUMPAD_LEFT ||
            event.GetKeyCode() == WXK_NUMPAD_RIGHT ||
            event.GetKeyCode() == WXK_NUMPAD_UP ||
            event.GetKeyCode() == WXK_NUMPAD_DOWN ||
            event.GetKeyCode() == WXK_NUMPAD_HOME ||
            event.GetKeyCode() == WXK_NUMPAD_PAGEUP ||
            event.GetKeyCode() == WXK_NUMPAD_PAGEDOWN ||
            event.GetKeyCode() == WXK_NUMPAD_END)
        {
            KeyboardNavigate(event.GetKeyCode(), flags);
            return;
        }

        long keycode = event.GetKeyCode();
        switch ( keycode )
        {
            case WXK_ESCAPE:
            case WXK_START:
            case WXK_LBUTTON:
            case WXK_RBUTTON:
            case WXK_CANCEL:
            case WXK_MBUTTON:
            case WXK_CLEAR:
            case WXK_SHIFT:
            case WXK_ALT:
            case WXK_CONTROL:
            case WXK_MENU:
            case WXK_PAUSE:
            case WXK_CAPITAL:
            case WXK_END:
            case WXK_HOME:
            case WXK_LEFT:
            case WXK_UP:
            case WXK_RIGHT:
            case WXK_DOWN:
            case WXK_SELECT:
            case WXK_PRINT:
            case WXK_EXECUTE:
            case WXK_SNAPSHOT:
            case WXK_INSERT:
            case WXK_HELP:
            case WXK_F1:
            case WXK_F2:
            case WXK_F3:
            case WXK_F4:
            case WXK_F5:
            case WXK_F6:
            case WXK_F7:
            case WXK_F8:
            case WXK_F9:
            case WXK_F10:
            case WXK_F11:
            case WXK_F12:
            case WXK_F13:
            case WXK_F14:
            case WXK_F15:
            case WXK_F16:
            case WXK_F17:
            case WXK_F18:
            case WXK_F19:
            case WXK_F20:
            case WXK_F21:
            case WXK_F22:
            case WXK_F23:
            case WXK_F24:
            case WXK_NUMLOCK:
            case WXK_SCROLL:
            case WXK_PAGEUP:
            case WXK_PAGEDOWN:
            case WXK_NUMPAD_F1:
            case WXK_NUMPAD_F2:
            case WXK_NUMPAD_F3:
            case WXK_NUMPAD_F4:
            case WXK_NUMPAD_HOME:
            case WXK_NUMPAD_LEFT:
            case WXK_NUMPAD_UP:
            case WXK_NUMPAD_RIGHT:
            case WXK_NUMPAD_DOWN:
            case WXK_NUMPAD_PAGEUP:
            case WXK_NUMPAD_PAGEDOWN:
            case WXK_NUMPAD_END:
            case WXK_NUMPAD_BEGIN:
            case WXK_NUMPAD_INSERT:
            case WXK_WINDOWS_LEFT:
            {
                return;
            }
            default:
            {
            }
        }

        // Must process this before translation, otherwise it's translated into a WXK_DELETE event.
        if (event.CmdDown() && event.GetKeyCode() == WXK_BACK)
        {
            BeginBatchUndo(_("Delete Text"));

            long newPos = m_caretPosition;

            bool processed = DeleteSelectedContent(& newPos);

            // Submit range in character positions, which are greater than caret positions,
            // so subtract 1 for deleted character and add 1 for conversion to character position.
            if (newPos > -1)
            {
                if (event.CmdDown())
                {
                    long pos = wxRichTextCtrl::FindNextWordPosition(-1);
                    if (pos < newPos)
                    {
                        GetBuffer().DeleteRangeWithUndo(wxRichTextRange(pos+1, newPos), this);
                        processed = true;
                    }
                }

                if (!processed)
                    GetBuffer().DeleteRangeWithUndo(wxRichTextRange(newPos, newPos), this);
            }

            EndBatchUndo();

            if (GetLastPosition() == -1)
            {
                GetBuffer().Reset();

                m_caretPosition = -1;
                PositionCaret();
                SetDefaultStyleToCursorStyle();
            }

            ScrollIntoView(m_caretPosition, WXK_LEFT);

            wxRichTextEvent cmdEvent(
                wxEVT_COMMAND_RICHTEXT_DELETE,
                GetId());
            cmdEvent.SetEventObject(this);
            cmdEvent.SetFlags(flags);
            cmdEvent.SetPosition(m_caretPosition+1);
            GetEventHandler()->ProcessEvent(cmdEvent);

            Update();
        }
        else
            event.Skip();

        return;
    }

    // all the other keys modify the controls contents which shouldn't be
    // possible if we're read-only
    if ( !IsEditable() )
    {
        event.Skip();
        return;
    }

    if (event.GetKeyCode() == WXK_RETURN)
    {
        BeginBatchUndo(_("Insert Text"));

        long newPos = m_caretPosition;

        DeleteSelectedContent(& newPos);

        if (event.ShiftDown())
        {
            wxString text;
            text = wxRichTextLineBreakChar;
            GetBuffer().InsertTextWithUndo(newPos+1, text, this);
            m_caretAtLineStart = true;
            PositionCaret();
        }
        else
            GetBuffer().InsertNewlineWithUndo(newPos+1, this, wxRICHTEXT_INSERT_WITH_PREVIOUS_PARAGRAPH_STYLE|wxRICHTEXT_INSERT_INTERACTIVE);

        EndBatchUndo();
        SetDefaultStyleToCursorStyle();

        ScrollIntoView(m_caretPosition, WXK_RIGHT);

        wxRichTextEvent cmdEvent(
            wxEVT_COMMAND_RICHTEXT_RETURN,
            GetId());
        cmdEvent.SetEventObject(this);
        cmdEvent.SetFlags(flags);
        cmdEvent.SetPosition(newPos+1);

        if (!GetEventHandler()->ProcessEvent(cmdEvent))
        {
            // Generate conventional event
            wxCommandEvent textEvent(wxEVT_COMMAND_TEXT_ENTER, GetId());
            InitCommandEvent(textEvent);

            GetEventHandler()->ProcessEvent(textEvent);
        }
        Update();
    }
    else if (event.GetKeyCode() == WXK_BACK)
    {
        BeginBatchUndo(_("Delete Text"));

        long newPos = m_caretPosition;

        bool processed = DeleteSelectedContent(& newPos);

        // Submit range in character positions, which are greater than caret positions,
        // so subtract 1 for deleted character and add 1 for conversion to character position.
        if (newPos > -1)
        {
            if (event.CmdDown())
            {
                long pos = wxRichTextCtrl::FindNextWordPosition(-1);
                if (pos < newPos)
                {
                    GetBuffer().DeleteRangeWithUndo(wxRichTextRange(pos+1, newPos), this);
                    processed = true;
                }
            }

            if (!processed)
                GetBuffer().DeleteRangeWithUndo(wxRichTextRange(newPos, newPos), this);
        }

        EndBatchUndo();

        if (GetLastPosition() == -1)
        {
            GetBuffer().Reset();

            m_caretPosition = -1;
            PositionCaret();
            SetDefaultStyleToCursorStyle();
        }

        ScrollIntoView(m_caretPosition, WXK_LEFT);

        wxRichTextEvent cmdEvent(
            wxEVT_COMMAND_RICHTEXT_DELETE,
            GetId());
        cmdEvent.SetEventObject(this);
        cmdEvent.SetFlags(flags);
        cmdEvent.SetPosition(m_caretPosition+1);
        GetEventHandler()->ProcessEvent(cmdEvent);

        Update();
    }
    else if (event.GetKeyCode() == WXK_DELETE)
    {
        BeginBatchUndo(_("Delete Text"));

        long newPos = m_caretPosition;

        bool processed = DeleteSelectedContent(& newPos);

        // Submit range in character positions, which are greater than caret positions,
        if (newPos < GetBuffer().GetRange().GetEnd()+1)
        {
            if (event.CmdDown())
            {
                long pos = wxRichTextCtrl::FindNextWordPosition(1);
                if (pos != -1 && (pos > newPos))
                {
                    GetBuffer().DeleteRangeWithUndo(wxRichTextRange(newPos+1, pos), this);
                    processed = true;
                }
            }

            if (!processed && newPos < (GetLastPosition()-1))
                GetBuffer().DeleteRangeWithUndo(wxRichTextRange(newPos+1, newPos+1), this);
        }

        EndBatchUndo();

        if (GetLastPosition() == -1)
        {
            GetBuffer().Reset();

            m_caretPosition = -1;
            PositionCaret();
            SetDefaultStyleToCursorStyle();
        }

        wxRichTextEvent cmdEvent(
            wxEVT_COMMAND_RICHTEXT_DELETE,
            GetId());
        cmdEvent.SetEventObject(this);
        cmdEvent.SetFlags(flags);
        cmdEvent.SetPosition(m_caretPosition+1);
        GetEventHandler()->ProcessEvent(cmdEvent);

        Update();
    }
    else
    {
        long keycode = event.GetKeyCode();
        switch ( keycode )
        {
            case WXK_ESCAPE:
            {
                event.Skip();
                return;
            }

            default:
            {
#ifdef __WXMAC__
                if (event.CmdDown())
#else
                // Fixes AltGr+key with European input languages on Windows
                if ((event.CmdDown() && !event.AltDown()) || (event.AltDown() && !event.CmdDown()))
#endif
                {
                    event.Skip();
                    return;
                }

                wxRichTextEvent cmdEvent(
                    wxEVT_COMMAND_RICHTEXT_CHARACTER,
                    GetId());
                cmdEvent.SetEventObject(this);
                cmdEvent.SetFlags(flags);
#if wxUSE_UNICODE
                cmdEvent.SetCharacter(event.GetUnicodeKey());
#else
                cmdEvent.SetCharacter((wxChar) keycode);
#endif
                cmdEvent.SetPosition(m_caretPosition+1);

                if (keycode == wxT('\t'))
                {
                    // See if we need to promote or demote the selection or paragraph at the cursor
                    // position, instead of inserting a tab.
                    long pos = GetAdjustedCaretPosition(GetCaretPosition());
                    wxRichTextParagraph* para = GetBuffer().GetParagraphAtPosition(pos);
                    if (para && para->GetRange().GetStart() == pos && para->GetAttributes().HasListStyleName())
                    {
                        wxRichTextRange range;
                        if (HasSelection())
                            range = GetSelectionRange();
                        else
                            range = para->GetRange().FromInternal();

                        int promoteBy = event.ShiftDown() ? 1 : -1;

                        PromoteList(promoteBy, range, NULL);

                        GetEventHandler()->ProcessEvent(cmdEvent);

                        return;
                    }
                }

                BeginBatchUndo(_("Insert Text"));

                long newPos = m_caretPosition;
                DeleteSelectedContent(& newPos);

#if wxUSE_UNICODE
                wxString str = event.GetUnicodeKey();
#else
                wxString str = (wxChar) event.GetKeyCode();
#endif
                GetBuffer().InsertTextWithUndo(newPos+1, str, this, 0);

                EndBatchUndo();

                SetDefaultStyleToCursorStyle();
                ScrollIntoView(m_caretPosition, WXK_RIGHT);

                GetEventHandler()->ProcessEvent(cmdEvent);

                Update();
            }
        }
    }
}

/// Delete content if there is a selection, e.g. when pressing a key.
bool wxRichTextCtrl::DeleteSelectedContent(long* newPos)
{
    if (HasSelection())
    {
        long pos = m_selectionRange.GetStart();
        wxRichTextRange range = m_selectionRange;

        // SelectAll causes more to be selected than doing it interactively,
        // and causes a new paragraph to be inserted. So for multiline buffers,
        // don't delete the final position.
        if (range.GetEnd() == GetLastPosition() && GetNumberOfLines() > 0)
            range.SetEnd(range.GetEnd()-1);

        GetBuffer().DeleteRangeWithUndo(range, this);
        m_selectionRange.SetRange(-2, -2);

        if (newPos)
            *newPos = pos-1;
        return true;
    }
    else
        return false;
}

/// Keyboard navigation

/*

Left:       left one character
Right:      right one character
Up:         up one line
Down:       down one line
Ctrl-Left:  left one word
Ctrl-Right: right one word
Ctrl-Up:    previous paragraph start
Ctrl-Down:  next start of paragraph
Home:       start of line
End:        end of line
Ctrl-Home:  start of document
Ctrl-End:   end of document
Page-Up:    Up a screen
Page-Down:  Down a screen

Maybe:

Ctrl-Alt-PgUp: Start of window
Ctrl-Alt-PgDn: End of window
F8:         Start selection mode
Esc:        End selection mode

Adding Shift does the above but starts/extends selection.


 */

bool wxRichTextCtrl::KeyboardNavigate(int keyCode, int flags)
{
    bool success = false;

    if (keyCode == WXK_RIGHT || keyCode == WXK_NUMPAD_RIGHT)
    {
        if (flags & wxRICHTEXT_CTRL_DOWN)
            success = WordRight(1, flags);
        else
            success = MoveRight(1, flags);
    }
    else if (keyCode == WXK_LEFT || keyCode == WXK_NUMPAD_LEFT)
    {
        if (flags & wxRICHTEXT_CTRL_DOWN)
            success = WordLeft(1, flags);
        else
            success = MoveLeft(1, flags);
    }
    else if (keyCode == WXK_UP || keyCode == WXK_NUMPAD_UP)
    {
        if (flags & wxRICHTEXT_CTRL_DOWN)
            success = MoveToParagraphStart(flags);
        else
            success = MoveUp(1, flags);
    }
    else if (keyCode == WXK_DOWN || keyCode == WXK_NUMPAD_DOWN)
    {
        if (flags & wxRICHTEXT_CTRL_DOWN)
            success = MoveToParagraphEnd(flags);
        else
            success = MoveDown(1, flags);
    }
    else if (keyCode == WXK_PAGEUP || keyCode == WXK_NUMPAD_PAGEUP)
    {
        success = PageUp(1, flags);
    }
    else if (keyCode == WXK_PAGEDOWN || keyCode == WXK_NUMPAD_PAGEDOWN)
    {
        success = PageDown(1, flags);
    }
    else if (keyCode == WXK_HOME || keyCode == WXK_NUMPAD_HOME)
    {
        if (flags & wxRICHTEXT_CTRL_DOWN)
            success = MoveHome(flags);
        else
            success = MoveToLineStart(flags);
    }
    else if (keyCode == WXK_END || keyCode == WXK_NUMPAD_END)
    {
        if (flags & wxRICHTEXT_CTRL_DOWN)
            success = MoveEnd(flags);
        else
            success = MoveToLineEnd(flags);
    }

    if (success)
    {
        ScrollIntoView(m_caretPosition, keyCode);
        SetDefaultStyleToCursorStyle();
    }

    return success;
}

/// Extend the selection. Selections are in caret positions.
bool wxRichTextCtrl::ExtendSelection(long oldPos, long newPos, int flags)
{
    if (flags & wxRICHTEXT_SHIFT_DOWN)
    {
        if (oldPos == newPos)
            return false;

        wxRichTextRange oldSelection = m_selectionRange;

        // If not currently selecting, start selecting
        if (m_selectionRange.GetStart() == -2)
        {
            m_selectionAnchor = oldPos;

            if (oldPos > newPos)
                m_selectionRange.SetRange(newPos+1, oldPos);
            else
                m_selectionRange.SetRange(oldPos+1, newPos);
        }
        else
        {
            // Always ensure that the selection range start is greater than
            // the end.
            if (newPos > m_selectionAnchor)
                m_selectionRange.SetRange(m_selectionAnchor+1, newPos);
            else if (newPos == m_selectionAnchor)
                m_selectionRange = wxRichTextRange(-2, -2);
            else
                m_selectionRange.SetRange(newPos+1, m_selectionAnchor);
        }

        wxRichTextCtrlRefreshForSelectionChange(*this, oldSelection, m_selectionRange);

        if (m_selectionRange.GetStart() > m_selectionRange.GetEnd())
        {
            wxLogDebug(wxT("Strange selection range"));
        }

        return true;
    }
    else
        return false;
}

/// Scroll into view, returning true if we scrolled.
/// This takes a _caret_ position.
bool wxRichTextCtrl::ScrollIntoView(long position, int keyCode)
{
    wxRichTextLine* line = GetVisibleLineForCaretPosition(position);

    if (!line)
        return false;

    int ppuX, ppuY;
    GetScrollPixelsPerUnit(& ppuX, & ppuY);

    int startXUnits, startYUnits;
    GetViewStart(& startXUnits, & startYUnits);
    int startY = startYUnits * ppuY;

    int sx = 0, sy = 0;
    GetVirtualSize(& sx, & sy);
    int sxUnits = 0;
    int syUnits = 0;
    if (ppuY != 0)
        syUnits = sy/ppuY;

    wxRect rect = line->GetRect();

    bool scrolled = false;

    wxSize clientSize = GetClientSize();
    clientSize.y -= GetBuffer().GetBottomMargin();

    if (GetWindowStyle() & wxRE_CENTRE_CARET)
    {
        int y = rect.y - GetClientSize().y/2;
        int yUnits = (int) (0.5 + ((float) y)/(float) ppuY);
        if (y >= 0 && (y + clientSize.y) < GetBuffer().GetCachedSize().y)
        {
            if (startYUnits != yUnits)
            {
                SetScrollbars(ppuX, ppuY, sxUnits, syUnits, 0, yUnits);
                scrolled = true;
            }
#if !wxRICHTEXT_USE_OWN_CARET
            if (scrolled)
#endif
                PositionCaret();

            return scrolled;
        }
    }

    // Going down
    if (keyCode == WXK_DOWN || keyCode == WXK_NUMPAD_DOWN ||
        keyCode == WXK_RIGHT || keyCode == WXK_NUMPAD_RIGHT ||
        keyCode == WXK_END || keyCode == WXK_NUMPAD_END ||
        keyCode == WXK_PAGEDOWN || keyCode == WXK_NUMPAD_PAGEDOWN)
    {
        if ((rect.y + rect.height) > (clientSize.y + startY))
        {
            // Make it scroll so this item is at the bottom
            // of the window
            int y = rect.y - (clientSize.y - rect.height);
            int yUnits = (int) (0.5 + ((float) y)/(float) ppuY);

            // If we're still off the screen, scroll another line down
            if ((rect.y + rect.height) > (clientSize.y + (yUnits*ppuY)))
                yUnits ++;

            if (startYUnits != yUnits)
            {
                SetScrollbars(ppuX, ppuY, sxUnits, syUnits, 0, yUnits);
                scrolled = true;
            }
        }
        else if (rect.y < (startY + GetBuffer().GetTopMargin()))
        {
            // Make it scroll so this item is at the top
            // of the window
            int y = rect.y - GetBuffer().GetTopMargin();
            int yUnits = (int) (0.5 + ((float) y)/(float) ppuY);

            if (startYUnits != yUnits)
            {
                SetScrollbars(ppuX, ppuY, sxUnits, syUnits, 0, yUnits);
                scrolled = true;
            }
        }
    }
    // Going up
    else if (keyCode == WXK_UP  || keyCode == WXK_NUMPAD_UP ||
             keyCode == WXK_LEFT || keyCode == WXK_NUMPAD_LEFT ||
             keyCode == WXK_HOME || keyCode == WXK_NUMPAD_HOME ||
             keyCode == WXK_PAGEUP || keyCode == WXK_NUMPAD_PAGEUP )
    {
        if (rect.y < (startY + GetBuffer().GetBottomMargin()))
        {
            // Make it scroll so this item is at the top
            // of the window
            int y = rect.y - GetBuffer().GetTopMargin();
            int yUnits = (int) (0.5 + ((float) y)/(float) ppuY);

            if (startYUnits != yUnits)
            {
                SetScrollbars(ppuX, ppuY, sxUnits, syUnits, 0, yUnits);
                scrolled = true;
            }
        }
        else if ((rect.y + rect.height) > (clientSize.y + startY))
        {
            // Make it scroll so this item is at the bottom
            // of the window
            int y = rect.y - (clientSize.y - rect.height);
            int yUnits = (int) (0.5 + ((float) y)/(float) ppuY);

            // If we're still off the screen, scroll another line down
            if ((rect.y + rect.height) > (clientSize.y + (yUnits*ppuY)))
                yUnits ++;

            if (startYUnits != yUnits)
            {
                SetScrollbars(ppuX, ppuY, sxUnits, syUnits, 0, yUnits);
                scrolled = true;
            }
        }
    }

#if !wxRICHTEXT_USE_OWN_CARET
    if (scrolled)
#endif
        PositionCaret();

    return scrolled;
}

/// Is the given position visible on the screen?
bool wxRichTextCtrl::IsPositionVisible(long pos) const
{
    wxRichTextLine* line = GetVisibleLineForCaretPosition(pos-1);

    if (!line)
        return false;

    int ppuX, ppuY;
    GetScrollPixelsPerUnit(& ppuX, & ppuY);

    int startX, startY;
    GetViewStart(& startX, & startY);
    startX = 0;
    startY = startY * ppuY;

    wxRect rect = line->GetRect();
    wxSize clientSize = GetClientSize();
    clientSize.y -= GetBuffer().GetBottomMargin();

    return (rect.GetTop() >= (startY + GetBuffer().GetTopMargin())) && (rect.GetBottom() <= (startY + clientSize.y));
}

void wxRichTextCtrl::SetCaretPosition(long position, bool showAtLineStart)
{
    m_caretPosition = position;
    m_caretAtLineStart = showAtLineStart;
}

/// Move caret one visual step forward: this may mean setting a flag
/// and keeping the same position if we're going from the end of one line
/// to the start of the next, which may be the exact same caret position.
void wxRichTextCtrl::MoveCaretForward(long oldPosition)
{
    wxRichTextParagraph* para = GetBuffer().GetParagraphAtPosition(oldPosition);

    // Only do the check if we're not at the end of the paragraph (where things work OK
    // anyway)
    if (para && (oldPosition != para->GetRange().GetEnd() - 1))
    {
        wxRichTextLine* line = GetBuffer().GetLineAtPosition(oldPosition);

        if (line)
        {
            wxRichTextRange lineRange = line->GetAbsoluteRange();

            // We're at the end of a line. See whether we need to
            // stay at the same actual caret position but change visual
            // position, or not.
            if (oldPosition == lineRange.GetEnd())
            {
                if (m_caretAtLineStart)
                {
                    // We're already at the start of the line, so actually move on now.
                    m_caretPosition = oldPosition + 1;
                    m_caretAtLineStart = false;
                }
                else
                {
                    // We're showing at the end of the line, so keep to
                    // the same position but indicate that we're to show
                    // at the start of the next line.
                    m_caretPosition = oldPosition;
                    m_caretAtLineStart = true;
                }
                SetDefaultStyleToCursorStyle();
                return;
            }
        }
    }
    m_caretPosition ++;
    SetDefaultStyleToCursorStyle();
}

/// Move caret one visual step backward: this may mean setting a flag
/// and keeping the same position if we're going from the end of one line
/// to the start of the next, which may be the exact same caret position.
void wxRichTextCtrl::MoveCaretBack(long oldPosition)
{
    wxRichTextParagraph* para = GetBuffer().GetParagraphAtPosition(oldPosition);

    // Only do the check if we're not at the start of the paragraph (where things work OK
    // anyway)
    if (para && (oldPosition != para->GetRange().GetStart()))
    {
        wxRichTextLine* line = GetBuffer().GetLineAtPosition(oldPosition);

        if (line)
        {
            wxRichTextRange lineRange = line->GetAbsoluteRange();

            // We're at the start of a line. See whether we need to
            // stay at the same actual caret position but change visual
            // position, or not.
            if (oldPosition == lineRange.GetStart())
            {
                m_caretPosition = oldPosition-1;
                m_caretAtLineStart = true;
                return;
            }
            else if (oldPosition == lineRange.GetEnd())
            {
                if (m_caretAtLineStart)
                {
                    // We're at the start of the line, so keep the same caret position
                    // but clear the start-of-line flag.
                    m_caretPosition = oldPosition;
                    m_caretAtLineStart = false;
                }
                else
                {
                    // We're showing at the end of the line, so go back
                    // to the previous character position.
                    m_caretPosition = oldPosition - 1;
                }
                SetDefaultStyleToCursorStyle();
                return;
            }
        }
    }
    m_caretPosition --;
    SetDefaultStyleToCursorStyle();
}

/// Move right
bool wxRichTextCtrl::MoveRight(int noPositions, int flags)
{
    long endPos = GetBuffer().GetRange().GetEnd();

    if (m_caretPosition + noPositions < endPos)
    {
        long oldPos = m_caretPosition;
        long newPos = m_caretPosition + noPositions;

        bool extendSel = ExtendSelection(m_caretPosition, newPos, flags);
        if (!extendSel)
            SelectNone();

        // Determine by looking at oldPos and m_caretPosition whether
        // we moved from the end of a line to the start of the next line, in which case
        // we want to adjust the caret position such that it is positioned at the
        // start of the next line, rather than jumping past the first character of the
        // line.
        if (noPositions == 1 && !extendSel)
            MoveCaretForward(oldPos);
        else
            SetCaretPosition(newPos);

        PositionCaret();
        SetDefaultStyleToCursorStyle();

        return true;
    }
    else
        return false;
}

/// Move left
bool wxRichTextCtrl::MoveLeft(int noPositions, int flags)
{
    long startPos = -1;

    if (m_caretPosition > startPos - noPositions + 1)
    {
        long oldPos = m_caretPosition;
        long newPos = m_caretPosition - noPositions;
        bool extendSel = ExtendSelection(m_caretPosition, newPos, flags);
        if (!extendSel)
            SelectNone();

        if (noPositions == 1 && !extendSel)
            MoveCaretBack(oldPos);
        else
            SetCaretPosition(newPos);

        PositionCaret();
        SetDefaultStyleToCursorStyle();

        return true;
    }
    else
        return false;
}

/// Move up
bool wxRichTextCtrl::MoveUp(int noLines, int flags)
{
    return MoveDown(- noLines, flags);
}

/// Move up
bool wxRichTextCtrl::MoveDown(int noLines, int flags)
{
    if (!GetCaret())
        return false;

    long lineNumber = GetBuffer().GetVisibleLineNumber(m_caretPosition, true, m_caretAtLineStart);
    wxPoint pt = GetCaret()->GetPosition();
    long newLine = lineNumber + noLines;

    if (lineNumber != -1)
    {
        if (noLines > 0)
        {
            long lastLine = GetBuffer().GetVisibleLineNumber(GetBuffer().GetRange().GetEnd());

            if (newLine > lastLine)
                return false;
        }
        else
        {
            if (newLine < 0)
                return false;
        }
    }

    wxRichTextLine* lineObj = GetBuffer().GetLineForVisibleLineNumber(newLine);
    if (lineObj)
    {
        pt.y = lineObj->GetAbsolutePosition().y + 2;
    }
    else
        return false;

    long newPos = 0;
    wxClientDC dc(this);
    PrepareDC(dc);
    dc.SetFont(GetFont());

    int hitTest = GetBuffer().HitTest(dc, pt, newPos);

    if (hitTest != wxRICHTEXT_HITTEST_NONE)
    {
        // If end of previous line, and hitTest is wxRICHTEXT_HITTEST_BEFORE,
        // we want to be at the end of the last line but with m_caretAtLineStart set to true,
        // so we view the caret at the start of the line.
        bool caretLineStart = false;
        if (hitTest & wxRICHTEXT_HITTEST_BEFORE)
        {
            wxRichTextLine* thisLine = GetBuffer().GetLineAtPosition(newPos-1);
            wxRichTextRange lineRange;
            if (thisLine)
                lineRange = thisLine->GetAbsoluteRange();

            if (thisLine && (newPos-1) == lineRange.GetEnd())
            {
                newPos --;
                caretLineStart = true;
            }
            else
            {
                wxRichTextParagraph* para = GetBuffer().GetParagraphAtPosition(newPos);
                if (para && para->GetRange().GetStart() == newPos)
                    newPos --;
            }
        }

        long newSelEnd = newPos;

        bool extendSel = ExtendSelection(m_caretPosition, newSelEnd, flags);
        if (!extendSel)
            SelectNone();

        SetCaretPosition(newPos, caretLineStart);
        PositionCaret();
        SetDefaultStyleToCursorStyle();

        return true;
    }

    return false;
}

/// Move to the end of the paragraph
bool wxRichTextCtrl::MoveToParagraphEnd(int flags)
{
    wxRichTextParagraph* para = GetBuffer().GetParagraphAtPosition(m_caretPosition, true);
    if (para)
    {
        long newPos = para->GetRange().GetEnd() - 1;
        bool extendSel = ExtendSelection(m_caretPosition, newPos, flags);
        if (!extendSel)
            SelectNone();

        SetCaretPosition(newPos);
        PositionCaret();
        SetDefaultStyleToCursorStyle();

        return true;
    }

    return false;
}

/// Move to the start of the paragraph
bool wxRichTextCtrl::MoveToParagraphStart(int flags)
{
    wxRichTextParagraph* para = GetBuffer().GetParagraphAtPosition(m_caretPosition, true);
    if (para)
    {
        long newPos = para->GetRange().GetStart() - 1;
        bool extendSel = ExtendSelection(m_caretPosition, newPos, flags);
        if (!extendSel)
            SelectNone();

        SetCaretPosition(newPos);
        PositionCaret();
        SetDefaultStyleToCursorStyle();

        return true;
    }

    return false;
}

/// Move to the end of the line
bool wxRichTextCtrl::MoveToLineEnd(int flags)
{
    wxRichTextLine* line = GetVisibleLineForCaretPosition(m_caretPosition);

    if (line)
    {
        wxRichTextRange lineRange = line->GetAbsoluteRange();
        long newPos = lineRange.GetEnd();
        bool extendSel = ExtendSelection(m_caretPosition, newPos, flags);
        if (!extendSel)
            SelectNone();

        SetCaretPosition(newPos);
        PositionCaret();
        SetDefaultStyleToCursorStyle();

        return true;
    }

    return false;
}

/// Move to the start of the line
bool wxRichTextCtrl::MoveToLineStart(int flags)
{
    wxRichTextLine* line = GetVisibleLineForCaretPosition(m_caretPosition);
    if (line)
    {
        wxRichTextRange lineRange = line->GetAbsoluteRange();
        long newPos = lineRange.GetStart()-1;

        bool extendSel = ExtendSelection(m_caretPosition, newPos, flags);
        if (!extendSel)
            SelectNone();

        wxRichTextParagraph* para = GetBuffer().GetParagraphForLine(line);

        SetCaretPosition(newPos, para->GetRange().GetStart() != lineRange.GetStart());
        PositionCaret();
        SetDefaultStyleToCursorStyle();

        return true;
    }

    return false;
}

/// Move to the start of the buffer
bool wxRichTextCtrl::MoveHome(int flags)
{
    if (m_caretPosition != -1)
    {
        bool extendSel = ExtendSelection(m_caretPosition, -1, flags);
        if (!extendSel)
            SelectNone();

        SetCaretPosition(-1);
        PositionCaret();
        SetDefaultStyleToCursorStyle();

        return true;
    }
    else
        return false;
}

/// Move to the end of the buffer
bool wxRichTextCtrl::MoveEnd(int flags)
{
    long endPos = GetBuffer().GetRange().GetEnd()-1;

    if (m_caretPosition != endPos)
    {
        bool extendSel = ExtendSelection(m_caretPosition, endPos, flags);
        if (!extendSel)
            SelectNone();

        SetCaretPosition(endPos);
        PositionCaret();
        SetDefaultStyleToCursorStyle();

        return true;
    }
    else
        return false;
}

/// Move noPages pages up
bool wxRichTextCtrl::PageUp(int noPages, int flags)
{
    return PageDown(- noPages, flags);
}

/// Move noPages pages down
bool wxRichTextCtrl::PageDown(int noPages, int flags)
{
    // Calculate which line occurs noPages * screen height further down.
    wxRichTextLine* line = GetVisibleLineForCaretPosition(m_caretPosition);
    if (line)
    {
        wxSize clientSize = GetClientSize();
        int newY = line->GetAbsolutePosition().y + noPages*clientSize.y;

        wxRichTextLine* newLine = GetBuffer().GetLineAtYPosition(newY);
        if (newLine)
        {
            wxRichTextRange lineRange = newLine->GetAbsoluteRange();
            long pos = lineRange.GetStart()-1;
            if (pos != m_caretPosition)
            {
                wxRichTextParagraph* para = GetBuffer().GetParagraphForLine(newLine);

                bool extendSel = ExtendSelection(m_caretPosition, pos, flags);
                if (!extendSel)
                    SelectNone();

                SetCaretPosition(pos, para->GetRange().GetStart() != lineRange.GetStart());
                PositionCaret();
                SetDefaultStyleToCursorStyle();

                return true;
            }
        }
    }

    return false;
}

static bool wxRichTextCtrlIsWhitespace(const wxString& str)
{
    return str == wxT(" ") || str == wxT("\t");
}

// Finds the caret position for the next word
long wxRichTextCtrl::FindNextWordPosition(int direction) const
{
    long endPos = GetBuffer().GetRange().GetEnd();

    if (direction > 0)
    {
        long i = m_caretPosition+1+direction; // +1 for conversion to character pos

        // First skip current text to space
        while (i < endPos && i > -1)
        {
            // i is in character, not caret positions
            wxString text = GetBuffer().GetTextForRange(wxRichTextRange(i, i));
            wxRichTextLine* line = GetBuffer().GetLineAtPosition(i, false);
            if (line && (i == line->GetAbsoluteRange().GetEnd()))
            {
                break;
            }
            else if (!wxRichTextCtrlIsWhitespace(text) && !text.empty())
                i += direction;
            else
            {
                break;
            }
        }
        while (i < endPos && i > -1)
        {
            // i is in character, not caret positions
            wxString text = GetBuffer().GetTextForRange(wxRichTextRange(i, i));
            wxRichTextLine* line = GetBuffer().GetLineAtPosition(i, false);
            if (line && (i == line->GetAbsoluteRange().GetEnd()))
                return wxMax(-1, i);

            if (text.empty()) // End of paragraph, or maybe an image
                return wxMax(-1, i - 1);
            else if (wxRichTextCtrlIsWhitespace(text) || text.empty())
                i += direction;
            else
            {
                // Convert to caret position
                return wxMax(-1, i - 1);
            }
        }
        if (i >= endPos)
            return endPos-1;
        return i-1;
    }
    else
    {
        long i = m_caretPosition;

        // First skip white space
        while (i < endPos && i > -1)
        {
            // i is in character, not caret positions
            wxString text = GetBuffer().GetTextForRange(wxRichTextRange(i, i));
            wxRichTextLine* line = GetBuffer().GetLineAtPosition(i, false);

            if (text.empty() || (line && (i == line->GetAbsoluteRange().GetStart()))) // End of paragraph, or maybe an image
                break;
            else if (wxRichTextCtrlIsWhitespace(text) || text.empty())
                i += direction;
            else
                break;
        }
        // Next skip current text to space
        while (i < endPos && i > -1)
        {
            // i is in character, not caret positions
            wxString text = GetBuffer().GetTextForRange(wxRichTextRange(i, i));
            wxRichTextLine* line = GetBuffer().GetLineAtPosition(i, false);
            if (line && line->GetAbsoluteRange().GetStart() == i)
                return i-1;

            if (!wxRichTextCtrlIsWhitespace(text) /* && !text.empty() */)
                i += direction;
            else
            {
                return i;
            }
        }
        if (i < -1)
            return -1;
        return i;
    }
}

/// Move n words left
bool wxRichTextCtrl::WordLeft(int WXUNUSED(n), int flags)
{
    long pos = FindNextWordPosition(-1);
    if (pos != m_caretPosition)
    {
        wxRichTextParagraph* para = GetBuffer().GetParagraphAtPosition(pos, true);

        bool extendSel = ExtendSelection(m_caretPosition, pos, flags);
        if (!extendSel)
            SelectNone();

        SetCaretPosition(pos, para->GetRange().GetStart() != pos);
        PositionCaret();
        SetDefaultStyleToCursorStyle();

        return true;
    }

    return false;
}

/// Move n words right
bool wxRichTextCtrl::WordRight(int WXUNUSED(n), int flags)
{
    long pos = FindNextWordPosition(1);
    if (pos != m_caretPosition)
    {
        wxRichTextParagraph* para = GetBuffer().GetParagraphAtPosition(pos, true);

        bool extendSel = ExtendSelection(m_caretPosition, pos, flags);
        if (!extendSel)
            SelectNone();

        SetCaretPosition(pos, para->GetRange().GetStart() != pos);
        PositionCaret();
        SetDefaultStyleToCursorStyle();

        return true;
    }

    return false;
}

/// Sizing
void wxRichTextCtrl::OnSize(wxSizeEvent& event)
{
    // Only do sizing optimization for large buffers
    if (GetBuffer().GetRange().GetEnd() > m_delayedLayoutThreshold)
    {
        m_fullLayoutRequired = true;
        m_fullLayoutTime = wxGetLocalTimeMillis();
        m_fullLayoutSavedPosition = GetFirstVisiblePosition();
        LayoutContent(true /* onlyVisibleRect */);
    }
    else
        GetBuffer().Invalidate(wxRICHTEXT_ALL);

#if wxRICHTEXT_BUFFERED_PAINTING
    RecreateBuffer();
#endif

    event.Skip();
}

/// Idle-time processing
void wxRichTextCtrl::OnIdle(wxIdleEvent& event)
{
#if wxRICHTEXT_USE_OWN_CARET
    if (((wxRichTextCaret*) GetCaret())->GetNeedsUpdate())
    {
        ((wxRichTextCaret*) GetCaret())->SetNeedsUpdate(false);
        PositionCaret();
        GetCaret()->Show();
    }
#endif

    const int layoutInterval = wxRICHTEXT_DEFAULT_LAYOUT_INTERVAL;

    if (m_fullLayoutRequired && (wxGetLocalTimeMillis() > (m_fullLayoutTime + layoutInterval)))
    {
        m_fullLayoutRequired = false;
        m_fullLayoutTime = 0;
        GetBuffer().Invalidate(wxRICHTEXT_ALL);
        ShowPosition(m_fullLayoutSavedPosition);
        Refresh(false);
    }

    if (m_caretPositionForDefaultStyle != -2)
    {
        // If the caret position has changed, no longer reflect the default style
        // in the UI.
        if (GetCaretPosition() != m_caretPositionForDefaultStyle)
            m_caretPositionForDefaultStyle = -2;
    }

    event.Skip();
}

/// Scrolling
void wxRichTextCtrl::OnScroll(wxScrollWinEvent& event)
{
#if wxRICHTEXT_USE_OWN_CARET
    if (!((wxRichTextCaret*) GetCaret())->GetNeedsUpdate())
    {
        GetCaret()->Hide();
        ((wxRichTextCaret*) GetCaret())->SetNeedsUpdate();
    }
#endif

    event.Skip();
}

/// Set up scrollbars, e.g. after a resize
void wxRichTextCtrl::SetupScrollbars(bool atTop)
{
    if (m_freezeCount)
        return;

    if (GetBuffer().IsEmpty())
    {
        SetScrollbars(0, 0, 0, 0, 0, 0);
        return;
    }

    // TODO: reimplement scrolling so we scroll by line, not by fixed number
    // of pixels. See e.g. wxVScrolledWindow for ideas.
    int pixelsPerUnit = 5;
    wxSize clientSize = GetClientSize();

    int maxHeight = GetBuffer().GetCachedSize().y + GetBuffer().GetTopMargin();

    // Round up so we have at least maxHeight pixels
    int unitsY = (int) (((float)maxHeight/(float)pixelsPerUnit) + 0.5);

    int startX = 0, startY = 0;
    if (!atTop)
        GetViewStart(& startX, & startY);

    int maxPositionX = 0; // wxMax(sz.x - clientSize.x, 0);
    int maxPositionY = (int) ((((float)(wxMax((unitsY*pixelsPerUnit) - clientSize.y, 0)))/((float)pixelsPerUnit)) + 0.5);

    int newStartX = wxMin(maxPositionX, startX);
    int newStartY = wxMin(maxPositionY, startY);

    int oldPPUX, oldPPUY;
    int oldStartX, oldStartY;
    int oldVirtualSizeX = 0, oldVirtualSizeY = 0;
    GetScrollPixelsPerUnit(& oldPPUX, & oldPPUY);
    GetViewStart(& oldStartX, & oldStartY);
    GetVirtualSize(& oldVirtualSizeX, & oldVirtualSizeY);
    if (oldPPUY > 0)
        oldVirtualSizeY /= oldPPUY;

    if (oldPPUX == 0 && oldPPUY == pixelsPerUnit && oldVirtualSizeY == unitsY && oldStartX == newStartX && oldStartY == newStartY)
        return;

    // Move to previous scroll position if
    // possible
    SetScrollbars(0, pixelsPerUnit, 0, unitsY, newStartX, newStartY);
}

/// Paint the background
void wxRichTextCtrl::PaintBackground(wxDC& dc)
{
    wxColour backgroundColour = GetBackgroundColour();
    if (!backgroundColour.Ok())
        backgroundColour = wxSystemSettings::GetColour(wxSYS_COLOUR_3DFACE);

    // Clear the background
    dc.SetBrush(wxBrush(backgroundColour));
    dc.SetPen(*wxTRANSPARENT_PEN);
    wxRect windowRect(GetClientSize());
    windowRect.x -= 2; windowRect.y -= 2;
    windowRect.width += 4; windowRect.height += 4;

    // We need to shift the rectangle to take into account
    // scrolling. Converting device to logical coordinates.
    CalcUnscrolledPosition(windowRect.x, windowRect.y, & windowRect.x, & windowRect.y);
    dc.DrawRectangle(windowRect);
}

#if wxRICHTEXT_BUFFERED_PAINTING
/// Recreate buffer bitmap if necessary
bool wxRichTextCtrl::RecreateBuffer(const wxSize& size)
{
    wxSize sz = size;
    if (sz == wxDefaultSize)
        sz = GetClientSize();

    if (sz.x < 1 || sz.y < 1)
        return false;

    if (!m_bufferBitmap.Ok() || m_bufferBitmap.GetWidth() < sz.x || m_bufferBitmap.GetHeight() < sz.y)
        m_bufferBitmap = wxBitmap(sz.x, sz.y);
    return m_bufferBitmap.Ok();
}
#endif

// ----------------------------------------------------------------------------
// file IO functions
// ----------------------------------------------------------------------------

bool wxRichTextCtrl::DoLoadFile(const wxString& filename, int fileType)
{
    bool success = GetBuffer().LoadFile(filename, fileType);
    if (success)
        m_filename = filename;

    DiscardEdits();
    SetInsertionPoint(0);
    LayoutContent();
    PositionCaret();
    SetupScrollbars(true);
    Refresh(false);
    SendTextUpdatedEvent();

    if (success)
        return true;
    else
    {
        wxLogError(_("File couldn't be loaded."));

        return false;
    }
}

bool wxRichTextCtrl::DoSaveFile(const wxString& filename, int fileType)
{
    if (GetBuffer().SaveFile(filename, fileType))
    {
        m_filename = filename;

        DiscardEdits();

        return true;
    }

    wxLogError(_("The text couldn't be saved."));

    return false;
}

// ----------------------------------------------------------------------------
// wxRichTextCtrl specific functionality
// ----------------------------------------------------------------------------

/// Add a new paragraph of text to the end of the buffer
wxRichTextRange wxRichTextCtrl::AddParagraph(const wxString& text)
{
    wxRichTextRange range = GetBuffer().AddParagraph(text);
    LayoutContent();
    return range;
}

/// Add an image
wxRichTextRange wxRichTextCtrl::AddImage(const wxImage& image)
{
    wxRichTextRange range = GetBuffer().AddImage(image);
    LayoutContent();
    return range;
}

// ----------------------------------------------------------------------------
// selection and ranges
// ----------------------------------------------------------------------------

void wxRichTextCtrl::SelectAll()
{
    SetSelection(-1, -1);
}

/// Select none
void wxRichTextCtrl::SelectNone()
{
    if (!(GetSelectionRange() == wxRichTextRange(-2, -2)))
    {
        wxRichTextRange oldSelection = m_selectionRange;

        m_selectionRange = wxRichTextRange(-2, -2);

        wxRichTextCtrlRefreshForSelectionChange(*this, oldSelection, m_selectionRange);
    }
    m_selectionAnchor = -2;
}

static bool wxIsWordDelimiter(const wxString& text)
{
    return !text.IsEmpty() && !wxIsalnum(text[0]);
}

/// Select the word at the given character position
bool wxRichTextCtrl::SelectWord(long position)
{
    if (position < 0 || position > GetBuffer().GetRange().GetEnd())
        return false;

    wxRichTextParagraph* para = GetBuffer().GetParagraphAtPosition(position);
    if (!para)
        return false;

    if (position == para->GetRange().GetEnd())
        position --;

    long positionStart = position;
    long positionEnd = position;

    for (positionStart = position; positionStart >= para->GetRange().GetStart(); positionStart --)
    {
        wxString text = GetBuffer().GetTextForRange(wxRichTextRange(positionStart, positionStart));
        if (wxIsWordDelimiter(text))
        {
            positionStart ++;
            break;
        }
    }
    if (positionStart < para->GetRange().GetStart())
        positionStart = para->GetRange().GetStart();

    for (positionEnd = position; positionEnd < para->GetRange().GetEnd(); positionEnd ++)
    {
        wxString text = GetBuffer().GetTextForRange(wxRichTextRange(positionEnd, positionEnd));
        if (wxIsWordDelimiter(text))
        {
            positionEnd --;
            break;
        }
    }
    if (positionEnd >= para->GetRange().GetEnd())
        positionEnd = para->GetRange().GetEnd();

    if (positionEnd < positionStart)
        return false;

    SetSelection(positionStart, positionEnd+1);

    if (positionStart >= 0)
    {
        MoveCaret(positionStart-1, true);
        SetDefaultStyleToCursorStyle();
    }

    return true;
}

wxString wxRichTextCtrl::GetStringSelection() const
{
    long from, to;
    GetSelection(&from, &to);

    return GetRange(from, to);
}

// ----------------------------------------------------------------------------
// hit testing
// ----------------------------------------------------------------------------

wxTextCtrlHitTestResult
wxRichTextCtrl::HitTest(const wxPoint& pt, wxTextCoord *x, wxTextCoord *y) const
{
    // implement in terms of the other overload as the native ports typically
    // can get the position and not (x, y) pair directly (although wxUniv
    // directly gets x and y -- and so overrides this method as well)
    long pos;
    wxTextCtrlHitTestResult rc = HitTest(pt, &pos);

    if ( rc != wxTE_HT_UNKNOWN )
    {
        PositionToXY(pos, x, y);
    }

    return rc;
}

wxTextCtrlHitTestResult
wxRichTextCtrl::HitTest(const wxPoint& pt,
                        long * pos) const
{
    wxClientDC dc((wxRichTextCtrl*) this);
    ((wxRichTextCtrl*)this)->PrepareDC(dc);

    // Buffer uses logical position (relative to start of buffer)
    // so convert
    wxPoint pt2 = GetLogicalPoint(pt);

    int hit = ((wxRichTextCtrl*)this)->GetBuffer().HitTest(dc, pt2, *pos);

    if ((hit & wxRICHTEXT_HITTEST_BEFORE) && (hit & wxRICHTEXT_HITTEST_OUTSIDE))
        return wxTE_HT_BEFORE;
    else if ((hit & wxRICHTEXT_HITTEST_AFTER) && (hit & wxRICHTEXT_HITTEST_OUTSIDE))
        return wxTE_HT_BEYOND;
    else if (hit & (wxRICHTEXT_HITTEST_BEFORE|wxRICHTEXT_HITTEST_AFTER))
        return wxTE_HT_ON_TEXT;

    return wxTE_HT_UNKNOWN;
}

// ----------------------------------------------------------------------------
// set/get the controls text
// ----------------------------------------------------------------------------

wxString wxRichTextCtrl::GetValue() const
{
    return GetBuffer().GetText();
}

wxString wxRichTextCtrl::GetRange(long from, long to) const
{
    // Public API for range is different from internals
    return GetBuffer().GetTextForRange(wxRichTextRange(from, to-1));
}

void wxRichTextCtrl::DoSetValue(const wxString& value, int flags)
{
    // Don't call Clear here, since it always sends a text updated event
    m_buffer.ResetAndClearCommands();
    m_buffer.SetDirty(true);
    m_caretPosition = -1;
    m_caretPositionForDefaultStyle = -2;
    m_caretAtLineStart = false;
    m_selectionRange.SetRange(-2, -2);

    Scroll(0,0);

    if (m_freezeCount == 0)
    {
        LayoutContent();
        Refresh(false);
    }

    if (!value.IsEmpty())
    {
        // Remove empty paragraph
        GetBuffer().Clear();
        DoWriteText(value, flags);

        // for compatibility, don't move the cursor when doing SetValue()
        SetInsertionPoint(0);
    }
    else
    {
        // still send an event for consistency
        if (flags & SetValue_SendEvent)
            SendTextUpdatedEvent();
    }
    DiscardEdits();
}

void wxRichTextCtrl::WriteText(const wxString& value)
{
    DoWriteText(value);
}

void wxRichTextCtrl::DoWriteText(const wxString& value, int flags)
{
    wxString valueUnix = wxTextFile::Translate(value, wxTextFileType_Unix);

    GetBuffer().InsertTextWithUndo(m_caretPosition+1, valueUnix, this, wxRICHTEXT_INSERT_WITH_PREVIOUS_PARAGRAPH_STYLE);

    if ( flags & SetValue_SendEvent )
        SendTextUpdatedEvent();
}

void wxRichTextCtrl::AppendText(const wxString& text)
{
    SetInsertionPointEnd();

    WriteText(text);
}

/// Write an image at the current insertion point
bool wxRichTextCtrl::WriteImage(const wxImage& image, int bitmapType)
{
    wxRichTextImageBlock imageBlock;

    wxImage image2 = image;
    if (imageBlock.MakeImageBlock(image2, bitmapType))
        return WriteImage(imageBlock);

    return false;
}

bool wxRichTextCtrl::WriteImage(const wxString& filename, int bitmapType)
{
    wxRichTextImageBlock imageBlock;

    wxImage image;
    if (imageBlock.MakeImageBlock(filename, bitmapType, image, false))
        return WriteImage(imageBlock);

    return false;
}

bool wxRichTextCtrl::WriteImage(const wxRichTextImageBlock& imageBlock)
{
    return GetBuffer().InsertImageWithUndo(m_caretPosition+1, imageBlock, this);
}

bool wxRichTextCtrl::WriteImage(const wxBitmap& bitmap, int bitmapType)
{
    if (bitmap.Ok())
    {
        wxRichTextImageBlock imageBlock;

        wxImage image = bitmap.ConvertToImage();
        if (image.Ok() && imageBlock.MakeImageBlock(image, bitmapType))
            return WriteImage(imageBlock);
    }

    return false;
}

/// Insert a newline (actually paragraph) at the current insertion point.
bool wxRichTextCtrl::Newline()
{
    return GetBuffer().InsertNewlineWithUndo(m_caretPosition+1, this, wxRICHTEXT_INSERT_WITH_PREVIOUS_PARAGRAPH_STYLE);
}

/// Insert a line break at the current insertion point.
bool wxRichTextCtrl::LineBreak()
{
    wxString text;
    text = wxRichTextLineBreakChar;
    return GetBuffer().InsertTextWithUndo(m_caretPosition+1, text, this);
}

// ----------------------------------------------------------------------------
// Clipboard operations
// ----------------------------------------------------------------------------

void wxRichTextCtrl::Copy()
{
    if (CanCopy())
    {
        wxRichTextRange range = GetInternalSelectionRange();
        GetBuffer().CopyToClipboard(range);
    }
}

void wxRichTextCtrl::Cut()
{
    if (CanCut())
    {
        wxRichTextRange range = GetInternalSelectionRange();
        GetBuffer().CopyToClipboard(range);

        DeleteSelectedContent();
        LayoutContent();
        Refresh(false);
    }
}

void wxRichTextCtrl::Paste()
{
    if (CanPaste())
    {
        BeginBatchUndo(_("Paste"));

        long newPos = m_caretPosition;
        DeleteSelectedContent(& newPos);

        GetBuffer().PasteFromClipboard(newPos);

        EndBatchUndo();
    }
}

void wxRichTextCtrl::DeleteSelection()
{
    if (CanDeleteSelection())
    {
        DeleteSelectedContent();
    }
}

bool wxRichTextCtrl::HasSelection() const
{
    return m_selectionRange.GetStart() != -2 && m_selectionRange.GetEnd() != -2;
}

bool wxRichTextCtrl::CanCopy() const
{
    // Can copy if there's a selection
    return HasSelection();
}

bool wxRichTextCtrl::CanCut() const
{
    return HasSelection() && IsEditable();
}

bool wxRichTextCtrl::CanPaste() const
{
    if ( !IsEditable() )
        return false;

    return GetBuffer().CanPasteFromClipboard();
}

bool wxRichTextCtrl::CanDeleteSelection() const
{
    return HasSelection() && IsEditable();
}


// ----------------------------------------------------------------------------
// Accessors
// ----------------------------------------------------------------------------

void wxRichTextCtrl::SetContextMenu(wxMenu* menu)
{
    if (m_contextMenu && m_contextMenu != menu)
        delete m_contextMenu;
    m_contextMenu = menu;
}

void wxRichTextCtrl::SetEditable(bool editable)
{
    m_editable = editable;
}

void wxRichTextCtrl::SetInsertionPoint(long pos)
{
    SelectNone();

    m_caretPosition = pos - 1;

    PositionCaret();

    SetDefaultStyleToCursorStyle();
}

void wxRichTextCtrl::SetInsertionPointEnd()
{
    long pos = GetLastPosition();
    SetInsertionPoint(pos);
}

long wxRichTextCtrl::GetInsertionPoint() const
{
    return m_caretPosition+1;
}

wxTextPos wxRichTextCtrl::GetLastPosition() const
{
    return GetBuffer().GetRange().GetEnd();
}

// If the return values from and to are the same, there is no
// selection.
void wxRichTextCtrl::GetSelection(long* from, long* to) const
{
    *from = m_selectionRange.GetStart();
    *to = m_selectionRange.GetEnd();
    if ((*to) != -1 && (*to) != -2)
        (*to) ++;
}

bool wxRichTextCtrl::IsEditable() const
{
    return m_editable;
}

// ----------------------------------------------------------------------------
// selection
// ----------------------------------------------------------------------------

void wxRichTextCtrl::SetSelection(long from, long to)
{
    // if from and to are both -1, it means (in wxWidgets) that all text should
    // be selected.
    if ( (from == -1) && (to == -1) )
    {
        from = 0;
        to = GetLastPosition()+1;
    }

    DoSetSelection(from, to);
}

void wxRichTextCtrl::DoSetSelection(long from, long to, bool WXUNUSED(scrollCaret))
{
    if (from == to)
    {
        SelectNone();
    }
    else
    {
        wxRichTextRange oldSelection = m_selectionRange;
        m_selectionAnchor = from-1;
        m_selectionRange.SetRange(from, to-1);

        m_caretPosition = wxMax(-1, to-1);

        wxRichTextCtrlRefreshForSelectionChange(*this, oldSelection, m_selectionRange);
        PositionCaret();
    }
}

// ----------------------------------------------------------------------------
// Editing
// ----------------------------------------------------------------------------

void wxRichTextCtrl::Replace(long WXUNUSED(from), long WXUNUSED(to),
                             const wxString& value)
{
    BeginBatchUndo(_("Replace"));

    DeleteSelectedContent();

    DoWriteText(value, SetValue_SelectionOnly);

    EndBatchUndo();
}

void wxRichTextCtrl::Remove(long from, long to)
{
    SelectNone();

    GetBuffer().DeleteRangeWithUndo(wxRichTextRange(from, to-1), this);

    LayoutContent();
    if (!IsFrozen())
        Refresh(false);
}

bool wxRichTextCtrl::IsModified() const
{
    return m_buffer.IsModified();
}

void wxRichTextCtrl::MarkDirty()
{
    m_buffer.Modify(true);
}

void wxRichTextCtrl::DiscardEdits()
{
    m_caretPositionForDefaultStyle = -2;
    m_buffer.Modify(false);
    m_buffer.GetCommandProcessor()->ClearCommands();
}

int wxRichTextCtrl::GetNumberOfLines() const
{
    return GetBuffer().GetParagraphCount();
}

// ----------------------------------------------------------------------------
// Positions <-> coords
// ----------------------------------------------------------------------------

long wxRichTextCtrl::XYToPosition(long x, long y) const
{
    return GetBuffer().XYToPosition(x, y);
}

bool wxRichTextCtrl::PositionToXY(long pos, long *x, long *y) const
{
    return GetBuffer().PositionToXY(pos, x, y);
}

// ----------------------------------------------------------------------------
//
// ----------------------------------------------------------------------------

void wxRichTextCtrl::ShowPosition(long pos)
{
    if (!IsPositionVisible(pos))
        ScrollIntoView(pos-1, WXK_DOWN);
}

int wxRichTextCtrl::GetLineLength(long lineNo) const
{
    return GetBuffer().GetParagraphLength(lineNo);
}

wxString wxRichTextCtrl::GetLineText(long lineNo) const
{
    return GetBuffer().GetParagraphText(lineNo);
}

// ----------------------------------------------------------------------------
// Undo/redo
// ----------------------------------------------------------------------------

void wxRichTextCtrl::Undo()
{
    if (CanUndo())
    {
        GetCommandProcessor()->Undo();
    }
}

void wxRichTextCtrl::Redo()
{
    if (CanRedo())
    {
        GetCommandProcessor()->Redo();
    }
}

bool wxRichTextCtrl::CanUndo() const
{
    return GetCommandProcessor()->CanUndo() && IsEditable();
}

bool wxRichTextCtrl::CanRedo() const
{
    return GetCommandProcessor()->CanRedo() && IsEditable();
}

// ----------------------------------------------------------------------------
// implementation details
// ----------------------------------------------------------------------------

void wxRichTextCtrl::Command(wxCommandEvent& event)
{
    SetValue(event.GetString());
    GetEventHandler()->ProcessEvent(event);
}

void wxRichTextCtrl::OnDropFiles(wxDropFilesEvent& event)
{
    // By default, load the first file into the text window.
    if (event.GetNumberOfFiles() > 0)
    {
        LoadFile(event.GetFiles()[0]);
    }
}

wxSize wxRichTextCtrl::DoGetBestSize() const
{
    return wxSize(10, 10);
}

// ----------------------------------------------------------------------------
// standard handlers for standard edit menu events
// ----------------------------------------------------------------------------

void wxRichTextCtrl::OnCut(wxCommandEvent& WXUNUSED(event))
{
    Cut();
}

void wxRichTextCtrl::OnClear(wxCommandEvent& WXUNUSED(event))
{
    DeleteSelection();
}

void wxRichTextCtrl::OnCopy(wxCommandEvent& WXUNUSED(event))
{
    Copy();
}

void wxRichTextCtrl::OnPaste(wxCommandEvent& WXUNUSED(event))
{
    Paste();
}

void wxRichTextCtrl::OnUndo(wxCommandEvent& WXUNUSED(event))
{
    Undo();
}

void wxRichTextCtrl::OnRedo(wxCommandEvent& WXUNUSED(event))
{
    Redo();
}

void wxRichTextCtrl::OnUpdateCut(wxUpdateUIEvent& event)
{
    event.Enable( CanCut() );
}

void wxRichTextCtrl::OnUpdateCopy(wxUpdateUIEvent& event)
{
    event.Enable( CanCopy() );
}

void wxRichTextCtrl::OnUpdateClear(wxUpdateUIEvent& event)
{
    event.Enable( CanDeleteSelection() );
}

void wxRichTextCtrl::OnUpdatePaste(wxUpdateUIEvent& event)
{
    event.Enable( CanPaste() );
}

void wxRichTextCtrl::OnUpdateUndo(wxUpdateUIEvent& event)
{
    event.Enable( CanUndo() );
    event.SetText( GetCommandProcessor()->GetUndoMenuLabel() );
}

void wxRichTextCtrl::OnUpdateRedo(wxUpdateUIEvent& event)
{
    event.Enable( CanRedo() );
    event.SetText( GetCommandProcessor()->GetRedoMenuLabel() );
}

void wxRichTextCtrl::OnSelectAll(wxCommandEvent& WXUNUSED(event))
{
    if (GetLastPosition() > 0)
        SelectAll();
}

void wxRichTextCtrl::OnUpdateSelectAll(wxUpdateUIEvent& event)
{
    event.Enable(GetLastPosition() > 0);
}

void wxRichTextCtrl::OnContextMenu(wxContextMenuEvent& event)
{
    if (event.GetEventObject() != this)
    {
        event.Skip();
        return;
    }

    if (m_contextMenu)
        PopupMenu(m_contextMenu);
    return;
}

bool wxRichTextCtrl::SetStyle(long start, long end, const wxTextAttrEx& style)
{
    return GetBuffer().SetStyle(wxRichTextRange(start, end-1), style);
}

bool wxRichTextCtrl::SetStyle(long start, long end, const wxTextAttr& style)
{
    return GetBuffer().SetStyle(wxRichTextRange(start, end-1), wxTextAttrEx(style));
}

bool wxRichTextCtrl::SetStyle(const wxRichTextRange& range, const wxRichTextAttr& style)
{
    return GetBuffer().SetStyle(range.ToInternal(), style);
}

// extended style setting operation with flags including:
// wxRICHTEXT_SETSTYLE_WITH_UNDO, wxRICHTEXT_SETSTYLE_OPTIMIZE, wxRICHTEXT_SETSTYLE_PARAGRAPHS_ONLY.
// see richtextbuffer.h for more details.
bool wxRichTextCtrl::SetStyleEx(long start, long end, const wxTextAttrEx& style, int flags)
{
    return GetBuffer().SetStyle(wxRichTextRange(start, end-1), style, flags);
}

bool wxRichTextCtrl::SetStyleEx(const wxRichTextRange& range, const wxTextAttrEx& style, int flags)
{
    return GetBuffer().SetStyle(range.ToInternal(), style, flags);
}

bool wxRichTextCtrl::SetStyleEx(const wxRichTextRange& range, const wxRichTextAttr& style, int flags)
{
    return GetBuffer().SetStyle(range.ToInternal(), style, flags);
}

bool wxRichTextCtrl::SetDefaultStyle(const wxTextAttrEx& style)
{
    return GetBuffer().SetDefaultStyle(style);
}

bool wxRichTextCtrl::SetDefaultStyle(const wxTextAttr& style)
{
    return GetBuffer().SetDefaultStyle(wxTextAttrEx(style));
}

const wxTextAttrEx& wxRichTextCtrl::GetDefaultStyleEx() const
{
    return GetBuffer().GetDefaultStyle();
}

const wxTextAttr& wxRichTextCtrl::GetDefaultStyle() const
{
    return GetBuffer().GetDefaultStyle();
}

bool wxRichTextCtrl::GetStyle(long position, wxTextAttr& style)
{
    wxTextAttrEx attr(style);
    if (GetBuffer().GetStyle(position, attr))
    {
        style = attr;
        return true;
    }
    else
        return false;
}

bool wxRichTextCtrl::GetStyle(long position, wxTextAttrEx& style)
{
    return GetBuffer().GetStyle(position, style);
}

bool wxRichTextCtrl::GetStyle(long position, wxRichTextAttr& style)
{
    return GetBuffer().GetStyle(position, style);
}

// get the common set of styles for the range
bool wxRichTextCtrl::GetStyleForRange(const wxRichTextRange& range, wxRichTextAttr& style)
{
    wxTextAttrEx styleEx;
    if (GetBuffer().GetStyleForRange(range.ToInternal(), styleEx))
    {
        style = styleEx;
        return true;
    }
    else
        return false;
}

bool wxRichTextCtrl::GetStyleForRange(const wxRichTextRange& range, wxTextAttrEx& style)
{
    return GetBuffer().GetStyleForRange(range.ToInternal(), style);
}

/// Get the content (uncombined) attributes for this position.

bool wxRichTextCtrl::GetUncombinedStyle(long position, wxTextAttr& style)
{
    wxTextAttrEx attr(style);
    if (GetBuffer().GetUncombinedStyle(position, attr))
    {
        style = attr;
        return true;
    }
    else
        return false;
}

bool wxRichTextCtrl::GetUncombinedStyle(long position, wxTextAttrEx& style)
{
    return GetBuffer().GetUncombinedStyle(position, style);
}

bool wxRichTextCtrl::GetUncombinedStyle(long position, wxRichTextAttr& style)
{
    return GetBuffer().GetUncombinedStyle(position, style);
}

/// Set font, and also the buffer attributes
bool wxRichTextCtrl::SetFont(const wxFont& font)
{
    wxControl::SetFont(font);

    wxTextAttrEx attr = GetBuffer().GetAttributes();
    attr.SetFont(font);
    GetBuffer().SetBasicStyle(attr);

    GetBuffer().Invalidate(wxRICHTEXT_ALL);
    Refresh(false);

    return true;
}

/// Transform logical to physical
wxPoint wxRichTextCtrl::GetPhysicalPoint(const wxPoint& ptLogical) const
{
    wxPoint pt;
    CalcScrolledPosition(ptLogical.x, ptLogical.y, & pt.x, & pt.y);

    return pt;
}

/// Transform physical to logical
wxPoint wxRichTextCtrl::GetLogicalPoint(const wxPoint& ptPhysical) const
{
    wxPoint pt;
    CalcUnscrolledPosition(ptPhysical.x, ptPhysical.y, & pt.x, & pt.y);

    return pt;
}

/// Position the caret
void wxRichTextCtrl::PositionCaret()
{
    if (!GetCaret())
        return;

    wxRect caretRect;
    if (GetCaretPositionForIndex(GetCaretPosition(), caretRect))
    {
        wxPoint newPt = caretRect.GetPosition();
        wxSize newSz = caretRect.GetSize();
        wxPoint pt = GetPhysicalPoint(newPt);
        if (GetCaret()->GetPosition() != pt || GetCaret()->GetSize() != newSz)
        {
            //wxLogDebug(wxT("Positioning caret %d, %d"), pt.x, pt.y);
            GetCaret()->Hide();
            if (GetCaret()->GetSize() != newSz)
                GetCaret()->SetSize(newSz);

            int halfSize = newSz.y/2;
            // If the caret is beyond the margin, hide it by moving it out of the way
            if (((pt.y + halfSize) < GetBuffer().GetTopMargin()) || ((pt.y + halfSize) > (GetClientSize().y - GetBuffer().GetBottomMargin())))
                pt.y = -200;

            GetCaret()->Move(pt);
            GetCaret()->Show();
        }
    }
}

/// Get the caret height and position for the given character position
bool wxRichTextCtrl::GetCaretPositionForIndex(long position, wxRect& rect)
{
    wxClientDC dc(this);
    dc.SetFont(GetFont());

    PrepareDC(dc);

    wxPoint pt;
    int height = 0;

    if (GetBuffer().FindPosition(dc, position, pt, & height, m_caretAtLineStart))
    {
        // Caret height can't be zero
        if (height == 0)
            height = dc.GetCharHeight();

        rect = wxRect(pt, wxSize(wxRICHTEXT_DEFAULT_CARET_WIDTH, height));
        return true;
    }

    return false;
}

/// Gets the line for the visible caret position. If the caret is
/// shown at the very end of the line, it means the next character is actually
/// on the following line. So let's get the line we're expecting to find
/// if this is the case.
wxRichTextLine* wxRichTextCtrl::GetVisibleLineForCaretPosition(long caretPosition) const
{
    wxRichTextLine* line = GetBuffer().GetLineAtPosition(caretPosition, true);
    wxRichTextParagraph* para = GetBuffer().GetParagraphAtPosition(caretPosition, true);
    if (line)
    {
        wxRichTextRange lineRange = line->GetAbsoluteRange();
        if (caretPosition == lineRange.GetStart()-1 &&
            (para->GetRange().GetStart() != lineRange.GetStart()))
        {
            if (!m_caretAtLineStart)
                line = GetBuffer().GetLineAtPosition(caretPosition-1, true);
        }
    }
    return line;
}


/// Move the caret to the given character position
bool wxRichTextCtrl::MoveCaret(long pos, bool showAtLineStart)
{
    if (GetBuffer().GetDirty())
        LayoutContent();

    if (pos <= GetBuffer().GetRange().GetEnd())
    {
        SetCaretPosition(pos, showAtLineStart);

        PositionCaret();

        return true;
    }
    else
        return false;
}

/// Layout the buffer: which we must do before certain operations, such as
/// setting the caret position.
bool wxRichTextCtrl::LayoutContent(bool onlyVisibleRect)
{
    if (GetBuffer().GetDirty() || onlyVisibleRect)
    {
        wxRect availableSpace(GetClientSize());
        if (availableSpace.width == 0)
            availableSpace.width = 10;
        if (availableSpace.height == 0)
            availableSpace.height = 10;

        int flags = wxRICHTEXT_FIXED_WIDTH|wxRICHTEXT_VARIABLE_HEIGHT;
        if (onlyVisibleRect)
        {
            flags |= wxRICHTEXT_LAYOUT_SPECIFIED_RECT;
            availableSpace.SetPosition(GetLogicalPoint(wxPoint(0, 0)));
        }

        wxClientDC dc(this);
        dc.SetFont(GetFont());

        PrepareDC(dc);

        GetBuffer().Defragment();
        GetBuffer().UpdateRanges();     // If items were deleted, ranges need recalculation
        GetBuffer().Layout(dc, availableSpace, flags);
        GetBuffer().SetDirty(false);

        if (!IsFrozen())
            SetupScrollbars();
    }

    return true;
}

/// Is all of the selection bold?
bool wxRichTextCtrl::IsSelectionBold()
{
    if (HasSelection())
    {
        wxRichTextAttr attr;
        wxRichTextRange range = GetSelectionRange();
        attr.SetFlags(wxTEXT_ATTR_FONT_WEIGHT);
        attr.SetFontWeight(wxBOLD);

        return HasCharacterAttributes(range, attr);
    }
    else
    {
        // If no selection, then we need to combine current style with default style
        // to see what the effect would be if we started typing.
        wxRichTextAttr attr;
        attr.SetFlags(wxTEXT_ATTR_FONT_WEIGHT);

        long pos = GetAdjustedCaretPosition(GetCaretPosition());
        if (GetStyle(pos, attr))
        {
            if (IsDefaultStyleShowing())
                wxRichTextApplyStyle(attr, GetDefaultStyleEx());
            return attr.GetFontWeight() == wxBOLD;
        }
    }
    return false;
}

/// Is all of the selection italics?
bool wxRichTextCtrl::IsSelectionItalics()
{
    if (HasSelection())
    {
        wxRichTextRange range = GetSelectionRange();
        wxRichTextAttr attr;
        attr.SetFlags(wxTEXT_ATTR_FONT_ITALIC);
        attr.SetFontStyle(wxITALIC);

        return HasCharacterAttributes(range, attr);
    }
    else
    {
        // If no selection, then we need to combine current style with default style
        // to see what the effect would be if we started typing.
        wxRichTextAttr attr;
        attr.SetFlags(wxTEXT_ATTR_FONT_ITALIC);

        long pos = GetAdjustedCaretPosition(GetCaretPosition());
        if (GetStyle(pos, attr))
        {
            if (IsDefaultStyleShowing())
                wxRichTextApplyStyle(attr, GetDefaultStyleEx());
            return attr.GetFontStyle() == wxITALIC;
        }
    }
    return false;
}

/// Is all of the selection underlined?
bool wxRichTextCtrl::IsSelectionUnderlined()
{
    if (HasSelection())
    {
        wxRichTextRange range = GetSelectionRange();
        wxRichTextAttr attr;
        attr.SetFlags(wxTEXT_ATTR_FONT_UNDERLINE);
        attr.SetFontUnderlined(true);

        return HasCharacterAttributes(range, attr);
    }
    else
    {
        // If no selection, then we need to combine current style with default style
        // to see what the effect would be if we started typing.
        wxRichTextAttr attr;
        attr.SetFlags(wxTEXT_ATTR_FONT_UNDERLINE);
        long pos = GetAdjustedCaretPosition(GetCaretPosition());

        if (GetStyle(pos, attr))
        {
            if (IsDefaultStyleShowing())
                wxRichTextApplyStyle(attr, GetDefaultStyleEx());
            return attr.GetFontUnderlined();
        }
    }
    return false;
}

/// Apply bold to the selection
bool wxRichTextCtrl::ApplyBoldToSelection()
{
    wxRichTextAttr attr;
    attr.SetFlags(wxTEXT_ATTR_FONT_WEIGHT);
    attr.SetFontWeight(IsSelectionBold() ? wxNORMAL : wxBOLD);

    if (HasSelection())
        return SetStyleEx(GetSelectionRange(), attr, wxRICHTEXT_SETSTYLE_WITH_UNDO|wxRICHTEXT_SETSTYLE_OPTIMIZE|wxRICHTEXT_SETSTYLE_CHARACTERS_ONLY);
    else
    {
        wxRichTextAttr current = GetDefaultStyleEx();
        current.Apply(attr);
        SetAndShowDefaultStyle(current);
    }
    return true;
}

/// Apply italic to the selection
bool wxRichTextCtrl::ApplyItalicToSelection()
{
    wxRichTextAttr attr;
    attr.SetFlags(wxTEXT_ATTR_FONT_ITALIC);
    attr.SetFontStyle(IsSelectionItalics() ? wxNORMAL : wxITALIC);

    if (HasSelection())
        return SetStyleEx(GetSelectionRange(), attr, wxRICHTEXT_SETSTYLE_WITH_UNDO|wxRICHTEXT_SETSTYLE_OPTIMIZE|wxRICHTEXT_SETSTYLE_CHARACTERS_ONLY);
    else
    {
        wxRichTextAttr current = GetDefaultStyleEx();
        current.Apply(attr);
        SetAndShowDefaultStyle(current);
    }
    return true;
}

/// Apply underline to the selection
bool wxRichTextCtrl::ApplyUnderlineToSelection()
{
    wxRichTextAttr attr;
    attr.SetFlags(wxTEXT_ATTR_FONT_UNDERLINE);
    attr.SetFontUnderlined(!IsSelectionUnderlined());

    if (HasSelection())
        return SetStyleEx(GetSelectionRange(), attr, wxRICHTEXT_SETSTYLE_WITH_UNDO|wxRICHTEXT_SETSTYLE_OPTIMIZE|wxRICHTEXT_SETSTYLE_CHARACTERS_ONLY);
    else
    {
        wxRichTextAttr current = GetDefaultStyleEx();
        current.Apply(attr);
        SetAndShowDefaultStyle(current);
    }
    return true;
}

/// Is all of the selection aligned according to the specified flag?
bool wxRichTextCtrl::IsSelectionAligned(wxTextAttrAlignment alignment)
{
    wxRichTextRange range;
    if (HasSelection())
        range = GetSelectionRange();
    else
        range = wxRichTextRange(GetCaretPosition()+1, GetCaretPosition()+2);

    wxRichTextAttr attr;
    attr.SetAlignment(alignment);

    return HasParagraphAttributes(range, attr);
}

/// Apply alignment to the selection
bool wxRichTextCtrl::ApplyAlignmentToSelection(wxTextAttrAlignment alignment)
{
    wxRichTextAttr attr;
    attr.SetAlignment(alignment);
    if (HasSelection())
        return SetStyle(GetSelectionRange(), attr);
    else
    {
        wxRichTextParagraph* para = GetBuffer().GetParagraphAtPosition(GetCaretPosition()+1);
        if (para)
            return SetStyleEx(para->GetRange().FromInternal(), attr, wxRICHTEXT_SETSTYLE_WITH_UNDO|wxRICHTEXT_SETSTYLE_OPTIMIZE|wxRICHTEXT_SETSTYLE_PARAGRAPHS_ONLY);
    }
    return true;
}

/// Apply a named style to the selection
bool wxRichTextCtrl::ApplyStyle(wxRichTextStyleDefinition* def)
{
    // Flags are defined within each definition, so only certain
    // attributes are applied.
    wxRichTextAttr attr(GetStyleSheet() ? def->GetStyleMergedWithBase(GetStyleSheet()) : def->GetStyle());

    int flags = wxRICHTEXT_SETSTYLE_WITH_UNDO|wxRICHTEXT_SETSTYLE_OPTIMIZE|wxRICHTEXT_SETSTYLE_RESET;

    if (def->IsKindOf(CLASSINFO(wxRichTextListStyleDefinition)))
    {
        flags |= wxRICHTEXT_SETSTYLE_PARAGRAPHS_ONLY;

        wxRichTextRange range;

        if (HasSelection())
            range = GetSelectionRange();
        else
        {
            long pos = GetAdjustedCaretPosition(GetCaretPosition());
            range = wxRichTextRange(pos, pos+1);
        }

        return SetListStyle(range, (wxRichTextListStyleDefinition*) def, flags);
    }

    bool isPara = false;

    // Make sure the attr has the style name
    if (def->IsKindOf(CLASSINFO(wxRichTextParagraphStyleDefinition)))
    {
        isPara = true;
        attr.SetParagraphStyleName(def->GetName());

        // If applying a paragraph style, we only want the paragraph nodes to adopt these
        // attributes, and not the leaf nodes. This will allow the content (e.g. text)
        // to change its style independently.
        flags |= wxRICHTEXT_SETSTYLE_PARAGRAPHS_ONLY;
    }
    else
        attr.SetCharacterStyleName(def->GetName());

    if (HasSelection())
        return SetStyleEx(GetSelectionRange(), attr, flags);
    else
    {
        wxRichTextAttr current = GetDefaultStyleEx();
        wxRichTextAttr defaultStyle(attr);
        if (isPara)
        {
            // Don't apply extra character styles since they are already implied
            // in the paragraph style
            defaultStyle.SetFlags(defaultStyle.GetFlags() & ~wxTEXT_ATTR_CHARACTER);
        }
        current.Apply(defaultStyle);
        SetAndShowDefaultStyle(current);

        // If it's a paragraph style, we want to apply the style to the
        // current paragraph even if we didn't select any text.
        if (isPara)
        {
            long pos = GetAdjustedCaretPosition(GetCaretPosition());
            wxRichTextParagraph* para = GetBuffer().GetParagraphAtPosition(pos);
            if (para)
            {
                return SetStyleEx(para->GetRange().FromInternal(), attr, flags);
            }
        }
        return true;
    }
}

/// Apply the style sheet to the buffer, for example if the styles have changed.
bool wxRichTextCtrl::ApplyStyleSheet(wxRichTextStyleSheet* styleSheet)
{
    if (!styleSheet)
        styleSheet = GetBuffer().GetStyleSheet();
    if (!styleSheet)
        return false;

    if (GetBuffer().ApplyStyleSheet(styleSheet))
    {
        GetBuffer().Invalidate(wxRICHTEXT_ALL);
        Refresh(false);
        return true;
    }
    else
        return false;
}

/// Sets the default style to the style under the cursor
bool wxRichTextCtrl::SetDefaultStyleToCursorStyle()
{
    wxTextAttrEx attr;
    attr.SetFlags(wxTEXT_ATTR_CHARACTER);

    // If at the start of a paragraph, use the next position.
    long pos = GetAdjustedCaretPosition(GetCaretPosition());

    if (GetUncombinedStyle(pos, attr))
    {
        SetDefaultStyle(attr);
        return true;
    }

    return false;
}

/// Returns the first visible position in the current view
long wxRichTextCtrl::GetFirstVisiblePosition() const
{
    wxRichTextLine* line = GetBuffer().GetLineAtYPosition(GetLogicalPoint(wxPoint(0, 0)).y);
    if (line)
        return line->GetAbsoluteRange().GetStart();
    else
        return 0;
}

/// Get the first visible point in the window
wxPoint wxRichTextCtrl::GetFirstVisiblePoint() const
{
    int ppuX, ppuY;
    int startXUnits, startYUnits;

    GetScrollPixelsPerUnit(& ppuX, & ppuY);
    GetViewStart(& startXUnits, & startYUnits);

    return wxPoint(startXUnits * ppuX, startYUnits * ppuY);
}

/// The adjusted caret position is the character position adjusted to take
/// into account whether we're at the start of a paragraph, in which case
/// style information should be taken from the next position, not current one.
long wxRichTextCtrl::GetAdjustedCaretPosition(long caretPos) const
{
    wxRichTextParagraph* para = GetBuffer().GetParagraphAtPosition(caretPos+1);

    if (para && (caretPos+1 == para->GetRange().GetStart()))
        caretPos ++;
    return caretPos;
}

/// Get/set the selection range in character positions. -1, -1 means no selection.
/// The range is in API convention, i.e. a single character selection is denoted
/// by (n, n+1)
wxRichTextRange wxRichTextCtrl::GetSelectionRange() const
{
    wxRichTextRange range = GetInternalSelectionRange();
    if (range != wxRichTextRange(-2,-2) && range != wxRichTextRange(-1,-1))
        range.SetEnd(range.GetEnd() + 1);
    return range;
}

void wxRichTextCtrl::SetSelectionRange(const wxRichTextRange& range)
{
    SetSelection(range.GetStart(), range.GetEnd());
}

/// Set list style
bool wxRichTextCtrl::SetListStyle(const wxRichTextRange& range, wxRichTextListStyleDefinition* def, int flags, int startFrom, int specifiedLevel)
{
    return GetBuffer().SetListStyle(range.ToInternal(), def, flags, startFrom, specifiedLevel);
}

bool wxRichTextCtrl::SetListStyle(const wxRichTextRange& range, const wxString& defName, int flags, int startFrom, int specifiedLevel)
{
    return GetBuffer().SetListStyle(range.ToInternal(), defName, flags, startFrom, specifiedLevel);
}

/// Clear list for given range
bool wxRichTextCtrl::ClearListStyle(const wxRichTextRange& range, int flags)
{
    return GetBuffer().ClearListStyle(range.ToInternal(), flags);
}

/// Number/renumber any list elements in the given range
bool wxRichTextCtrl::NumberList(const wxRichTextRange& range, wxRichTextListStyleDefinition* def, int flags, int startFrom, int specifiedLevel)
{
    return GetBuffer().NumberList(range.ToInternal(), def, flags, startFrom, specifiedLevel);
}

bool wxRichTextCtrl::NumberList(const wxRichTextRange& range, const wxString& defName, int flags, int startFrom, int specifiedLevel)
{
    return GetBuffer().NumberList(range.ToInternal(), defName, flags, startFrom, specifiedLevel);
}

/// Promote the list items within the given range. promoteBy can be a positive or negative number, e.g. 1 or -1
bool wxRichTextCtrl::PromoteList(int promoteBy, const wxRichTextRange& range, wxRichTextListStyleDefinition* def, int flags, int specifiedLevel)
{
    return GetBuffer().PromoteList(promoteBy, range.ToInternal(), def, flags, specifiedLevel);
}

bool wxRichTextCtrl::PromoteList(int promoteBy, const wxRichTextRange& range, const wxString& defName, int flags, int specifiedLevel)
{
    return GetBuffer().PromoteList(promoteBy, range.ToInternal(), defName, flags, specifiedLevel);
}

/// Deletes the content in the given range
bool wxRichTextCtrl::Delete(const wxRichTextRange& range)
{
    return GetBuffer().DeleteRangeWithUndo(range.ToInternal(), this);
}

const wxArrayString& wxRichTextCtrl::GetAvailableFontNames()
{
    if (sm_availableFontNames.GetCount() == 0)
    {
        sm_availableFontNames = wxFontEnumerator::GetFacenames();
        sm_availableFontNames.Sort();
    }
    return sm_availableFontNames;
}

void wxRichTextCtrl::ClearAvailableFontNames()
{
    sm_availableFontNames.Clear();
}

// Refresh the area affected by a selection change
bool wxRichTextCtrlRefreshForSelectionChange(wxRichTextCtrl& ctrl, const wxRichTextRange& oldSelection, const wxRichTextRange& newSelection)
{
    // Calculate the refresh rectangle - just the affected lines
    long firstPos, lastPos;
    if (oldSelection.GetStart() == -2 && newSelection.GetStart() != -2)
    {
        firstPos = newSelection.GetStart();
        lastPos = newSelection.GetEnd();
    }
    else if (oldSelection.GetStart() != -2 && newSelection.GetStart() == -2)
    {
        firstPos = oldSelection.GetStart();
        lastPos = oldSelection.GetEnd();
    }
    else if (oldSelection.GetStart() == -2 && newSelection.GetStart() == -2)
    {
        return false;
    }
    else
    {
        firstPos = wxMin(oldSelection.GetStart(), newSelection.GetStart());
        lastPos = wxMax(oldSelection.GetEnd(), newSelection.GetEnd());
    }

    wxRichTextLine* firstLine = ctrl.GetBuffer().GetLineAtPosition(firstPos);
    wxRichTextLine* lastLine = ctrl.GetBuffer().GetLineAtPosition(lastPos);

    if (firstLine && lastLine)
    {
        wxSize clientSize = ctrl.GetClientSize();
        wxPoint pt1 = ctrl.GetPhysicalPoint(firstLine->GetAbsolutePosition());
        wxPoint pt2 = ctrl.GetPhysicalPoint(lastLine->GetAbsolutePosition()) + wxPoint(0, lastLine->GetSize().y);

        pt1.x = 0;
        pt1.y = wxMax(0, pt1.y);
        pt2.x = 0;
        pt2.y = wxMin(clientSize.y, pt2.y);

        wxRect rect(pt1, wxSize(clientSize.x, pt2.y - pt1.y));
        ctrl.RefreshRect(rect, false);
    }
    else
        ctrl.Refresh(false);

    return true;
}

#if wxRICHTEXT_USE_OWN_CARET

// ----------------------------------------------------------------------------
// initialization and destruction
// ----------------------------------------------------------------------------

void wxRichTextCaret::Init()
{
    m_hasFocus = true;

    m_xOld =
    m_yOld = -1;
    m_richTextCtrl = NULL;
    m_needsUpdate = false;
    m_flashOn = true;
}

wxRichTextCaret::~wxRichTextCaret()
{
    if (m_timer.IsRunning())
        m_timer.Stop();
}

// ----------------------------------------------------------------------------
// showing/hiding/moving the caret (base class interface)
// ----------------------------------------------------------------------------

void wxRichTextCaret::DoShow()
{
    m_flashOn = true;

    if (!m_timer.IsRunning())
        m_timer.Start(GetBlinkTime());

    Refresh();
}

void wxRichTextCaret::DoHide()
{
    if (m_timer.IsRunning())
        m_timer.Stop();

    Refresh();
}

void wxRichTextCaret::DoMove()
{
    if (IsVisible())
    {
        Refresh();

        if (m_xOld != -1 && m_yOld != -1)
        {
            if (m_richTextCtrl)
            {
                wxRect rect(GetPosition(), GetSize());
                m_richTextCtrl->RefreshRect(rect, false);
            }
        }
    }

    m_xOld = m_x;
    m_yOld = m_y;
}

void wxRichTextCaret::DoSize()
{
    int countVisible = m_countVisible;
    if (countVisible > 0)
    {
        m_countVisible = 0;
        DoHide();
    }

    if (countVisible > 0)
    {
        m_countVisible = countVisible;
        DoShow();
    }
}

// ----------------------------------------------------------------------------
// handling the focus
// ----------------------------------------------------------------------------

void wxRichTextCaret::OnSetFocus()
{
    m_hasFocus = true;

    if ( IsVisible() )
        Refresh();
}

void wxRichTextCaret::OnKillFocus()
{
    m_hasFocus = false;
}

// ----------------------------------------------------------------------------
// drawing the caret
// ----------------------------------------------------------------------------

void wxRichTextCaret::Refresh()
{
    if (m_richTextCtrl)
    {
        wxRect rect(GetPosition(), GetSize());
        m_richTextCtrl->RefreshRect(rect, false);
    }
}

void wxRichTextCaret::DoDraw(wxDC *dc)
{
    dc->SetPen( *wxBLACK_PEN );

    dc->SetBrush(*(m_hasFocus ? wxBLACK_BRUSH : wxTRANSPARENT_BRUSH));
    dc->SetPen(*wxBLACK_PEN);

    wxPoint pt(m_x, m_y);

    if (m_richTextCtrl)
    {
        pt = m_richTextCtrl->GetLogicalPoint(pt);
    }
    if (IsVisible() && m_flashOn)
        dc->DrawRectangle(pt.x, pt.y, m_width, m_height);
}

void wxRichTextCaret::Notify()
{
    m_flashOn = !m_flashOn;
    Refresh();
}

void wxRichTextCaretTimer::Notify()
{
    m_caret->Notify();
}

#endif
    // wxRICHTEXT_USE_OWN_CARET
#endif
    // wxUSE_RICHTEXT
