/////////////////////////////////////////////////////////////////////////////
// Name:        src/univ/topluniv.cpp
// Author:      Vaclav Slavik
// Id:          $Id: topluniv.cpp 43616 2006-11-23 14:50:42Z VS $
// Copyright:   (c) 2001-2002 SciTech Software, Inc. (www.scitechsoft.com)
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

#include "wx/toplevel.h"

#ifndef WX_PRECOMP
    #include "wx/dcclient.h"
    #include "wx/settings.h"
    #include "wx/bitmap.h"
    #include "wx/image.h"
    #include "wx/frame.h"
#endif

#include "wx/univ/renderer.h"
#include "wx/cshelp.h"
#include "wx/evtloop.h"

// ----------------------------------------------------------------------------
// wxStdTLWInputHandler: handles focus, resizing and titlebar buttons clicks
// ----------------------------------------------------------------------------

class WXDLLEXPORT wxStdTLWInputHandler : public wxStdInputHandler
{
public:
    wxStdTLWInputHandler(wxInputHandler *inphand);

    virtual bool HandleMouse(wxInputConsumer *consumer,
                             const wxMouseEvent& event);
    virtual bool HandleMouseMove(wxInputConsumer *consumer, const wxMouseEvent& event);
    virtual bool HandleActivation(wxInputConsumer *consumer, bool activated);

private:
    // the window (button) which has capture or NULL and the last hittest result
    wxTopLevelWindow *m_winCapture;
    long              m_winHitTest;
    long              m_winPressed;
    bool              m_borderCursorOn;
    wxCursor          m_origCursor;
};


// ----------------------------------------------------------------------------
// event tables
// ----------------------------------------------------------------------------

BEGIN_EVENT_TABLE(wxTopLevelWindow, wxTopLevelWindowNative)
    WX_EVENT_TABLE_INPUT_CONSUMER(wxTopLevelWindow)
    EVT_NC_PAINT(wxTopLevelWindow::OnNcPaint)
    EVT_MENU_RANGE(wxID_CLOSE_FRAME, wxID_RESTORE_FRAME, wxTopLevelWindow::OnSystemMenu)
END_EVENT_TABLE()

WX_FORWARD_TO_INPUT_CONSUMER(wxTopLevelWindow)

// ============================================================================
// implementation
// ============================================================================

int wxTopLevelWindow::ms_drawDecorations = -1;
int wxTopLevelWindow::ms_canIconize = -1;

void wxTopLevelWindow::Init()
{
    // once-only ms_drawDecorations initialization
    if ( ms_drawDecorations == -1 )
    {
        ms_drawDecorations =
            !wxSystemSettings::HasFeature(wxSYS_CAN_DRAW_FRAME_DECORATIONS) ||
            wxGetEnv(wxT("WXDECOR"), NULL);
    }

    m_usingNativeDecorations = ms_drawDecorations == 0;
    m_isActive = false;
    m_windowStyle = 0;
    m_pressedButton = 0;
}

bool wxTopLevelWindow::Create(wxWindow *parent,
                              wxWindowID id,
                              const wxString& title,
                              const wxPoint& pos,
                              const wxSize& size,
                              long style,
                              const wxString &name)
{
    // init them to avoid compiler warnings
    long styleOrig = 0,
         exstyleOrig = 0;

    if ( !m_usingNativeDecorations )
    {
        CreateInputHandler(wxINP_HANDLER_TOPLEVEL);

        styleOrig = style;
        exstyleOrig = GetExtraStyle();
        style &= ~(wxCAPTION | wxMINIMIZE_BOX | wxMAXIMIZE_BOX |
                   wxSYSTEM_MENU | wxRESIZE_BORDER | wxFRAME_TOOL_WINDOW |
                   wxRESIZE_BORDER);
        style |= wxBORDER_NONE;
        SetExtraStyle(exstyleOrig & ~wxWS_EX_CONTEXTHELP);
    }

    if ( !wxTopLevelWindowNative::Create(parent, id, title, pos,
                                         size, style, name) )
        return false;

    if ( !m_usingNativeDecorations )
    {
        m_windowStyle = styleOrig;
        m_exStyle = exstyleOrig;
    }

    return true;
}

bool wxTopLevelWindow::ShowFullScreen(bool show, long style)
{
    if ( show == IsFullScreen() ) return false;

    if ( !m_usingNativeDecorations )
    {
        if ( show )
        {
            m_fsSavedStyle = m_windowStyle;
            if ( style & wxFULLSCREEN_NOBORDER )
                m_windowStyle |= wxSIMPLE_BORDER;
            if ( style & wxFULLSCREEN_NOCAPTION )
                m_windowStyle &= ~wxCAPTION;
        }
        else
        {
            m_windowStyle = m_fsSavedStyle;
        }
    }

    return wxTopLevelWindowNative::ShowFullScreen(show, style);
}

/* static */
void wxTopLevelWindow::UseNativeDecorationsByDefault(bool native)
{
    ms_drawDecorations = !native;
}

void wxTopLevelWindow::UseNativeDecorations(bool native)
{
    wxASSERT_MSG( !m_windowStyle, _T("must be called before Create()") );

    m_usingNativeDecorations = native;
}

bool wxTopLevelWindow::IsUsingNativeDecorations() const
{
    return m_usingNativeDecorations;
}

long wxTopLevelWindow::GetDecorationsStyle() const
{
    long style = 0;

    if ( m_windowStyle & wxCAPTION )
    {
        if ( ms_canIconize == -1 )
        {
            ms_canIconize = wxSystemSettings::HasFeature(wxSYS_CAN_ICONIZE_FRAME);
        }

        style |= wxTOPLEVEL_TITLEBAR;
        if ( m_windowStyle & wxCLOSE_BOX )
            style |= wxTOPLEVEL_BUTTON_CLOSE;
        if ( (m_windowStyle & wxMINIMIZE_BOX) && ms_canIconize )
            style |= wxTOPLEVEL_BUTTON_ICONIZE;
        if ( m_windowStyle & wxMAXIMIZE_BOX )
        {
            if ( IsMaximized() )
                style |= wxTOPLEVEL_BUTTON_RESTORE;
            else
                style |= wxTOPLEVEL_BUTTON_MAXIMIZE;
        }
#if wxUSE_HELP
        if ( m_exStyle & wxWS_EX_CONTEXTHELP)
            style |= wxTOPLEVEL_BUTTON_HELP;
#endif
    }
    if ( (m_windowStyle & (wxSIMPLE_BORDER | wxNO_BORDER)) == 0 )
        style |= wxTOPLEVEL_BORDER;
    if ( m_windowStyle & (wxRESIZE_BORDER | wxRESIZE_BORDER) )
        style |= wxTOPLEVEL_RESIZEABLE;

    if ( IsMaximized() )
        style |= wxTOPLEVEL_MAXIMIZED;
    if ( GetIcon().Ok() )
        style |= wxTOPLEVEL_ICON;
    if ( m_isActive )
        style |= wxTOPLEVEL_ACTIVE;

    return style;
}

void wxTopLevelWindow::RefreshTitleBar()
{
    wxNcPaintEvent event(GetId());
    event.SetEventObject(this);
    GetEventHandler()->ProcessEvent(event);
}

// ----------------------------------------------------------------------------
// client area handling
// ----------------------------------------------------------------------------

wxPoint wxTopLevelWindow::GetClientAreaOrigin() const
{
    if ( !m_usingNativeDecorations )
    {
        int w, h;
        wxTopLevelWindowNative::DoGetClientSize(&w, &h);
        wxRect rect = wxRect(wxTopLevelWindowNative::GetClientAreaOrigin(),
                             wxSize(w, h));
        rect = m_renderer->GetFrameClientArea(rect,
                                              GetDecorationsStyle());
        return rect.GetPosition();
    }
    else
    {
        return wxTopLevelWindowNative::GetClientAreaOrigin();
    }
}

void wxTopLevelWindow::DoGetClientSize(int *width, int *height) const
{
    if ( !m_usingNativeDecorations )
    {
        int w, h;
        wxTopLevelWindowNative::DoGetClientSize(&w, &h);
        wxRect rect = wxRect(wxTopLevelWindowNative::GetClientAreaOrigin(),
                             wxSize(w, h));
        rect = m_renderer->GetFrameClientArea(rect,
                                              GetDecorationsStyle());
        if ( width )
            *width = rect.width;
        if ( height )
            *height = rect.height;
    }
    else
        wxTopLevelWindowNative::DoGetClientSize(width, height);
}

void wxTopLevelWindow::DoSetClientSize(int width, int height)
{
    if ( !m_usingNativeDecorations )
    {
        wxSize size = m_renderer->GetFrameTotalSize(wxSize(width, height),
                                               GetDecorationsStyle());
        wxTopLevelWindowNative::DoSetClientSize(size.x, size.y);
    }
    else
        wxTopLevelWindowNative::DoSetClientSize(width, height);
}

void wxTopLevelWindow::OnNcPaint(wxNcPaintEvent& event)
{
    if ( m_usingNativeDecorations || !m_renderer )
    {
        event.Skip();
    }
    else // we're drawing the decorations ourselves
    {
        // get the window rect
        wxRect rect(GetSize());

        wxWindowDC dc(this);
        m_renderer->DrawFrameTitleBar(dc, rect,
                                      GetTitle(), m_titlebarIcon,
                                      GetDecorationsStyle(),
                                      m_pressedButton,
                                      wxCONTROL_PRESSED);
    }
}

long wxTopLevelWindow::HitTest(const wxPoint& pt) const
{
    int w, h;
    wxTopLevelWindowNative::DoGetClientSize(&w, &h);
    wxRect rect(wxTopLevelWindowNative::GetClientAreaOrigin(), wxSize(w, h));

    return m_renderer->HitTestFrame(rect, pt+GetClientAreaOrigin(), GetDecorationsStyle());
}

wxSize wxTopLevelWindow::GetMinSize() const
{
    wxSize size = wxTopLevelWindowNative::GetMinSize();
    if ( !m_usingNativeDecorations )
    {
        size.IncTo(m_renderer->GetFrameMinSize(GetDecorationsStyle()));
    }

    return size;
}

// ----------------------------------------------------------------------------
// icons
// ----------------------------------------------------------------------------

void wxTopLevelWindow::SetIcons(const wxIconBundle& icons)
{
    wxTopLevelWindowNative::SetIcons(icons);

    if ( !m_usingNativeDecorations && m_renderer )
    {
        wxSize size = m_renderer->GetFrameIconSize();
        const wxIcon& icon = icons.GetIcon( size );

        if ( !icon.Ok() || size.x == wxDefaultCoord  )
            m_titlebarIcon = icon;
        else
        {
            wxBitmap bmp1;
            bmp1.CopyFromIcon(icon);
            if ( !bmp1.Ok() )
                m_titlebarIcon = wxNullIcon;
            else if ( bmp1.GetWidth() == size.x && bmp1.GetHeight() == size.y )
                m_titlebarIcon = icon;
#if wxUSE_IMAGE
            else
            {
                wxImage img = bmp1.ConvertToImage();
                img.Rescale(size.x, size.y);
                m_titlebarIcon.CopyFromBitmap(wxBitmap(img));
            }
#endif // wxUSE_IMAGE
        }
    }
}

// ----------------------------------------------------------------------------
// interactive manipulation
// ----------------------------------------------------------------------------


static bool wxGetResizingCursor(long hitTestResult, wxCursor& cursor)
{
    if ( hitTestResult & wxHT_TOPLEVEL_ANY_BORDER )
    {
        switch (hitTestResult)
        {
            case wxHT_TOPLEVEL_BORDER_N:
            case wxHT_TOPLEVEL_BORDER_S:
                cursor = wxCursor(wxCURSOR_SIZENS);
                break;
            case wxHT_TOPLEVEL_BORDER_W:
            case wxHT_TOPLEVEL_BORDER_E:
                cursor = wxCursor(wxCURSOR_SIZEWE);
                break;
            case wxHT_TOPLEVEL_BORDER_NE:
            case wxHT_TOPLEVEL_BORDER_SW:
                cursor = wxCursor(wxCURSOR_SIZENESW);
                break;
            case wxHT_TOPLEVEL_BORDER_NW:
            case wxHT_TOPLEVEL_BORDER_SE:
                cursor = wxCursor(wxCURSOR_SIZENWSE);
                break;
            default:
                return false;
        }
        return true;
    }

    return false;
}

#define wxINTERACTIVE_RESIZE_DIR \
          (wxINTERACTIVE_RESIZE_W | wxINTERACTIVE_RESIZE_E | \
           wxINTERACTIVE_RESIZE_S | wxINTERACTIVE_RESIZE_N)

struct wxInteractiveMoveData
{
    wxTopLevelWindow     *m_window;
    wxEventLoop          *m_evtLoop;
    int                   m_flags;
    wxRect                m_rect;
    wxRect                m_rectOrig;
    wxPoint               m_pos;
    wxSize                m_minSize, m_maxSize;
    bool                  m_sizingCursor;
};

class wxInteractiveMoveHandler : public wxEvtHandler
{
public:
    wxInteractiveMoveHandler(wxInteractiveMoveData& data) : m_data(data) {}

private:
    DECLARE_EVENT_TABLE()
    void OnMouseMove(wxMouseEvent& event);
    void OnMouseDown(wxMouseEvent& event);
    void OnMouseUp(wxMouseEvent& event);
    void OnKeyDown(wxKeyEvent& event);

    wxInteractiveMoveData& m_data;
};

BEGIN_EVENT_TABLE(wxInteractiveMoveHandler, wxEvtHandler)
    EVT_MOTION(wxInteractiveMoveHandler::OnMouseMove)
    EVT_LEFT_DOWN(wxInteractiveMoveHandler::OnMouseDown)
    EVT_LEFT_UP(wxInteractiveMoveHandler::OnMouseUp)
    EVT_KEY_DOWN(wxInteractiveMoveHandler::OnKeyDown)
END_EVENT_TABLE()


static inline LINKAGEMODE
void wxApplyResize(wxInteractiveMoveData& data, const wxPoint& diff)
{
    if ( data.m_flags & wxINTERACTIVE_RESIZE_W )
    {
        data.m_rect.x += diff.x;
        data.m_rect.width -= diff.x;
    }
    else if ( data.m_flags & wxINTERACTIVE_RESIZE_E )
    {
        data.m_rect.width += diff.x;
    }
    if ( data.m_flags & wxINTERACTIVE_RESIZE_N )
    {
        data.m_rect.y += diff.y;
        data.m_rect.height -= diff.y;
    }
    else if ( data.m_flags & wxINTERACTIVE_RESIZE_S )
    {
        data.m_rect.height += diff.y;
    }

    if ( data.m_minSize.x != wxDefaultCoord && data.m_rect.width < data.m_minSize.x )
    {
        if ( data.m_flags & wxINTERACTIVE_RESIZE_W )
            data.m_rect.x -= data.m_minSize.x - data.m_rect.width;
        data.m_rect.width = data.m_minSize.x;
    }
    if ( data.m_maxSize.x != wxDefaultCoord && data.m_rect.width > data.m_maxSize.x )
    {
        if ( data.m_flags & wxINTERACTIVE_RESIZE_W )
            data.m_rect.x -= data.m_minSize.x - data.m_rect.width;
        data.m_rect.width = data.m_maxSize.x;
    }
    if ( data.m_minSize.y != wxDefaultCoord && data.m_rect.height < data.m_minSize.y )
    {
        if ( data.m_flags & wxINTERACTIVE_RESIZE_N )
            data.m_rect.y -= data.m_minSize.y - data.m_rect.height;
        data.m_rect.height = data.m_minSize.y;
    }
    if ( data.m_maxSize.y != wxDefaultCoord && data.m_rect.height > data.m_maxSize.y )
    {
        if ( data.m_flags & wxINTERACTIVE_RESIZE_N )
            data.m_rect.y -= data.m_minSize.y - data.m_rect.height;
        data.m_rect.height = data.m_maxSize.y;
    }
}

void wxInteractiveMoveHandler::OnMouseMove(wxMouseEvent& event)
{
    if ( m_data.m_flags & wxINTERACTIVE_WAIT_FOR_INPUT )
        event.Skip();

    else if ( m_data.m_flags & wxINTERACTIVE_MOVE )
    {
        wxPoint diff = wxGetMousePosition() - m_data.m_pos;
        m_data.m_rect = m_data.m_rectOrig;
        m_data.m_rect.Offset(diff);
        m_data.m_window->Move(m_data.m_rect.GetPosition());
    }

    else if ( m_data.m_flags & wxINTERACTIVE_RESIZE )
    {
        wxPoint diff = wxGetMousePosition() - m_data.m_pos;
        m_data.m_rect = m_data.m_rectOrig;
        wxApplyResize(m_data, diff);
        m_data.m_window->SetSize(m_data.m_rect);
    }
}

void wxInteractiveMoveHandler::OnMouseDown(wxMouseEvent& WXUNUSED(event))
{
    if ( m_data.m_flags & wxINTERACTIVE_WAIT_FOR_INPUT )
    {
        m_data.m_evtLoop->Exit();
    }
}

void wxInteractiveMoveHandler::OnKeyDown(wxKeyEvent& event)
{
    wxPoint diff(wxDefaultCoord,wxDefaultCoord);

    switch ( event.GetKeyCode() )
    {
        case WXK_UP:    diff = wxPoint(0, -16); break;
        case WXK_DOWN:  diff = wxPoint(0, 16);  break;
        case WXK_LEFT:  diff = wxPoint(-16, 0); break;
        case WXK_RIGHT: diff = wxPoint(16, 0);  break;
        case WXK_ESCAPE:
            m_data.m_window->SetSize(m_data.m_rectOrig);
            m_data.m_evtLoop->Exit();
            return;
        case WXK_RETURN:
            m_data.m_evtLoop->Exit();
            return;
    }

    if ( diff.x != wxDefaultCoord )
    {
        if ( m_data.m_flags & wxINTERACTIVE_WAIT_FOR_INPUT )
        {
            m_data.m_flags &= ~wxINTERACTIVE_WAIT_FOR_INPUT;
            if ( m_data.m_sizingCursor )
            {
                wxEndBusyCursor();
                m_data.m_sizingCursor = false;
            }

            if ( m_data.m_flags & wxINTERACTIVE_MOVE )
            {
                m_data.m_pos = m_data.m_window->GetPosition() +
                               wxPoint(m_data.m_window->GetSize().x/2, 8);
            }
        }

        wxPoint warp;
        bool changeCur = false;

        if ( m_data.m_flags & wxINTERACTIVE_MOVE )
        {
            m_data.m_rect.Offset(diff);
            m_data.m_window->Move(m_data.m_rect.GetPosition());
            warp = wxPoint(m_data.m_window->GetSize().x/2, 8);
        }
        else /* wxINTERACTIVE_RESIZE */
        {
            if ( !(m_data.m_flags &
                  (wxINTERACTIVE_RESIZE_N | wxINTERACTIVE_RESIZE_S)) )
            {
                if ( diff.y < 0 )
                {
                    m_data.m_flags |= wxINTERACTIVE_RESIZE_N;
                    m_data.m_pos.y = m_data.m_window->GetPosition().y;
                    changeCur = true;
                }
                else if ( diff.y > 0 )
                {
                    m_data.m_flags |= wxINTERACTIVE_RESIZE_S;
                    m_data.m_pos.y = m_data.m_window->GetPosition().y +
                                     m_data.m_window->GetSize().y;
                    changeCur = true;
                }
            }
            if ( !(m_data.m_flags &
                  (wxINTERACTIVE_RESIZE_W | wxINTERACTIVE_RESIZE_E)) )
            {
                if ( diff.x < 0 )
                {
                    m_data.m_flags |= wxINTERACTIVE_RESIZE_W;
                    m_data.m_pos.x = m_data.m_window->GetPosition().x;
                    changeCur = true;
                }
                else if ( diff.x > 0 )
                {
                    m_data.m_flags |= wxINTERACTIVE_RESIZE_E;
                    m_data.m_pos.x = m_data.m_window->GetPosition().x +
                                     m_data.m_window->GetSize().x;
                    changeCur = true;
                }
            }

            wxApplyResize(m_data, diff);
            m_data.m_window->SetSize(m_data.m_rect);

            if ( m_data.m_flags & wxINTERACTIVE_RESIZE_W )
                warp.x = 0;
            else if ( m_data.m_flags & wxINTERACTIVE_RESIZE_E )
                warp.x = m_data.m_window->GetSize().x-1;
            else
                warp.x = wxGetMousePosition().x - m_data.m_window->GetPosition().x;

            if ( m_data.m_flags & wxINTERACTIVE_RESIZE_N )
                warp.y = 0;
            else if ( m_data.m_flags & wxINTERACTIVE_RESIZE_S )
                warp.y = m_data.m_window->GetSize().y-1;
            else
                warp.y = wxGetMousePosition().y - m_data.m_window->GetPosition().y;
        }

        warp -= m_data.m_window->GetClientAreaOrigin();
        m_data.m_window->WarpPointer(warp.x, warp.y);

        if ( changeCur )
        {
            long hit = m_data.m_window->HitTest(warp);
            wxCursor cur;
            if ( wxGetResizingCursor(hit, cur) )
            {
                if ( m_data.m_sizingCursor )
                    wxEndBusyCursor();
                wxBeginBusyCursor(&cur);
                m_data.m_sizingCursor = true;
            }
        }
    }
}

void wxInteractiveMoveHandler::OnMouseUp(wxMouseEvent& WXUNUSED(event))
{
    m_data.m_evtLoop->Exit();
}


void wxTopLevelWindow::InteractiveMove(int flags)
{
    wxASSERT_MSG( !((flags & wxINTERACTIVE_MOVE) && (flags & wxINTERACTIVE_RESIZE)),
                  wxT("can't move and resize window at the same time") );

    wxASSERT_MSG( !(flags & wxINTERACTIVE_RESIZE) ||
                  (flags & wxINTERACTIVE_WAIT_FOR_INPUT) ||
                  (flags & wxINTERACTIVE_RESIZE_DIR),
                  wxT("direction of resizing not specified") );

    wxInteractiveMoveData data;
    wxEventLoop loop;

    SetFocus();

#ifndef __WXGTK__
    if ( flags & wxINTERACTIVE_WAIT_FOR_INPUT )
    {
        wxCursor sizingCursor(wxCURSOR_SIZING);
        wxBeginBusyCursor(&sizingCursor);
        data.m_sizingCursor = true;
    }
    else
#endif
        data.m_sizingCursor = false;

    data.m_window = this;
    data.m_evtLoop = &loop;
    data.m_flags = flags;
    data.m_rect = data.m_rectOrig = GetRect();
    data.m_pos = wxGetMousePosition();
    data.m_minSize = wxSize(GetMinWidth(), GetMinHeight());
    data.m_maxSize = wxSize(GetMaxWidth(), GetMaxHeight());

    wxEvtHandler *handler = new wxInteractiveMoveHandler(data);
    this->PushEventHandler(handler);

    CaptureMouse();
    loop.Run();
    ReleaseMouse();

    this->RemoveEventHandler(handler);
    delete handler;

    if ( data.m_sizingCursor )
        wxEndBusyCursor();
}

// ----------------------------------------------------------------------------
// actions
// ----------------------------------------------------------------------------

void wxTopLevelWindow::ClickTitleBarButton(long button)
{
    switch ( button )
    {
        case wxTOPLEVEL_BUTTON_CLOSE:
            Close();
            break;

        case wxTOPLEVEL_BUTTON_ICONIZE:
            Iconize();
            break;

        case wxTOPLEVEL_BUTTON_MAXIMIZE:
            Maximize();
            break;

        case wxTOPLEVEL_BUTTON_RESTORE:
            Restore();
            break;

        case wxTOPLEVEL_BUTTON_HELP:
#if wxUSE_HELP
            {
            wxContextHelp contextHelp(this);
            }
#endif
            break;

        default:
            wxFAIL_MSG(wxT("incorrect button specification"));
    }
}

bool wxTopLevelWindow::PerformAction(const wxControlAction& action,
                                     long numArg,
                                     const wxString& WXUNUSED(strArg))
{
    bool isActive = numArg != 0;

    if ( action == wxACTION_TOPLEVEL_ACTIVATE )
    {
        if ( m_isActive != isActive )
        {
            m_isActive = isActive;
            RefreshTitleBar();
        }
        return true;
    }

    else if ( action == wxACTION_TOPLEVEL_BUTTON_PRESS )
    {
        m_pressedButton = numArg;
        RefreshTitleBar();
        return true;
    }

    else if ( action == wxACTION_TOPLEVEL_BUTTON_RELEASE )
    {
        m_pressedButton = 0;
        RefreshTitleBar();
        return true;
    }

    else if ( action == wxACTION_TOPLEVEL_BUTTON_CLICK )
    {
        m_pressedButton = 0;
        RefreshTitleBar();
        ClickTitleBarButton(numArg);
        return true;
    }

    else if ( action == wxACTION_TOPLEVEL_MOVE )
    {
        InteractiveMove(wxINTERACTIVE_MOVE);
        return true;
    }

    else if ( action == wxACTION_TOPLEVEL_RESIZE )
    {
        int flags = wxINTERACTIVE_RESIZE;
        if ( numArg & wxHT_TOPLEVEL_BORDER_N )
            flags |= wxINTERACTIVE_RESIZE_N;
        if ( numArg & wxHT_TOPLEVEL_BORDER_S )
            flags |= wxINTERACTIVE_RESIZE_S;
        if ( numArg & wxHT_TOPLEVEL_BORDER_W )
            flags |= wxINTERACTIVE_RESIZE_W;
        if ( numArg & wxHT_TOPLEVEL_BORDER_E )
            flags |= wxINTERACTIVE_RESIZE_E;
        InteractiveMove(flags);
        return true;
    }

    else
        return false;
}

void wxTopLevelWindow::OnSystemMenu(wxCommandEvent& event)
{
    bool ret = true;

    switch (event.GetId())
    {
        case wxID_CLOSE_FRAME:
            ret = PerformAction(wxACTION_TOPLEVEL_BUTTON_CLICK,
                                wxTOPLEVEL_BUTTON_CLOSE);
            break;
        case wxID_MOVE_FRAME:
            InteractiveMove(wxINTERACTIVE_MOVE | wxINTERACTIVE_WAIT_FOR_INPUT);
            break;
        case wxID_RESIZE_FRAME:
            InteractiveMove(wxINTERACTIVE_RESIZE | wxINTERACTIVE_WAIT_FOR_INPUT);
            break;
        case wxID_MAXIMIZE_FRAME:
            ret = PerformAction(wxACTION_TOPLEVEL_BUTTON_CLICK,
                                wxTOPLEVEL_BUTTON_MAXIMIZE);
            break;
        case wxID_ICONIZE_FRAME:
            ret = PerformAction(wxACTION_TOPLEVEL_BUTTON_CLICK,
                                wxTOPLEVEL_BUTTON_ICONIZE);
            break;
        case wxID_RESTORE_FRAME:
            ret = PerformAction(wxACTION_TOPLEVEL_BUTTON_CLICK,
                                wxTOPLEVEL_BUTTON_RESTORE);
            break;

        default:
            ret = false;
    }

    if ( !ret )
        event.Skip();
}

/* static */
wxInputHandler *
wxTopLevelWindow::GetStdInputHandler(wxInputHandler *handlerDef)
{
    static wxStdTLWInputHandler s_handler(handlerDef);

    return &s_handler;
}

// ============================================================================
// wxStdTLWInputHandler: handles focus, resizing and titlebar buttons clicks
// ============================================================================

wxStdTLWInputHandler::wxStdTLWInputHandler(wxInputHandler *inphand)
                    : wxStdInputHandler(inphand)
{
    m_winCapture = NULL;
    m_winHitTest = 0;
    m_winPressed = 0;
    m_borderCursorOn = false;
}

bool wxStdTLWInputHandler::HandleMouse(wxInputConsumer *consumer,
                                       const wxMouseEvent& event)
{
    // the button has 2 states: pressed and normal with the following
    // transitions between them:
    //
    //      normal -> left down -> capture mouse and go to pressed state
    //      pressed -> left up inside -> generate click -> go to normal
    //                         outside ------------------>
    //
    // the other mouse buttons are ignored
    if ( event.Button(1) )
    {
        if ( event.ButtonDown(1) )
        {
            wxTopLevelWindow *w = wxStaticCast(consumer->GetInputWindow(), wxTopLevelWindow);
            long hit = w->HitTest(event.GetPosition());

            if ( hit & wxHT_TOPLEVEL_ANY_BUTTON )
            {
                m_winCapture = w;
                m_winCapture->CaptureMouse();
                m_winHitTest = hit;
                m_winPressed = hit;
                consumer->PerformAction(wxACTION_TOPLEVEL_BUTTON_PRESS, m_winPressed);
                return true;
            }
            else if ( (hit & wxHT_TOPLEVEL_TITLEBAR) && !w->IsMaximized() )
            {
                consumer->PerformAction(wxACTION_TOPLEVEL_MOVE);
                return true;
            }
            else if ( (consumer->GetInputWindow()->GetWindowStyle() & wxRESIZE_BORDER)
                      && (hit & wxHT_TOPLEVEL_ANY_BORDER) )
            {
                consumer->PerformAction(wxACTION_TOPLEVEL_RESIZE, hit);
                return true;
            }
        }

        else // up
        {
            if ( m_winCapture )
            {
                m_winCapture->ReleaseMouse();
                m_winCapture = NULL;

                if ( m_winHitTest == m_winPressed )
                {
                    consumer->PerformAction(wxACTION_TOPLEVEL_BUTTON_CLICK, m_winPressed);
                    return true;
                }
            }
            //else: the mouse was released outside the window, this doesn't
            //      count as a click
        }
    }

    return wxStdInputHandler::HandleMouse(consumer, event);
}

bool wxStdTLWInputHandler::HandleMouseMove(wxInputConsumer *consumer,
                                           const wxMouseEvent& event)
{
    if ( event.GetEventObject() == m_winCapture )
    {
        long hit = m_winCapture->HitTest(event.GetPosition());

        if ( hit != m_winHitTest )
        {
            if ( hit != m_winPressed )
                consumer->PerformAction(wxACTION_TOPLEVEL_BUTTON_RELEASE, m_winPressed);
            else
                consumer->PerformAction(wxACTION_TOPLEVEL_BUTTON_PRESS, m_winPressed);

            m_winHitTest = hit;
            return true;
        }
    }
    else if ( consumer->GetInputWindow()->GetWindowStyle() & wxRESIZE_BORDER )
    {
        wxTopLevelWindow *win = wxStaticCast(consumer->GetInputWindow(),
                                             wxTopLevelWindow);
        long hit = win->HitTest(event.GetPosition());

        if ( hit != m_winHitTest )
        {
            m_winHitTest = hit;

            if ( m_borderCursorOn )
            {
                m_borderCursorOn = false;
                win->SetCursor(m_origCursor);
            }

            if ( hit & wxHT_TOPLEVEL_ANY_BORDER )
            {
                wxCursor cur;

                m_borderCursorOn = wxGetResizingCursor(hit, cur);
                if ( m_borderCursorOn )
                {
                    m_origCursor = win->GetCursor();
                    win->SetCursor(cur);
                }
            }
        }
    }

    return wxStdInputHandler::HandleMouseMove(consumer, event);
}

bool wxStdTLWInputHandler::HandleActivation(wxInputConsumer *consumer,
                                            bool activated)
{
    if ( m_borderCursorOn )
    {
        consumer->GetInputWindow()->SetCursor(m_origCursor);
        m_borderCursorOn = false;
    }
    consumer->PerformAction(wxACTION_TOPLEVEL_ACTIVATE, activated);
    return false;
}
