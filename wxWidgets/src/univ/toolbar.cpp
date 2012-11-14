/////////////////////////////////////////////////////////////////////////////
// Name:        src/univ/toolbar.cpp
// Purpose:     implementation of wxToolBar for wxUniversal
// Author:      Robert Roebling, Vadim Zeitlin (universalization)
// Modified by:
// Created:     20.02.02
// Id:          $Id: toolbar.cpp 42840 2006-10-31 13:09:08Z VZ $
// Copyright:   (c) 2001 Robert Roebling,
//              (c) 2002 SciTech Software, Inc. (www.scitechsoft.com)
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

#if wxUSE_TOOLBAR

#include "wx/toolbar.h"

#ifndef WX_PRECOMP
    #include "wx/utils.h"
    #include "wx/app.h"
    #include "wx/log.h"
    #include "wx/frame.h"
    #include "wx/dc.h"
    #include "wx/image.h"
#endif

#include "wx/univ/renderer.h"

// ----------------------------------------------------------------------------
// wxStdToolbarInputHandler: translates SPACE and ENTER keys and the left mouse
// click into button press/release actions
// ----------------------------------------------------------------------------

class WXDLLEXPORT wxStdToolbarInputHandler : public wxStdInputHandler
{
public:
    wxStdToolbarInputHandler(wxInputHandler *inphand);

    virtual bool HandleKey(wxInputConsumer *consumer,
                           const wxKeyEvent& event,
                           bool pressed);
    virtual bool HandleMouse(wxInputConsumer *consumer,
                             const wxMouseEvent& event);
    virtual bool HandleMouseMove(wxInputConsumer *consumer, const wxMouseEvent& event);
    virtual bool HandleFocus(wxInputConsumer *consumer, const wxFocusEvent& event);
    virtual bool HandleActivation(wxInputConsumer *consumer, bool activated);

private:
    wxWindow            *m_winCapture;
    wxToolBarToolBase   *m_toolCapture;
    wxToolBarToolBase   *m_toolLast;
};

// ----------------------------------------------------------------------------
// constants
// ----------------------------------------------------------------------------

// value meaning that m_widthSeparator is not initialized
static const wxCoord INVALID_WIDTH = wxDefaultCoord;

// ----------------------------------------------------------------------------
// wxToolBarTool: our implementation of wxToolBarToolBase
// ----------------------------------------------------------------------------

class WXDLLEXPORT wxToolBarTool : public wxToolBarToolBase
{
public:
    wxToolBarTool(wxToolBar *tbar,
                  int id,
                  const wxString& label,
                  const wxBitmap& bmpNormal,
                  const wxBitmap& bmpDisabled,
                  wxItemKind kind,
                  wxObject *clientData,
                  const wxString& shortHelp,
                  const wxString& longHelp)
        : wxToolBarToolBase(tbar, id, label, bmpNormal, bmpDisabled, kind,
                            clientData, shortHelp, longHelp)
    {
        // no position yet
        m_x =
        m_y = wxDefaultCoord;
        m_width =
        m_height = 0;

        // not pressed yet
        m_isInverted = false;

        // mouse not here yet
        m_underMouse = false;
    }

    wxToolBarTool(wxToolBar *tbar, wxControl *control)
        : wxToolBarToolBase(tbar, control)
    {
        // no position yet
        m_x =
        m_y = wxDefaultCoord;
        m_width =
        m_height = 0;

        // not pressed yet
        m_isInverted = false;

        // mouse not here yet
        m_underMouse = false;
    }

    // is this tool pressed, even temporarily? (this is different from being
    // permanently toggled which is what IsToggled() returns)
    bool IsPressed() const
        { return CanBeToggled() ? IsToggled() != m_isInverted : m_isInverted; }

    // are we temporarily pressed/unpressed?
    bool IsInverted() const { return m_isInverted; }

    // press the tool temporarily by inverting its toggle state
    void Invert() { m_isInverted = !m_isInverted; }

    // Set underMouse
    void SetUnderMouse( bool under = true ) { m_underMouse = under; }
    bool IsUnderMouse() { return m_underMouse; }

public:
    // the tool position (for controls)
    wxCoord m_x;
    wxCoord m_y;
    wxCoord m_width;
    wxCoord m_height;

private:
    // true if the tool is pressed
    bool m_isInverted;

    // true if the tool is under the mouse
    bool m_underMouse;
};

// ============================================================================
// wxToolBar implementation
// ============================================================================

IMPLEMENT_DYNAMIC_CLASS(wxToolBar, wxControl)

// ----------------------------------------------------------------------------
// wxToolBar creation
// ----------------------------------------------------------------------------

void wxToolBar::Init()
{
    // no tools yet
    m_needsLayout = false;

    // unknown widths for the tools and separators
    m_widthSeparator = INVALID_WIDTH;

    m_maxWidth =
    m_maxHeight = 0;

    wxRenderer *renderer = GetRenderer();

    SetToolBitmapSize(renderer->GetToolBarButtonSize(&m_widthSeparator));
    SetMargins(renderer->GetToolBarMargin());
}

bool wxToolBar::Create(wxWindow *parent,
                       wxWindowID id,
                       const wxPoint& pos,
                       const wxSize& size,
                       long style,
                       const wxString& name)
{
    if ( !wxToolBarBase::Create(parent, id, pos, size, style,
                                wxDefaultValidator, name) )
    {
        return false;
    }

    FixupStyle();

    CreateInputHandler(wxINP_HANDLER_TOOLBAR);

    SetInitialSize(size);

    return true;
}

wxToolBar::~wxToolBar()
{
    // Make sure the toolbar is removed from the parent.
    SetSize(0,0);
}

void wxToolBar::SetMargins(int x, int y)
{
    // This required for similar visual effects under
    // native platforms and wxUniv.
    wxToolBarBase::SetMargins( x + 3, y + 3 );
}

// ----------------------------------------------------------------------------
// wxToolBar tool-related methods
// ----------------------------------------------------------------------------

wxToolBarToolBase *wxToolBar::FindToolForPosition(wxCoord x, wxCoord y) const
{
    // check the "other" direction first: it must be inside the toolbar or we
    // don't risk finding anything
    if ( IsVertical() )
    {
        if ( x < 0 || x > m_maxWidth )
            return NULL;

        // we always use x, even for a vertical toolbar, this makes the code
        // below simpler
        x = y;
    }
    else // horizontal
    {
        if ( y < 0 || y > m_maxHeight )
            return NULL;
    }

    for ( wxToolBarToolsList::compatibility_iterator node = m_tools.GetFirst();
          node;
          node = node->GetNext() )
    {
        wxToolBarToolBase *tool =  node->GetData();
        wxRect rectTool = GetToolRect(tool);

        wxCoord startTool, endTool;
        GetRectLimits(rectTool, &startTool, &endTool);

        if ( x >= startTool && x <= endTool )
        {
            // don't return the separators from here, they don't accept any
            // input anyhow
            return tool->IsSeparator() ? NULL : tool;
        }
    }

    return NULL;
}

void wxToolBar::SetToolShortHelp(int id, const wxString& help)
{
    wxToolBarToolBase *tool = FindById(id);

    wxCHECK_RET( tool, _T("SetToolShortHelp: no such tool") );

    tool->SetShortHelp(help);
}

bool wxToolBar::DoInsertTool(size_t WXUNUSED(pos),
                             wxToolBarToolBase * WXUNUSED(tool))
{
    // recalculate the toolbar geometry before redrawing it the next time
    m_needsLayout = true;

    // and ensure that we indeed are going to redraw
    Refresh();

    return true;
}

bool wxToolBar::DoDeleteTool(size_t WXUNUSED(pos),
                             wxToolBarToolBase * WXUNUSED(tool))
{
    // as above
    m_needsLayout = true;

    Refresh();

    return true;
}

void wxToolBar::DoEnableTool(wxToolBarToolBase *tool, bool enable)
{
#if wxUSE_IMAGE
    // created disabled-state bitmap on demand
    if ( !enable && !tool->GetDisabledBitmap().Ok() )
    {
        wxImage image(tool->GetNormalBitmap().ConvertToImage());

        tool->SetDisabledBitmap(image.ConvertToGreyscale());
    }
#endif // wxUSE_IMAGE

    RefreshTool(tool);
}

void wxToolBar::DoToggleTool(wxToolBarToolBase *tool, bool WXUNUSED(toggle))
{
    // note that if we're called the tool did change state (the base class
    // checks for it), so it's not necessary to check for this again here
    RefreshTool(tool);
}

void wxToolBar::DoSetToggle(wxToolBarToolBase *tool, bool WXUNUSED(toggle))
{
    RefreshTool(tool);
}

wxToolBarToolBase *wxToolBar::CreateTool(int id,
                                         const wxString& label,
                                         const wxBitmap& bmpNormal,
                                         const wxBitmap& bmpDisabled,
                                         wxItemKind kind,
                                         wxObject *clientData,
                                         const wxString& shortHelp,
                                         const wxString& longHelp)
{
    return new wxToolBarTool(this, id, label, bmpNormal, bmpDisabled, kind,
                             clientData, shortHelp, longHelp);
}

wxToolBarToolBase *wxToolBar::CreateTool(wxControl *control)
{
    return new wxToolBarTool(this, control);
}

// ----------------------------------------------------------------------------
// wxToolBar geometry
// ----------------------------------------------------------------------------

wxRect wxToolBar::GetToolRect(wxToolBarToolBase *toolBase) const
{
    const wxToolBarTool *tool = (wxToolBarTool *)toolBase;

    wxRect rect;

    wxCHECK_MSG( tool, rect, _T("GetToolRect: NULL tool") );

    // ensure that we always have the valid tool position
    if ( m_needsLayout )
    {
        wxConstCast(this, wxToolBar)->DoLayout();
    }

    rect.x = tool->m_x - m_xMargin;
    rect.y = tool->m_y - m_yMargin;

    if ( IsVertical() )
    {
        if (tool->IsButton())
        {
            if(!HasFlag(wxTB_TEXT))
            {
                rect.width = m_defaultWidth;
                rect.height = m_defaultHeight;
            }
            else
            {
                rect.width = m_defaultWidth +
                    GetFont().GetPointSize() * tool->GetLabel().length();
                rect.height = m_defaultHeight;
            }
        }
        else if (tool->IsSeparator())
        {
            rect.width = m_defaultWidth;
            rect.height = m_widthSeparator;
        }
        else // control
        {
            rect.width = tool->m_width;
            rect.height = tool->m_height;
        }
    }
    else // horizontal
    {
        if (tool->IsButton())
        {
            if(!HasFlag(wxTB_TEXT))
            {
                rect.width = m_defaultWidth;
                rect.height = m_defaultHeight;
            }
            else
            {
                rect.width = m_defaultWidth +
                    GetFont().GetPointSize() * tool->GetLabel().length();
                rect.height = m_defaultHeight;
            }
        }
        else if (tool->IsSeparator())
        {
            rect.width = m_widthSeparator;
            rect.height = m_defaultHeight;
        }
        else // control
        {
            rect.width = tool->m_width;
            rect.height = tool->m_height;
        }
    }

    rect.width += 2*m_xMargin;
    rect.height += 2*m_yMargin;

    return rect;
}

bool wxToolBar::Realize()
{
    if ( !wxToolBarBase::Realize() )
        return false;

    m_needsLayout = true;
    DoLayout();

    SetInitialSize(wxDefaultSize);

    return true;
}

void wxToolBar::SetWindowStyleFlag( long style )
{
    wxToolBarBase::SetWindowStyleFlag(style);

    m_needsLayout = true;

    Refresh();
}

void wxToolBar::DoLayout()
{
    wxASSERT_MSG( m_needsLayout, _T("why are we called?") );

    m_needsLayout = false;

    wxCoord x = m_xMargin,
            y = m_yMargin;

    wxCoord widthTool = 0, maxWidthTool = 0;
    wxCoord heightTool = 0, maxHeightTool = 0;
    wxCoord margin = IsVertical() ? m_xMargin : m_yMargin;
    wxCoord *pCur = IsVertical() ? &y : &x;

    // calculate the positions of all elements
    for ( wxToolBarToolsList::compatibility_iterator node = m_tools.GetFirst();
          node;
          node = node->GetNext() )
    {
        wxToolBarTool *tool = (wxToolBarTool *) node->GetData();

        tool->m_x = x;
        tool->m_y = y;

        // TODO ugly number fiddling
        if (tool->IsButton())
        {
            if (IsVertical())
            {
                widthTool = m_defaultHeight;
                heightTool = m_defaultWidth;
                if(HasFlag(wxTB_TEXT))
                    heightTool += GetFont().GetPointSize() * tool->GetLabel().length();
            }
            else
            {
                widthTool = m_defaultWidth;
                if(HasFlag(wxTB_TEXT))
                    widthTool += GetFont().GetPointSize() * tool->GetLabel().length();

                heightTool = m_defaultHeight;
            }

            if(widthTool > maxWidthTool) // Record max width of tool
            {
                maxWidthTool = widthTool;
            }

            if(heightTool > maxHeightTool) // Record max width of tool
            {
                maxHeightTool = heightTool;
            }

            *pCur += widthTool;
        }
        else if (tool->IsSeparator())
        {
            *pCur += m_widthSeparator;
        }
        else if (!IsVertical()) // horizontal control
        {
            wxControl *control = tool->GetControl();
            wxSize size = control->GetSize();
            tool->m_y += (m_defaultHeight - size.y)/2;
            tool->m_width = size.x;
            tool->m_height = size.y;

            *pCur += tool->m_width;
        }
        *pCur += margin;
    }

    // calculate the total toolbar size
    wxCoord xMin, yMin;

    if(!HasFlag(wxTB_TEXT))
    {
        xMin = m_defaultWidth + 2*m_xMargin;
        yMin = m_defaultHeight + 2*m_yMargin;
    }
    else
    {
        if (IsVertical())
        {
            xMin = heightTool + 2*m_xMargin;
            yMin = widthTool + 2*m_xMargin;
        }
        else
        {
            xMin = maxWidthTool + 2*m_xMargin;
            yMin = heightTool + 2*m_xMargin;
        }
    }

    m_maxWidth = x < xMin ? xMin : x;
    m_maxHeight = y < yMin ? yMin : y;
}

wxSize wxToolBar::DoGetBestClientSize() const
{
    return wxSize(m_maxWidth, m_maxHeight);
}

void wxToolBar::DoSetSize(int x, int y, int width, int height, int sizeFlags)
{
    int old_width, old_height;
    GetSize(&old_width, &old_height);

    wxToolBarBase::DoSetSize(x, y, width, height, sizeFlags);

    // Correct width and height if needed.
    if ( width == wxDefaultCoord || height == wxDefaultCoord )
    {
        int tmp_width, tmp_height;
        GetSize(&tmp_width, &tmp_height);

        if ( width == wxDefaultCoord )
            width = tmp_width;
        if ( height == wxDefaultCoord )
            height = tmp_height;
    }

    // We must refresh the frame size when the toolbar changes size
    // otherwise the toolbar can be shown incorrectly
    if ( old_width != width || old_height != height )
    {
        // But before we send the size event check it
        // we have a frame that is not being deleted.
        wxFrame *frame = wxDynamicCast(GetParent(), wxFrame);
        if ( frame && !frame->IsBeingDeleted() )
        {
            frame->SendSizeEvent();
        }
    }
}

// ----------------------------------------------------------------------------
// wxToolBar drawing
// ----------------------------------------------------------------------------

void wxToolBar::RefreshTool(wxToolBarToolBase *tool)
{
    RefreshRect(GetToolRect(tool));
}

void wxToolBar::GetRectLimits(const wxRect& rect,
                              wxCoord *start,
                              wxCoord *end) const
{
    wxCHECK_RET( start && end, _T("NULL pointer in GetRectLimits") );

    if ( IsVertical() )
    {
        *start = rect.GetTop();
        *end = rect.GetBottom();
    }
    else // horizontal
    {
        *start = rect.GetLeft();
        *end = rect.GetRight();
    }
}

void wxToolBar::DoDraw(wxControlRenderer *renderer)
{
    // prepare the variables used below
    wxDC& dc = renderer->GetDC();
    wxRenderer *rend = renderer->GetRenderer();
    dc.SetFont(GetFont());

    // draw the border separating us from the menubar (if there is no menubar
    // we probably shouldn't draw it?)
    if ( !IsVertical() )
    {
        rend->DrawHorizontalLine(dc, 0, 0, GetClientSize().x);
    }

    // get the update rect and its limits depending on the orientation
    wxRect rectUpdate = GetUpdateClientRect();
    wxCoord start, end;
    GetRectLimits(rectUpdate, &start, &end);

    // and redraw all the tools intersecting it
    for ( wxToolBarToolsList::compatibility_iterator node = m_tools.GetFirst();
          node;
          node = node->GetNext() )
    {
        wxToolBarTool *tool = (wxToolBarTool*) node->GetData();
        wxRect rectTool = GetToolRect(tool);
        wxCoord startTool, endTool;
        GetRectLimits(rectTool, &startTool, &endTool);

        if ( endTool < start )
        {
            // we're still to the left of the area to redraw
            continue;
        }

        if ( startTool > end )
        {
            // we're beyond the area to redraw, nothing left to do
            break;
        }

        if (tool->IsSeparator() && !HasFlag(wxTB_FLAT))
        {
            // Draw separators only in flat mode
            continue;
        }

        // deal with the flags
        int flags = 0;

        if ( tool->IsEnabled() )
        {
            // The toolbars without wxTB_FLAT don't react to the mouse hovering
            if ( !HasFlag(wxTB_FLAT) || tool->IsUnderMouse() )
                flags |= wxCONTROL_CURRENT;
        }
        else // disabled tool
        {
            flags |= wxCONTROL_DISABLED;
        }

        //if ( tool == m_toolCaptured )
        //    flags |= wxCONTROL_FOCUSED;

        if ( tool->IsPressed() )
            flags = wxCONTROL_PRESSED;

        wxString label;
        wxBitmap bitmap;
        if ( !tool->IsSeparator() )
        {
            label = tool->GetLabel();
            bitmap = tool->GetBitmap();
        }
        //else: leave both the label and the bitmap invalid to draw a separator

        if ( !tool->IsControl() )
        {
            int tbStyle = 0;
            if(HasFlag(wxTB_TEXT))
            {
                tbStyle |= wxTB_TEXT;
            }

            if(HasFlag(wxTB_VERTICAL))
            {
                tbStyle |= wxTB_VERTICAL;
            }
            else
            {
                tbStyle |= wxTB_HORIZONTAL;
            }
            rend->DrawToolBarButton(dc, label, bitmap, rectTool, flags, tool->GetStyle(), tbStyle);
        }
        else // control
        {
            wxControl *control = tool->GetControl();
            control->Move(tool->m_x, tool->m_y);
        }
    }
}

// ----------------------------------------------------------------------------
// wxToolBar actions
// ----------------------------------------------------------------------------

bool wxToolBar::PerformAction(const wxControlAction& action,
                              long numArg,
                              const wxString& strArg)
{
    wxToolBarTool *tool = (wxToolBarTool*) FindById(numArg);
    if (!tool)
        return false;

    if ( action == wxACTION_TOOLBAR_TOGGLE )
    {
        PerformAction( wxACTION_BUTTON_RELEASE, numArg );

        PerformAction( wxACTION_BUTTON_CLICK, numArg );

        // Write by Danny Raynor to change state again.
        // Check button still pressed or not
        if ( tool->CanBeToggled() && tool->IsToggled() )
        {
            tool->Toggle(false);
        }

        if( tool->IsInverted() )
        {
            PerformAction( wxACTION_TOOLBAR_RELEASE, numArg );
        }

        // Set mouse leave toolbar button range (If still in the range,
        // toolbar button would get focus again
        PerformAction( wxACTION_TOOLBAR_LEAVE, numArg );
    }
    else if ( action == wxACTION_TOOLBAR_PRESS )
    {
        wxLogTrace(_T("toolbar"), _T("Button '%s' pressed."), tool->GetShortHelp().c_str());

        tool->Invert();

        RefreshTool( tool );
    }
    else if ( action == wxACTION_TOOLBAR_RELEASE )
    {
        wxLogTrace(_T("toolbar"), _T("Button '%s' released."), tool->GetShortHelp().c_str());

        wxASSERT_MSG( tool->IsInverted(), _T("release unpressed button?") );

        tool->Invert();

        RefreshTool( tool );
    }
    else if ( action == wxACTION_TOOLBAR_CLICK )
    {
        bool isToggled;
        if ( tool->CanBeToggled() )
        {
            tool->Toggle();

            RefreshTool( tool );

            isToggled = tool->IsToggled();
        }
        else // simple non-checkable tool
        {
            isToggled = false;
        }
        OnLeftClick( tool->GetId(), isToggled );
    }
    else if ( action == wxACTION_TOOLBAR_ENTER )
    {
        wxCHECK_MSG( tool, false, _T("no tool to enter?") );

        if ( HasFlag(wxTB_FLAT) && tool->IsEnabled() )
        {
            tool->SetUnderMouse( true );

            if ( !tool->IsToggled() )
                RefreshTool( tool );
        }
    }
    else if ( action == wxACTION_TOOLBAR_LEAVE )
    {
        wxCHECK_MSG( tool, false, _T("no tool to leave?") );

        if ( HasFlag(wxTB_FLAT) && tool->IsEnabled() )
        {
            tool->SetUnderMouse( false );

            if ( !tool->IsToggled() )
                RefreshTool( tool );
        }
    }
    else
        return wxControl::PerformAction(action, numArg, strArg);

    return true;
}

/* static */
wxInputHandler *wxToolBar::GetStdInputHandler(wxInputHandler *handlerDef)
{
    static wxStdToolbarInputHandler s_handler(handlerDef);

    return &s_handler;
}

// ============================================================================
// wxStdToolbarInputHandler implementation
// ============================================================================

wxStdToolbarInputHandler::wxStdToolbarInputHandler(wxInputHandler *handler)
                        : wxStdInputHandler(handler)
{
    m_winCapture = NULL;
    m_toolCapture = NULL;
    m_toolLast = NULL;
}

bool wxStdToolbarInputHandler::HandleKey(wxInputConsumer *consumer,
                                         const wxKeyEvent& event,
                                         bool pressed)
{
    // TODO: when we have a current button we should allow the arrow
    //       keys to move it
    return wxStdInputHandler::HandleKey(consumer, event, pressed);
}

bool wxStdToolbarInputHandler::HandleMouse(wxInputConsumer *consumer,
                                           const wxMouseEvent& event)
{
    wxToolBar *tbar = wxStaticCast(consumer->GetInputWindow(), wxToolBar);
    wxToolBarToolBase *tool = tbar->FindToolForPosition(event.GetX(), event.GetY());

    if ( event.Button(1) )
    {

        if ( event.LeftDown() || event.LeftDClick() )
        {
            if ( !tool || !tool->IsEnabled() )
                return true;

            m_winCapture = tbar;
            m_winCapture->CaptureMouse();

            m_toolCapture = tool;

            consumer->PerformAction( wxACTION_BUTTON_PRESS, tool->GetId() );

            return true;
        }
        else if ( event.LeftUp() )
        {
            if ( m_winCapture )
            {
                m_winCapture->ReleaseMouse();
                m_winCapture = NULL;
            }

            if (m_toolCapture)
            {
                if ( tool == m_toolCapture )
                    consumer->PerformAction( wxACTION_BUTTON_TOGGLE, m_toolCapture->GetId() );
                else
                    consumer->PerformAction( wxACTION_TOOLBAR_LEAVE, m_toolCapture->GetId() );
            }

            m_toolCapture = NULL;

            return true;
        }
        //else: don't do anything special about the double click
    }

    return wxStdInputHandler::HandleMouse(consumer, event);
}

bool wxStdToolbarInputHandler::HandleMouseMove(wxInputConsumer *consumer,
                                               const wxMouseEvent& event)
{
    if ( !wxStdInputHandler::HandleMouseMove(consumer, event) )
    {
        wxToolBar *tbar = wxStaticCast(consumer->GetInputWindow(), wxToolBar);

        wxToolBarTool *tool;
        if ( event.Leaving() )
        {
            // We cannot possibly be over a tool when
            // leaving the toolbar
            tool = NULL;
        }
        else
        {
            tool = (wxToolBarTool*) tbar->FindToolForPosition( event.GetX(), event.GetY() );
        }

        if (m_toolCapture)
        {
            // During capture we only care of the captured tool
            if (tool && (tool != m_toolCapture))
                tool = NULL;

            if (tool == m_toolLast)
                return true;

            if (tool)
                consumer->PerformAction( wxACTION_BUTTON_PRESS, m_toolCapture->GetId() );
            else
                consumer->PerformAction( wxACTION_BUTTON_RELEASE, m_toolCapture->GetId() );

            m_toolLast = tool;
        }
        else
        {
            if (tool == m_toolLast)
               return true;

            if (m_toolLast)
            {
                // Leave old tool if any
                consumer->PerformAction( wxACTION_TOOLBAR_LEAVE, m_toolLast->GetId() );
            }

            if (tool)
            {
                // Enter new tool if any
                consumer->PerformAction( wxACTION_TOOLBAR_ENTER, tool->GetId() );
            }

            m_toolLast = tool;
        }

        return true;
    }

    return false;
}

bool wxStdToolbarInputHandler::HandleFocus(wxInputConsumer *consumer,
                                           const wxFocusEvent& WXUNUSED(event))
{
    if ( m_toolCapture )
    {
        // We shouldn't be left with a highlighted button
        consumer->PerformAction( wxACTION_TOOLBAR_LEAVE, m_toolCapture->GetId() );
    }

    return true;
}

bool wxStdToolbarInputHandler::HandleActivation(wxInputConsumer *consumer,
                                                bool activated)
{
    if (m_toolCapture && !activated)
    {
        // We shouldn't be left with a highlighted button
        consumer->PerformAction( wxACTION_TOOLBAR_LEAVE, m_toolCapture->GetId() );
    }

    return true;
}

#endif // wxUSE_TOOLBAR
