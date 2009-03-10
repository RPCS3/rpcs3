///////////////////////////////////////////////////////////////////////////////
// Name:        src/generic/srchctlg.cpp
// Purpose:     implements wxSearchCtrl as a composite control
// Author:      Vince Harron
// Created:     2006-02-19
// RCS-ID:      $Id: srchctlg.cpp 47962 2007-08-08 12:38:13Z JS $
// Copyright:   Vince Harron
// License:     wxWindows licence
///////////////////////////////////////////////////////////////////////////////

// For compilers that support precompilation, includes "wx.h".
#include "wx/wxprec.h"

#ifdef __BORLANDC__
    #pragma hdrstop
#endif

#if wxUSE_SEARCHCTRL

#include "wx/srchctrl.h"

#ifndef WX_PRECOMP
    #include "wx/button.h"
    #include "wx/dcclient.h"
    #include "wx/menu.h"
    #include "wx/dcmemory.h"
#endif //WX_PRECOMP

#if !wxUSE_NATIVE_SEARCH_CONTROL

#include "wx/image.h"

#define WXMAX(a,b) ((a)>(b)?(a):(b))

// ----------------------------------------------------------------------------
// constants
// ----------------------------------------------------------------------------

// the margin between the text control and the search/cancel buttons
static const wxCoord MARGIN = 2;

// border around all controls to compensate for wxSIMPLE_BORDER
#if defined(__WXMSW__)
static const wxCoord BORDER = 0;
static const wxCoord ICON_MARGIN = 2;
static const wxCoord ICON_OFFSET = 2;
#else
static const wxCoord BORDER = 2;
static const wxCoord ICON_MARGIN = 0;
static const wxCoord ICON_OFFSET = 0;
#endif

// ----------------------------------------------------------------------------
// TODO: These functions or something like them should probably be made
// public.  There are similar functions in src/aui/dockart.cpp...

static double wxBlendColour(double fg, double bg, double alpha)
{
    double result = bg + (alpha * (fg - bg));
    if (result < 0.0)
        result = 0.0;
    if (result > 255)
        result = 255;
    return result;
}

static wxColor wxStepColour(const wxColor& c, int ialpha)
{
    if (ialpha == 100)
        return c;

    double r = c.Red(), g = c.Green(), b = c.Blue();
    double bg;

    // ialpha is 0..200 where 0 is completely black
    // and 200 is completely white and 100 is the same
    // convert that to normal alpha 0.0 - 1.0
    ialpha = wxMin(ialpha, 200);
    ialpha = wxMax(ialpha, 0);
    double alpha = ((double)(ialpha - 100.0))/100.0;

    if (ialpha > 100)
    {
        // blend with white
        bg = 255.0;
        alpha = 1.0 - alpha;  // 0 = transparent fg; 1 = opaque fg
    }
     else
    {
        // blend with black
        bg = 0.0;
        alpha = 1.0 + alpha;  // 0 = transparent fg; 1 = opaque fg
    }

    r = wxBlendColour(r, bg, alpha);
    g = wxBlendColour(g, bg, alpha);
    b = wxBlendColour(b, bg, alpha);

    return wxColour((unsigned char)r, (unsigned char)g, (unsigned char)b);
}

#define LIGHT_STEP 160

// ----------------------------------------------------------------------------
// wxSearchTextCtrl: text control used by search control
// ----------------------------------------------------------------------------

class wxSearchTextCtrl : public wxTextCtrl
{
public:
    wxSearchTextCtrl(wxSearchCtrl *search, const wxString& value, int style)
        : wxTextCtrl(search, wxID_ANY, value, wxDefaultPosition, wxDefaultSize,
                     style | wxNO_BORDER)
    {
        m_search = search;
        m_defaultFG = GetForegroundColour();

        // remove the default minsize, the searchctrl will have one instead
        SetSizeHints(wxDefaultCoord,wxDefaultCoord);
    }

    void SetDescriptiveText(const wxString& text)
    {
        if ( GetValue() == m_descriptiveText )
        {
            ChangeValue(wxEmptyString);
        }

        m_descriptiveText = text;
    }

    wxString GetDescriptiveText() const
    {
        return m_descriptiveText;
    }

protected:
    void OnText(wxCommandEvent& eventText)
    {
        wxCommandEvent event(eventText);
        event.SetEventObject(m_search);
        event.SetId(m_search->GetId());

        m_search->GetEventHandler()->ProcessEvent(event);
    }

    void OnTextUrl(wxTextUrlEvent& eventText)
    {
        // copy constructor is disabled for some reason?
        //wxTextUrlEvent event(eventText);
        wxTextUrlEvent event(
            m_search->GetId(),
            eventText.GetMouseEvent(),
            eventText.GetURLStart(),
            eventText.GetURLEnd()
            );
        event.SetEventObject(m_search);

        m_search->GetEventHandler()->ProcessEvent(event);
    }

    void OnIdle(wxIdleEvent& WXUNUSED(event))
    {
        if ( IsEmpty() && !(wxWindow::FindFocus() == this) )
        {
            ChangeValue(m_descriptiveText);
            SetInsertionPoint(0);
            SetForegroundColour(wxStepColour(m_defaultFG, LIGHT_STEP));
        }
    }

    void OnFocus(wxFocusEvent& event)
    {
        event.Skip();
        if ( GetValue() == m_descriptiveText )
        {
            ChangeValue(wxEmptyString);
            SetForegroundColour(m_defaultFG);
        }
    }

private:
    wxSearchCtrl* m_search;
    wxString      m_descriptiveText;
    wxColour      m_defaultFG;

    DECLARE_EVENT_TABLE()
};

BEGIN_EVENT_TABLE(wxSearchTextCtrl, wxTextCtrl)
    EVT_TEXT(wxID_ANY, wxSearchTextCtrl::OnText)
    EVT_TEXT_ENTER(wxID_ANY, wxSearchTextCtrl::OnText)
    EVT_TEXT_URL(wxID_ANY, wxSearchTextCtrl::OnTextUrl)
    EVT_TEXT_MAXLEN(wxID_ANY, wxSearchTextCtrl::OnText)
    EVT_IDLE(wxSearchTextCtrl::OnIdle)
    EVT_SET_FOCUS(wxSearchTextCtrl::OnFocus)
END_EVENT_TABLE()

// ----------------------------------------------------------------------------
// wxSearchButton: search button used by search control
// ----------------------------------------------------------------------------

class wxSearchButton : public wxControl
{
public:
    wxSearchButton(wxSearchCtrl *search, int eventType, const wxBitmap& bmp)
        : wxControl(search, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxNO_BORDER),
          m_search(search),
          m_eventType(eventType),
          m_bmp(bmp)
    { }

    void SetBitmapLabel(const wxBitmap& label) { m_bmp = label; }


protected:
    wxSize DoGetBestSize() const
    {
        return wxSize(m_bmp.GetWidth(), m_bmp.GetHeight());
    }

    void OnLeftUp(wxMouseEvent&)
    {
        wxCommandEvent event(m_eventType, m_search->GetId());
        event.SetEventObject(m_search);

        GetEventHandler()->ProcessEvent(event);

        m_search->SetFocus();

#if wxUSE_MENUS
        if ( m_eventType == wxEVT_COMMAND_SEARCHCTRL_SEARCH_BTN )
        {
            // this happens automatically, just like on Mac OS X
            m_search->PopupSearchMenu();
        }
#endif // wxUSE_MENUS
    }

    void OnPaint(wxPaintEvent&)
    {
        wxPaintDC dc(this);
        dc.DrawBitmap(m_bmp, 0,0, true);
    }


private:
    wxSearchCtrl *m_search;
    wxEventType   m_eventType;
    wxBitmap      m_bmp;

    DECLARE_EVENT_TABLE()
};

BEGIN_EVENT_TABLE(wxSearchButton, wxControl)
    EVT_LEFT_UP(wxSearchButton::OnLeftUp)
    EVT_PAINT(wxSearchButton::OnPaint)
END_EVENT_TABLE()

BEGIN_EVENT_TABLE(wxSearchCtrl, wxSearchCtrlBase)
    EVT_SEARCHCTRL_SEARCH_BTN(wxID_ANY, wxSearchCtrl::OnSearchButton)
    EVT_SET_FOCUS(wxSearchCtrl::OnSetFocus)
    EVT_SIZE(wxSearchCtrl::OnSize)
END_EVENT_TABLE()

IMPLEMENT_DYNAMIC_CLASS(wxSearchCtrl, wxSearchCtrlBase)

// ============================================================================
// implementation
// ============================================================================

// ----------------------------------------------------------------------------
// wxSearchCtrl creation
// ----------------------------------------------------------------------------

// creation
// --------

wxSearchCtrl::wxSearchCtrl()
{
    Init();
}

wxSearchCtrl::wxSearchCtrl(wxWindow *parent, wxWindowID id,
           const wxString& value,
           const wxPoint& pos,
           const wxSize& size,
           long style,
           const wxValidator& validator,
           const wxString& name)
{
    Init();

    Create(parent, id, value, pos, size, style, validator, name);
}

void wxSearchCtrl::Init()
{
    m_text = NULL;
    m_searchButton = NULL;
    m_cancelButton = NULL;
#if wxUSE_MENUS
    m_menu = NULL;
#endif // wxUSE_MENUS

    m_searchButtonVisible = true;
    m_cancelButtonVisible = false;

    m_searchBitmapUser = false;
    m_cancelBitmapUser = false;
#if wxUSE_MENUS
    m_searchMenuBitmapUser = false;
#endif // wxUSE_MENUS
}

bool wxSearchCtrl::Create(wxWindow *parent, wxWindowID id,
            const wxString& value,
            const wxPoint& pos,
            const wxSize& size,
            long style,
            const wxValidator& validator,
            const wxString& name)
{
	int borderStyle = wxBORDER_SIMPLE;

#if defined(__WXMSW__)
    borderStyle = GetThemedBorderStyle();
    if (borderStyle == wxBORDER_SUNKEN)
        borderStyle = wxBORDER_SIMPLE;
#elif defined(__WXGTK__)
    borderStyle = wxBORDER_SUNKEN;
#endif

    if ( !wxTextCtrlBase::Create(parent, id, pos, size, borderStyle | (style & ~wxBORDER_MASK), validator, name) )
    {
        return false;
    }

    m_text = new wxSearchTextCtrl(this, value, style & ~wxBORDER_MASK);
    m_text->SetDescriptiveText(_("Search"));

    wxSize sizeText = m_text->GetBestSize();

    m_searchButton = new wxSearchButton(this,wxEVT_COMMAND_SEARCHCTRL_SEARCH_BTN,m_searchBitmap);
    m_cancelButton = new wxSearchButton(this,wxEVT_COMMAND_SEARCHCTRL_CANCEL_BTN,m_cancelBitmap);

    SetForegroundColour( m_text->GetForegroundColour() );
    m_searchButton->SetForegroundColour( m_text->GetForegroundColour() );
    m_cancelButton->SetForegroundColour( m_text->GetForegroundColour() );

    SetBackgroundColour( m_text->GetBackgroundColour() );
    m_searchButton->SetBackgroundColour( m_text->GetBackgroundColour() );
    m_cancelButton->SetBackgroundColour( m_text->GetBackgroundColour() );

    RecalcBitmaps();

    SetInitialSize(size);
    Move(pos);
    return true;
}

wxSearchCtrl::~wxSearchCtrl()
{
    delete m_text;
    delete m_searchButton;
    delete m_cancelButton;
#if wxUSE_MENUS
    delete m_menu;
#endif // wxUSE_MENUS
}


// search control specific interfaces
#if wxUSE_MENUS

void wxSearchCtrl::SetMenu( wxMenu* menu )
{
    if ( menu == m_menu )
    {
        // no change
        return;
    }
    bool hadMenu = (m_menu != NULL);
    delete m_menu;
    m_menu = menu;

    if ( m_menu && !hadMenu )
    {
        m_searchButton->SetBitmapLabel(m_searchMenuBitmap);
        m_searchButton->Refresh();
    }
    else if ( !m_menu && hadMenu )
    {
        m_searchButton->SetBitmapLabel(m_searchBitmap);
        if ( m_searchButtonVisible )
        {
            m_searchButton->Refresh();
        }
    }
    wxRect rect = GetRect();
    LayoutControls(0, 0, rect.GetWidth(), rect.GetHeight());
}

wxMenu* wxSearchCtrl::GetMenu()
{
    return m_menu;
}

#endif // wxUSE_MENUS

void wxSearchCtrl::ShowSearchButton( bool show )
{
    if ( m_searchButtonVisible == show )
    {
        // no change
        return;
    }
    m_searchButtonVisible = show;
    if ( m_searchButtonVisible )
    {
        RecalcBitmaps();
    }

    wxRect rect = GetRect();
    LayoutControls(0, 0, rect.GetWidth(), rect.GetHeight());
}

bool wxSearchCtrl::IsSearchButtonVisible() const
{
    return m_searchButtonVisible;
}


void wxSearchCtrl::ShowCancelButton( bool show )
{
    if ( m_cancelButtonVisible == show )
    {
        // no change
        return;
    }
    m_cancelButtonVisible = show;

    wxRect rect = GetRect();
    LayoutControls(0, 0, rect.GetWidth(), rect.GetHeight());
}

bool wxSearchCtrl::IsCancelButtonVisible() const
{
    return m_cancelButtonVisible;
}

void wxSearchCtrl::SetDescriptiveText(const wxString& text)
{
    m_text->SetDescriptiveText(text);
}

wxString wxSearchCtrl::GetDescriptiveText() const
{
    return m_text->GetDescriptiveText();
}

// ----------------------------------------------------------------------------
// geometry
// ----------------------------------------------------------------------------

wxSize wxSearchCtrl::DoGetBestSize() const
{
    wxSize sizeText = m_text->GetBestSize();
    wxSize sizeSearch(0,0);
    wxSize sizeCancel(0,0);
    int searchMargin = 0;
    int cancelMargin = 0;
    if ( m_searchButtonVisible || HasMenu() )
    {
        sizeSearch = m_searchButton->GetBestSize();
        searchMargin = MARGIN;
    }
    if ( m_cancelButtonVisible )
    {
        sizeCancel = m_cancelButton->GetBestSize();
        cancelMargin = MARGIN;
    }

    int horizontalBorder = 1 + ( sizeText.y - sizeText.y * 14 / 21 ) / 2;

    // buttons are square and equal to the height of the text control
    int height = sizeText.y;
    return wxSize(sizeSearch.x + searchMargin + sizeText.x + cancelMargin + sizeCancel.x + 2*horizontalBorder,
                  height + 2*BORDER);
}

void wxSearchCtrl::DoMoveWindow(int x, int y, int width, int height)
{
    wxSearchCtrlBase::DoMoveWindow(x, y, width, height);

    LayoutControls(0, 0, width, height);
}

void wxSearchCtrl::LayoutControls(int x, int y, int width, int height)
{
    if ( !m_text )
        return;

    wxSize sizeText = m_text->GetBestSize();
    // make room for the search menu & clear button
    int horizontalBorder = ( sizeText.y - sizeText.y * 14 / 21 ) / 2;
    x += horizontalBorder;
    y += BORDER;
    width -= horizontalBorder*2;
    height -= BORDER*2;

    wxSize sizeSearch(0,0);
    wxSize sizeCancel(0,0);
    int searchMargin = 0;
    int cancelMargin = 0;
    if ( m_searchButtonVisible || HasMenu() )
    {
        sizeSearch = m_searchButton->GetBestSize();
        searchMargin = MARGIN;
    }
    if ( m_cancelButtonVisible )
    {
        sizeCancel = m_cancelButton->GetBestSize();
        cancelMargin = MARGIN;
    }
    m_searchButton->Show( m_searchButtonVisible || HasMenu() );
    m_cancelButton->Show( m_cancelButtonVisible );

    if ( sizeSearch.x + sizeCancel.x > width )
    {
        sizeSearch.x = width/2;
        sizeCancel.x = width/2;
        searchMargin = 0;
        cancelMargin = 0;
    }
    wxCoord textWidth = width - sizeSearch.x - sizeCancel.x - searchMargin - cancelMargin - 1;

    // position the subcontrols inside the client area

    m_searchButton->SetSize(x, y + ICON_OFFSET - 1, sizeSearch.x, height);
    m_text->SetSize( x + sizeSearch.x + searchMargin,
                     y + ICON_OFFSET - BORDER,
                     textWidth,
                     height);
    m_cancelButton->SetSize(x + sizeSearch.x + searchMargin + textWidth + cancelMargin,
                            y + ICON_OFFSET - 1, sizeCancel.x, height);
}


// accessors
// ---------

wxString wxSearchCtrl::GetValue() const
{
    wxString value = m_text->GetValue();
    if (value == m_text->GetDescriptiveText())
        return wxEmptyString;
    else
        return value;
}
void wxSearchCtrl::SetValue(const wxString& value)
{
    m_text->SetValue(value);
}

wxString wxSearchCtrl::GetRange(long from, long to) const
{
    return m_text->GetRange(from, to);
}

int wxSearchCtrl::GetLineLength(long lineNo) const
{
    return m_text->GetLineLength(lineNo);
}
wxString wxSearchCtrl::GetLineText(long lineNo) const
{
    return m_text->GetLineText(lineNo);
}
int wxSearchCtrl::GetNumberOfLines() const
{
    return m_text->GetNumberOfLines();
}

bool wxSearchCtrl::IsModified() const
{
    return m_text->IsModified();
}
bool wxSearchCtrl::IsEditable() const
{
    return m_text->IsEditable();
}

// more readable flag testing methods
bool wxSearchCtrl::IsSingleLine() const
{
    return m_text->IsSingleLine();
}
bool wxSearchCtrl::IsMultiLine() const
{
    return m_text->IsMultiLine();
}

// If the return values from and to are the same, there is no selection.
void wxSearchCtrl::GetSelection(long* from, long* to) const
{
    m_text->GetSelection(from, to);
}

wxString wxSearchCtrl::GetStringSelection() const
{
    return m_text->GetStringSelection();
}

// operations
// ----------

// editing
void wxSearchCtrl::Clear()
{
    m_text->Clear();
}
void wxSearchCtrl::Replace(long from, long to, const wxString& value)
{
    m_text->Replace(from, to, value);
}
void wxSearchCtrl::Remove(long from, long to)
{
    m_text->Remove(from, to);
}

// load/save the controls contents from/to the file
bool wxSearchCtrl::LoadFile(const wxString& file)
{
    return m_text->LoadFile(file);
}
bool wxSearchCtrl::SaveFile(const wxString& file)
{
    return m_text->SaveFile(file);
}

// sets/clears the dirty flag
void wxSearchCtrl::MarkDirty()
{
    m_text->MarkDirty();
}
void wxSearchCtrl::DiscardEdits()
{
    m_text->DiscardEdits();
}

// set the max number of characters which may be entered in a single line
// text control
void wxSearchCtrl::SetMaxLength(unsigned long len)
{
    m_text->SetMaxLength(len);
}

// writing text inserts it at the current position, appending always
// inserts it at the end
void wxSearchCtrl::WriteText(const wxString& text)
{
    m_text->WriteText(text);
}
void wxSearchCtrl::AppendText(const wxString& text)
{
    m_text->AppendText(text);
}

// insert the character which would have resulted from this key event,
// return true if anything has been inserted
bool wxSearchCtrl::EmulateKeyPress(const wxKeyEvent& event)
{
    return m_text->EmulateKeyPress(event);
}

// text control under some platforms supports the text styles: these
// methods allow to apply the given text style to the given selection or to
// set/get the style which will be used for all appended text
bool wxSearchCtrl::SetStyle(long start, long end, const wxTextAttr& style)
{
    return m_text->SetStyle(start, end, style);
}
bool wxSearchCtrl::GetStyle(long position, wxTextAttr& style)
{
    return m_text->GetStyle(position, style);
}
bool wxSearchCtrl::SetDefaultStyle(const wxTextAttr& style)
{
    return m_text->SetDefaultStyle(style);
}
const wxTextAttr& wxSearchCtrl::GetDefaultStyle() const
{
    return m_text->GetDefaultStyle();
}

// translate between the position (which is just an index in the text ctrl
// considering all its contents as a single strings) and (x, y) coordinates
// which represent column and line.
long wxSearchCtrl::XYToPosition(long x, long y) const
{
    return m_text->XYToPosition(x, y);
}
bool wxSearchCtrl::PositionToXY(long pos, long *x, long *y) const
{
    return m_text->PositionToXY(pos, x, y);
}

void wxSearchCtrl::ShowPosition(long pos)
{
    m_text->ShowPosition(pos);
}

// find the character at position given in pixels
//
// NB: pt is in device coords (not adjusted for the client area origin nor
//     scrolling)
wxTextCtrlHitTestResult wxSearchCtrl::HitTest(const wxPoint& pt, long *pos) const
{
    return m_text->HitTest(pt, pos);
}
wxTextCtrlHitTestResult wxSearchCtrl::HitTest(const wxPoint& pt,
                                        wxTextCoord *col,
                                        wxTextCoord *row) const
{
    return m_text->HitTest(pt, col, row);
}

// Clipboard operations
void wxSearchCtrl::Copy()
{
    m_text->Copy();
}
void wxSearchCtrl::Cut()
{
    m_text->Cut();
}
void wxSearchCtrl::Paste()
{
    m_text->Paste();
}

bool wxSearchCtrl::CanCopy() const
{
    return m_text->CanCopy();
}
bool wxSearchCtrl::CanCut() const
{
    return m_text->CanCut();
}
bool wxSearchCtrl::CanPaste() const
{
    return m_text->CanPaste();
}

// Undo/redo
void wxSearchCtrl::Undo()
{
    m_text->Undo();
}
void wxSearchCtrl::Redo()
{
    m_text->Redo();
}

bool wxSearchCtrl::CanUndo() const
{
    return m_text->CanUndo();
}
bool wxSearchCtrl::CanRedo() const
{
    return m_text->CanRedo();
}

// Insertion point
void wxSearchCtrl::SetInsertionPoint(long pos)
{
    m_text->SetInsertionPoint(pos);
}
void wxSearchCtrl::SetInsertionPointEnd()
{
    m_text->SetInsertionPointEnd();
}
long wxSearchCtrl::GetInsertionPoint() const
{
    return m_text->GetInsertionPoint();
}
wxTextPos wxSearchCtrl::GetLastPosition() const
{
    return m_text->GetLastPosition();
}

void wxSearchCtrl::SetSelection(long from, long to)
{
    m_text->SetSelection(from, to);
}
void wxSearchCtrl::SelectAll()
{
    m_text->SelectAll();
}

void wxSearchCtrl::SetEditable(bool editable)
{
    m_text->SetEditable(editable);
}

bool wxSearchCtrl::SetFont(const wxFont& font)
{
    bool result = wxSearchCtrlBase::SetFont(font);
    if ( result && m_text )
    {
        result = m_text->SetFont(font);
    }
    RecalcBitmaps();
    return result;
}

// search control generic only
void wxSearchCtrl::SetSearchBitmap( const wxBitmap& bitmap )
{
    m_searchBitmap = bitmap;
    m_searchBitmapUser = bitmap.Ok();
    if ( m_searchBitmapUser )
    {
        if ( m_searchButton && !HasMenu() )
        {
            m_searchButton->SetBitmapLabel( m_searchBitmap );
        }
    }
    else
    {
        // the user bitmap was just cleared, generate one
        RecalcBitmaps();
    }
}

#if wxUSE_MENUS

void wxSearchCtrl::SetSearchMenuBitmap( const wxBitmap& bitmap )
{
    m_searchMenuBitmap = bitmap;
    m_searchMenuBitmapUser = bitmap.Ok();
    if ( m_searchMenuBitmapUser )
    {
        if ( m_searchButton && m_menu )
        {
            m_searchButton->SetBitmapLabel( m_searchMenuBitmap );
        }
    }
    else
    {
        // the user bitmap was just cleared, generate one
        RecalcBitmaps();
    }
}

#endif // wxUSE_MENUS

void wxSearchCtrl::SetCancelBitmap( const wxBitmap& bitmap )
{
    m_cancelBitmap = bitmap;
    m_cancelBitmapUser = bitmap.Ok();
    if ( m_cancelBitmapUser )
    {
        if ( m_cancelButton )
        {
            m_cancelButton->SetBitmapLabel( m_cancelBitmap );
        }
    }
    else
    {
        // the user bitmap was just cleared, generate one
        RecalcBitmaps();
    }
}

#if 0

// override streambuf method
#if wxHAS_TEXT_WINDOW_STREAM
int overflow(int i);
#endif // wxHAS_TEXT_WINDOW_STREAM

// stream-like insertion operators: these are always available, whether we
// were, or not, compiled with streambuf support
wxTextCtrl& operator<<(const wxString& s);
wxTextCtrl& operator<<(int i);
wxTextCtrl& operator<<(long i);
wxTextCtrl& operator<<(float f);
wxTextCtrl& operator<<(double d);
wxTextCtrl& operator<<(const wxChar c);
#endif

void wxSearchCtrl::DoSetValue(const wxString& value, int flags)
{
    m_text->ChangeValue( value );
    if ( flags & SetValue_SendEvent )
        SendTextUpdatedEvent();
}

// do the window-specific processing after processing the update event
void wxSearchCtrl::DoUpdateWindowUI(wxUpdateUIEvent& event)
{
    wxSearchCtrlBase::DoUpdateWindowUI(event);
}

bool wxSearchCtrl::ShouldInheritColours() const
{
    return true;
}

// icons are rendered at 3-8 times larger than necessary and downscaled for
// antialiasing
static int GetMultiplier()
{
#ifdef __WXWINCE__
    // speed up bitmap generation by using a small bitmap
    return 3;
#else
    int depth = ::wxDisplayDepth();

    if  ( depth >= 24 )
    {
        return 8;
    }
    return 6;
#endif
}

wxBitmap wxSearchCtrl::RenderSearchBitmap( int x, int y, bool renderDrop )
{
    wxColour bg = GetBackgroundColour();
    wxColour fg = wxStepColour(GetForegroundColour(), LIGHT_STEP-20);

    //===============================================================================
    // begin drawing code
    //===============================================================================
    // image stats

    // force width:height ratio
    if ( 14*x > y*20 )
    {
        // x is too big
        x = y*20/14;
    }
    else
    {
        // y is too big
        y = x*14/20;
    }

    // glass 11x11, top left corner
    // handle (9,9)-(13,13)
    // drop (13,16)-(19,6)-(16,9)

    int multiplier = GetMultiplier();
    int penWidth = multiplier * 2;

    penWidth = penWidth * x / 20;

    wxBitmap bitmap( multiplier*x, multiplier*y );
    wxMemoryDC mem;
    mem.SelectObject(bitmap);

    // clear background
    mem.SetBrush( wxBrush(bg) );
    mem.SetPen( wxPen(bg) );
    mem.DrawRectangle(0,0,bitmap.GetWidth(),bitmap.GetHeight());

    // draw drop glass
    mem.SetBrush( wxBrush(fg) );
    mem.SetPen( wxPen(fg) );
    int glassBase = 5 * x / 20;
    int glassFactor = 2*glassBase + 1;
    int radius = multiplier*glassFactor/2;
    mem.DrawCircle(radius,radius,radius);
    mem.SetBrush( wxBrush(bg) );
    mem.SetPen( wxPen(bg) );
    mem.DrawCircle(radius,radius,radius-penWidth);

    // draw handle
    int lineStart = radius + (radius-penWidth/2) * 707 / 1000; // 707 / 1000 = 0.707 = 1/sqrt(2);

    mem.SetPen( wxPen(fg) );
    mem.SetBrush( wxBrush(fg) );
    int handleCornerShift = penWidth * 707 / 1000 / 2; // 707 / 1000 = 0.707 = 1/sqrt(2);
    handleCornerShift = WXMAX( handleCornerShift, 1 );
    int handleBase = 4 * x / 20;
    int handleLength = 2*handleBase+1;
    wxPoint handlePolygon[] =
    {
        wxPoint(-handleCornerShift,+handleCornerShift),
        wxPoint(+handleCornerShift,-handleCornerShift),
        wxPoint(multiplier*handleLength/2+handleCornerShift,multiplier*handleLength/2-handleCornerShift),
        wxPoint(multiplier*handleLength/2-handleCornerShift,multiplier*handleLength/2+handleCornerShift),
    };
    mem.DrawPolygon(WXSIZEOF(handlePolygon),handlePolygon,lineStart,lineStart);

    // draw drop triangle
    int triangleX = 13 * x / 20;
    int triangleY = 5 * x / 20;
    int triangleBase = 3 * x / 20;
    int triangleFactor = triangleBase*2+1;
    if ( renderDrop )
    {
        wxPoint dropPolygon[] =
        {
            wxPoint(multiplier*0,multiplier*0), // triangle left
            wxPoint(multiplier*triangleFactor-1,multiplier*0), // triangle right
            wxPoint(multiplier*triangleFactor/2,multiplier*triangleFactor/2), // triangle bottom
        };
        mem.DrawPolygon(WXSIZEOF(dropPolygon),dropPolygon,multiplier*triangleX,multiplier*triangleY);
    }
    mem.SelectObject(wxNullBitmap);

    //===============================================================================
    // end drawing code
    //===============================================================================

    if ( multiplier != 1 )
    {
        wxImage image = bitmap.ConvertToImage();
        image.Rescale(x,y);
        bitmap = wxBitmap( image );
    }
    if ( !renderDrop )
    {
        // Trim the edge where the arrow would have gone
        bitmap = bitmap.GetSubBitmap(wxRect(0,0, y,y));
    }

    return bitmap;
}

wxBitmap wxSearchCtrl::RenderCancelBitmap( int x, int y )
{
    wxColour bg = GetBackgroundColour();
    wxColour fg = wxStepColour(GetForegroundColour(), LIGHT_STEP);

    //===============================================================================
    // begin drawing code
    //===============================================================================
    // image stats

    // total size 14x14
    // force 1:1 ratio
    if ( x > y )
    {
        // x is too big
        x = y;
    }
    else
    {
        // y is too big
        y = x;
    }

    // 14x14 circle
    // cross line starts (4,4)-(10,10)
    // drop (13,16)-(19,6)-(16,9)

    int multiplier = GetMultiplier();

    int penWidth = multiplier * x / 14;

    wxBitmap bitmap( multiplier*x, multiplier*y );
    wxMemoryDC mem;
    mem.SelectObject(bitmap);

    // clear background
    mem.SetBrush( wxBrush(bg) );
    mem.SetPen( wxPen(bg) );
    mem.DrawRectangle(0,0,bitmap.GetWidth(),bitmap.GetHeight());

    // draw drop glass
    mem.SetBrush( wxBrush(fg) );
    mem.SetPen( wxPen(fg) );
    int radius = multiplier*x/2;
    mem.DrawCircle(radius,radius,radius);

    // draw cross
    int lineStartBase = 4 * x / 14;
    int lineLength = x - 2*lineStartBase;

    mem.SetPen( wxPen(bg) );
    mem.SetBrush( wxBrush(bg) );
    int handleCornerShift = penWidth/2;
    handleCornerShift = WXMAX( handleCornerShift, 1 );
    wxPoint handlePolygon[] =
    {
        wxPoint(-handleCornerShift,+handleCornerShift),
        wxPoint(+handleCornerShift,-handleCornerShift),
        wxPoint(multiplier*lineLength+handleCornerShift,multiplier*lineLength-handleCornerShift),
        wxPoint(multiplier*lineLength-handleCornerShift,multiplier*lineLength+handleCornerShift),
    };
    mem.DrawPolygon(WXSIZEOF(handlePolygon),handlePolygon,multiplier*lineStartBase,multiplier*lineStartBase);
    wxPoint handlePolygon2[] =
    {
        wxPoint(+handleCornerShift,+handleCornerShift),
        wxPoint(-handleCornerShift,-handleCornerShift),
        wxPoint(multiplier*lineLength-handleCornerShift,-multiplier*lineLength-handleCornerShift),
        wxPoint(multiplier*lineLength+handleCornerShift,-multiplier*lineLength+handleCornerShift),
    };
    mem.DrawPolygon(WXSIZEOF(handlePolygon2),handlePolygon2,multiplier*lineStartBase,multiplier*(x-lineStartBase));

    //===============================================================================
    // end drawing code
    //===============================================================================

    if ( multiplier != 1 )
    {
        wxImage image = bitmap.ConvertToImage();
        image.Rescale(x,y);
        bitmap = wxBitmap( image );
    }

    return bitmap;
}

void wxSearchCtrl::RecalcBitmaps()
{
    if ( !m_text )
    {
        return;
    }
    wxSize sizeText = m_text->GetBestSize();

    int bitmapHeight = sizeText.y - 2 * ICON_MARGIN;
    int bitmapWidth  = sizeText.y * 20 / 14;

    if ( !m_searchBitmapUser )
    {
        if (
            !m_searchBitmap.Ok() ||
            m_searchBitmap.GetHeight() != bitmapHeight ||
            m_searchBitmap.GetWidth() != bitmapWidth
            )
        {
            m_searchBitmap = RenderSearchBitmap(bitmapWidth,bitmapHeight,false);
            if ( !HasMenu() )
            {
                m_searchButton->SetBitmapLabel(m_searchBitmap);
            }
        }
        // else this bitmap was set by user, don't alter
    }

#if wxUSE_MENUS
    if ( !m_searchMenuBitmapUser )
    {
        if (
            !m_searchMenuBitmap.Ok() ||
            m_searchMenuBitmap.GetHeight() != bitmapHeight ||
            m_searchMenuBitmap.GetWidth() != bitmapWidth
            )
        {
            m_searchMenuBitmap = RenderSearchBitmap(bitmapWidth,bitmapHeight,true);
            if ( m_menu )
            {
                m_searchButton->SetBitmapLabel(m_searchMenuBitmap);
            }
        }
        // else this bitmap was set by user, don't alter
    }
#endif // wxUSE_MENUS

    if ( !m_cancelBitmapUser )
    {
        if (
            !m_cancelBitmap.Ok() ||
            m_cancelBitmap.GetHeight() != bitmapHeight ||
            m_cancelBitmap.GetWidth() != bitmapHeight
            )
        {
            m_cancelBitmap = RenderCancelBitmap(bitmapHeight-BORDER-1,bitmapHeight-BORDER-1); // square
            m_cancelButton->SetBitmapLabel(m_cancelBitmap);
        }
        // else this bitmap was set by user, don't alter
    }
}

void wxSearchCtrl::OnSearchButton( wxCommandEvent& event )
{
    event.Skip();
}

void wxSearchCtrl::OnSetFocus( wxFocusEvent& /*event*/ )
{
    if ( m_text )
    {
        m_text->SetFocus();
    }
}

void wxSearchCtrl::OnSize( wxSizeEvent& WXUNUSED(event) )
{
    int width, height;
    GetSize(&width, &height);
    LayoutControls(0, 0, width, height);
}

#if wxUSE_MENUS

void wxSearchCtrl::PopupSearchMenu()
{
    if ( m_menu )
    {
        wxSize size = GetSize();
        PopupMenu( m_menu, 0, size.y );
    }
}

#endif // wxUSE_MENUS

#endif // !wxUSE_NATIVE_SEARCH_CONTROL

#endif // wxUSE_SEARCHCTRL
