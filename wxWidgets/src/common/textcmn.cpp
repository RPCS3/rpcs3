///////////////////////////////////////////////////////////////////////////////
// Name:        src/common/textcmn.cpp
// Purpose:     implementation of platform-independent functions of wxTextCtrl
// Author:      Julian Smart
// Modified by:
// Created:     13.07.99
// RCS-ID:      $Id: textcmn.cpp 62095 2009-09-24 18:20:21Z JS $
// Copyright:   (c) wxWidgets team
// Licence:     wxWindows licence
///////////////////////////////////////////////////////////////////////////////

// ============================================================================
// declarations
// ============================================================================

// for compilers that support precompilation, includes "wx.h".
#include "wx/wxprec.h"

#ifdef __BORLANDC__
    #pragma hdrstop
#endif

#ifndef WX_PRECOMP
    #include "wx/event.h"
#endif // WX_PRECOMP

#if wxUSE_TEXTCTRL

#include "wx/textctrl.h"

#ifndef WX_PRECOMP
    #include "wx/intl.h"
    #include "wx/log.h"
#endif // WX_PRECOMP

#include "wx/ffile.h"

// ----------------------------------------------------------------------------
// macros
// ----------------------------------------------------------------------------

// we don't have any objects of type wxTextCtrlBase in the program, only
// wxTextCtrl, so this cast is safe
#define TEXTCTRL(ptr)   ((wxTextCtrl *)(ptr))

// ============================================================================
// implementation
// ============================================================================

IMPLEMENT_DYNAMIC_CLASS(wxTextUrlEvent, wxCommandEvent)

DEFINE_EVENT_TYPE(wxEVT_COMMAND_TEXT_UPDATED)
DEFINE_EVENT_TYPE(wxEVT_COMMAND_TEXT_ENTER)
DEFINE_EVENT_TYPE(wxEVT_COMMAND_TEXT_URL)
DEFINE_EVENT_TYPE(wxEVT_COMMAND_TEXT_MAXLEN)

IMPLEMENT_ABSTRACT_CLASS(wxTextCtrlBase, wxControl)

// ----------------------------------------------------------------------------
// style functions - not implemented here
// ----------------------------------------------------------------------------

wxTextAttr::wxTextAttr(const wxColour& colText,
               const wxColour& colBack,
               const wxFont& font,
               wxTextAttrAlignment alignment)
    : m_colText(colText), m_colBack(colBack), m_font(font), m_textAlignment(alignment)
{
    m_flags = 0;
    m_leftIndent = 0;
    m_leftSubIndent = 0;
    m_rightIndent = 0;
    if (m_colText.Ok()) m_flags |= wxTEXT_ATTR_TEXT_COLOUR;
    if (m_colBack.Ok()) m_flags |= wxTEXT_ATTR_BACKGROUND_COLOUR;
    if (m_font.Ok()) m_flags |= wxTEXT_ATTR_FONT;
    if (alignment != wxTEXT_ALIGNMENT_DEFAULT)
        m_flags |= wxTEXT_ATTR_ALIGNMENT;
}

void wxTextAttr::Init()
{
    m_textAlignment = wxTEXT_ALIGNMENT_DEFAULT;
    m_flags = 0;
    m_leftIndent = 0;
    m_leftSubIndent = 0;
    m_rightIndent = 0;
}

/* static */
wxTextAttr wxTextAttr::Combine(const wxTextAttr& attr,
                               const wxTextAttr& attrDef,
                               const wxTextCtrlBase *text)
{
    wxFont font = attr.GetFont();
    if ( !font.Ok() )
    {
        font = attrDef.GetFont();

        if ( text && !font.Ok() )
            font = text->GetFont();
    }

    wxColour colFg = attr.GetTextColour();
    if ( !colFg.Ok() )
    {
        colFg = attrDef.GetTextColour();

        if ( text && !colFg.Ok() )
            colFg = text->GetForegroundColour();
    }

    wxColour colBg = attr.GetBackgroundColour();
    if ( !colBg.Ok() )
    {
        colBg = attrDef.GetBackgroundColour();

        if ( text && !colBg.Ok() )
            colBg = text->GetBackgroundColour();
    }

    wxTextAttr newAttr(colFg, colBg, font);

    if (attr.HasAlignment())
        newAttr.SetAlignment(attr.GetAlignment());
    else if (attrDef.HasAlignment())
        newAttr.SetAlignment(attrDef.GetAlignment());

    if (attr.HasTabs())
        newAttr.SetTabs(attr.GetTabs());
    else if (attrDef.HasTabs())
        newAttr.SetTabs(attrDef.GetTabs());

    if (attr.HasLeftIndent())
        newAttr.SetLeftIndent(attr.GetLeftIndent(), attr.GetLeftSubIndent());
    else if (attrDef.HasLeftIndent())
        newAttr.SetLeftIndent(attrDef.GetLeftIndent(), attr.GetLeftSubIndent());

    if (attr.HasRightIndent())
        newAttr.SetRightIndent(attr.GetRightIndent());
    else if (attrDef.HasRightIndent())
        newAttr.SetRightIndent(attrDef.GetRightIndent());

    return newAttr;
}

void wxTextAttr::operator= (const wxTextAttr& attr)
{
    m_font = attr.m_font;
    m_colText = attr.m_colText;
    m_colBack = attr.m_colBack;
    m_textAlignment = attr.m_textAlignment;
    m_leftIndent = attr.m_leftIndent;
    m_leftSubIndent = attr.m_leftSubIndent;
    m_rightIndent = attr.m_rightIndent;
    m_tabs = attr.m_tabs;
    m_flags = attr.m_flags;
}


// apply styling to text range
bool wxTextCtrlBase::SetStyle(long WXUNUSED(start), long WXUNUSED(end),
                              const wxTextAttr& WXUNUSED(style))
{
    // to be implemented in derived TextCtrl classes
    return false;
}

// get the styling at the given position
bool wxTextCtrlBase::GetStyle(long WXUNUSED(position), wxTextAttr& WXUNUSED(style))
{
    // to be implemented in derived TextCtrl classes
    return false;
}

// change default text attributes
bool wxTextCtrlBase::SetDefaultStyle(const wxTextAttr& style)
{
    // keep the old attributes if the new style doesn't specify them unless the
    // new style is empty - then reset m_defaultStyle (as there is no other way
    // to do it)
    if ( style.IsDefault() )
        m_defaultStyle = style;
    else
        m_defaultStyle = wxTextAttr::Combine(style, m_defaultStyle, this);

    return true;
}

// get default text attributes
const wxTextAttr& wxTextCtrlBase::GetDefaultStyle() const
{
    return m_defaultStyle;
}

// ----------------------------------------------------------------------------
// file IO functions
// ----------------------------------------------------------------------------

bool wxTextCtrlBase::DoLoadFile(const wxString& filename, int WXUNUSED(fileType))
{
#if wxUSE_FFILE
    wxFFile file(filename);
    if ( file.IsOpened() )
    {
        wxString text;
        if ( file.ReadAll(&text) )
        {
            SetValue(text);

            DiscardEdits();

            m_filename = filename;

            return true;
        }
    }

    wxLogError(_("File couldn't be loaded."));
#endif // wxUSE_FFILE

    return false;
}

bool wxTextCtrlBase::SaveFile(const wxString& filename, int fileType)
{
    wxString filenameToUse = filename.empty() ? m_filename : filename;
    if ( filenameToUse.empty() )
    {
        // what kind of message to give? is it an error or a program bug?
        wxLogDebug(wxT("Can't save textctrl to file without filename."));

        return false;
    }

    return DoSaveFile(filenameToUse, fileType);
}

bool wxTextCtrlBase::DoSaveFile(const wxString& filename, int WXUNUSED(fileType))
{
#if wxUSE_FFILE
    wxFFile file(filename, _T("w"));
    if ( file.IsOpened() && file.Write(GetValue()) )
    {
        // if it worked, save for future calls
        m_filename = filename;

        // it's not modified any longer
        DiscardEdits();

        return true;
    }
#endif // wxUSE_FFILE

    wxLogError(_("The text couldn't be saved."));

    return false;
}

// ----------------------------------------------------------------------------
// stream-like insertion operator
// ----------------------------------------------------------------------------

wxTextCtrl& wxTextCtrlBase::operator<<(const wxString& s)
{
    AppendText(s);
    return *TEXTCTRL(this);
}

wxTextCtrl& wxTextCtrlBase::operator<<(float f)
{
    wxString str;
    str.Printf(wxT("%.2f"), f);
    AppendText(str);
    return *TEXTCTRL(this);
}

wxTextCtrl& wxTextCtrlBase::operator<<(double d)
{
    wxString str;
    str.Printf(wxT("%.2f"), d);
    AppendText(str);
    return *TEXTCTRL(this);
}

wxTextCtrl& wxTextCtrlBase::operator<<(int i)
{
    wxString str;
    str.Printf(wxT("%d"), i);
    AppendText(str);
    return *TEXTCTRL(this);
}

wxTextCtrl& wxTextCtrlBase::operator<<(long i)
{
    wxString str;
    str.Printf(wxT("%ld"), i);
    AppendText(str);
    return *TEXTCTRL(this);
}

wxTextCtrl& wxTextCtrlBase::operator<<(const wxChar c)
{
    return operator<<(wxString(c));
}

// ----------------------------------------------------------------------------
// streambuf methods implementation
// ----------------------------------------------------------------------------

#if wxHAS_TEXT_WINDOW_STREAM

int wxTextCtrlBase::overflow(int c)
{
    AppendText((wxChar)c);

    // return something different from EOF
    return 0;
}

#endif // wxHAS_TEXT_WINDOW_STREAM

// ----------------------------------------------------------------------------
// clipboard stuff
// ----------------------------------------------------------------------------

bool wxTextCtrlBase::CanCopy() const
{
    // can copy if there's a selection
    long from, to;
    GetSelection(&from, &to);
    return from != to;
}

bool wxTextCtrlBase::CanCut() const
{
    // can cut if there's a selection and if we're not read only
    return CanCopy() && IsEditable();
}

bool wxTextCtrlBase::CanPaste() const
{
    // can paste if we are not read only
    return IsEditable();
}

// ----------------------------------------------------------------------------
// emulating key presses
// ----------------------------------------------------------------------------

#ifdef __WIN32__
// the generic version is unused in wxMSW
bool wxTextCtrlBase::EmulateKeyPress(const wxKeyEvent& WXUNUSED(event))
{
    return false;
}
#else // !__WIN32__
bool wxTextCtrlBase::EmulateKeyPress(const wxKeyEvent& event)
{
    wxChar ch = 0;
    int keycode = event.GetKeyCode();
    switch ( keycode )
    {
        case WXK_NUMPAD0:
        case WXK_NUMPAD1:
        case WXK_NUMPAD2:
        case WXK_NUMPAD3:
        case WXK_NUMPAD4:
        case WXK_NUMPAD5:
        case WXK_NUMPAD6:
        case WXK_NUMPAD7:
        case WXK_NUMPAD8:
        case WXK_NUMPAD9:
            ch = (wxChar)(_T('0') + keycode - WXK_NUMPAD0);
            break;

        case WXK_MULTIPLY:
        case WXK_NUMPAD_MULTIPLY:
            ch = _T('*');
            break;

        case WXK_ADD:
        case WXK_NUMPAD_ADD:
            ch = _T('+');
            break;

        case WXK_SUBTRACT:
        case WXK_NUMPAD_SUBTRACT:
            ch = _T('-');
            break;

        case WXK_DECIMAL:
        case WXK_NUMPAD_DECIMAL:
            ch = _T('.');
            break;

        case WXK_DIVIDE:
        case WXK_NUMPAD_DIVIDE:
            ch = _T('/');
            break;

        case WXK_DELETE:
        case WXK_NUMPAD_DELETE:
            // delete the character at cursor
            {
                const long pos = GetInsertionPoint();
                if ( pos < GetLastPosition() )
                    Remove(pos, pos + 1);
            }
            break;

        case WXK_BACK:
            // delete the character before the cursor
            {
                const long pos = GetInsertionPoint();
                if ( pos > 0 )
                    Remove(pos - 1, pos);
            }
            break;

        default:
#if wxUSE_UNICODE
            if ( event.GetUnicodeKey() )
            {
                ch = event.GetUnicodeKey();
            }
            else
#endif
            if ( keycode < 256 && keycode >= 0 && wxIsprint(keycode) )
            {
                // FIXME this is not going to work for non letters...
                if ( !event.ShiftDown() )
                {
                    keycode = wxTolower(keycode);
                }

                ch = (wxChar)keycode;
            }
            else
            {
                ch = _T('\0');
            }
    }

    if ( ch )
    {
        WriteText(ch);

        return true;
    }

    return false;
}
#endif // !__WIN32__

// ----------------------------------------------------------------------------
// selection and ranges
// ----------------------------------------------------------------------------

void wxTextCtrlBase::SelectAll()
{
    SetSelection(0, GetLastPosition());
}

wxString wxTextCtrlBase::GetStringSelection() const
{
    long from, to;
    GetSelection(&from, &to);

    return GetRange(from, to);
}

wxString wxTextCtrlBase::GetRange(long from, long to) const
{
    wxString sel;
    if ( from < to )
    {
        sel = GetValue().Mid(from, to - from);
    }

    return sel;
}

// do the window-specific processing after processing the update event
void wxTextCtrlBase::DoUpdateWindowUI(wxUpdateUIEvent& event)
{
    // call inherited, but skip the wxControl's version, and call directly the
    // wxWindow's one instead, because the only reason why we are overriding this
    // function is that we want to use SetValue() instead of wxControl::SetLabel()
    wxWindowBase::DoUpdateWindowUI(event);

    // update text
    if ( event.GetSetText() )
    {
        if ( event.GetText() != GetValue() )
            SetValue(event.GetText());
    }
}

// ----------------------------------------------------------------------------
// hit testing
// ----------------------------------------------------------------------------

wxTextCtrlHitTestResult
wxTextCtrlBase::HitTest(const wxPoint& pt, wxTextCoord *x, wxTextCoord *y) const
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
wxTextCtrlBase::HitTest(const wxPoint& WXUNUSED(pt),
                        long * WXUNUSED(pos)) const
{
    // not implemented
    return wxTE_HT_UNKNOWN;
}

// ----------------------------------------------------------------------------
// events
// ----------------------------------------------------------------------------

void wxTextCtrlBase::SendTextUpdatedEvent()
{
    wxCommandEvent event(wxEVT_COMMAND_TEXT_UPDATED, GetId());

    // do not do this as it could be very inefficient if the text control
    // contains a lot of text and we're not using ref-counted wxString
    // implementation -- instead, event.GetString() will query the control for
    // its current text if needed
    //event.SetString(GetValue());

    event.SetEventObject(this);
    GetEventHandler()->ProcessEvent(event);
}

#else // !wxUSE_TEXTCTRL

// define this one even if !wxUSE_TEXTCTRL because it is also used by other
// controls (wxComboBox and wxSpinCtrl)

DEFINE_EVENT_TYPE(wxEVT_COMMAND_TEXT_UPDATED)

#endif // wxUSE_TEXTCTRL/!wxUSE_TEXTCTRL
