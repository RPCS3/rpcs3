/////////////////////////////////////////////////////////////////////////////
// Name:        src/generic/buttonbar.cpp
// Purpose:     wxButtonToolBar implementation
// Author:      Julian Smart, after Robert Roebling, Vadim Zeitlin, SciTech
// Modified by:
// Created:     2006-04-13
// Id:          $Id: buttonbar.cpp 42816 2006-10-31 08:50:17Z RD $
// Copyright:   (c) Julian Smart, Robert Roebling, Vadim Zeitlin,
//              SciTech Software, Inc.
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

// Currently, only for Mac as a toolbar replacement.
#if defined(__WXMAC__) && wxUSE_TOOLBAR && wxUSE_BMPBUTTON

#include "wx/generic/buttonbar.h"

#ifndef WX_PRECOMP
    #include "wx/utils.h"
    #include "wx/app.h"
    #include "wx/log.h"
    #include "wx/frame.h"
    #include "wx/dcclient.h"
    #include "wx/settings.h"
    #include "wx/image.h"
#endif

// ----------------------------------------------------------------------------
// wxButtonToolBarTool: our implementation of wxToolBarToolBase
// ----------------------------------------------------------------------------

class WXDLLEXPORT wxButtonToolBarTool : public wxToolBarToolBase
{
public:
    wxButtonToolBarTool(wxButtonToolBar *tbar,
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
        m_x = m_y = wxDefaultCoord;
        m_width =
        m_height = 0;

        m_button = NULL;
    }

    wxButtonToolBarTool(wxButtonToolBar *tbar, wxControl *control)
        : wxToolBarToolBase(tbar, control)
    {
        m_x = m_y = wxDefaultCoord;
        m_width =
        m_height = 0;
        m_button = NULL;
    }

    wxBitmapButton* GetButton() const { return m_button; }
    void SetButton(wxBitmapButton* button) { m_button = button; }

public:
    // the tool position (for controls)
    wxCoord m_x;
    wxCoord m_y;
    wxCoord m_width;
    wxCoord m_height;

private:
    // the control representing the button
    wxBitmapButton* m_button;
};

// ============================================================================
// wxButtonToolBar implementation
// ============================================================================

IMPLEMENT_DYNAMIC_CLASS(wxButtonToolBar, wxControl)

BEGIN_EVENT_TABLE(wxButtonToolBar, wxControl)
    EVT_BUTTON(wxID_ANY, wxButtonToolBar::OnCommand)
    EVT_PAINT(wxButtonToolBar::OnPaint)
    EVT_LEFT_UP(wxButtonToolBar::OnLeftUp)
END_EVENT_TABLE()

// ----------------------------------------------------------------------------
// wxButtonToolBar creation
// ----------------------------------------------------------------------------

void wxButtonToolBar::Init()
{
    // no tools yet
    m_needsLayout = false;

    // unknown widths for the tools and separators
    m_widthSeparator = wxDefaultCoord;

    m_maxWidth = m_maxHeight = 0;

    m_labelMargin = 2;
    m_labelHeight = 0;

    SetMargins(8, 2);
    SetToolPacking(8);
}

bool wxButtonToolBar::Create(wxWindow *parent,
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

    // wxColour lightBackground(244, 244, 244);

    wxFont font(wxSMALL_FONT->GetPointSize(), wxNORMAL_FONT->GetFamily(), wxNORMAL_FONT->GetStyle(), wxNORMAL);
    SetFont(font);

    // Calculate the label height if necessary
    if (GetWindowStyle() & wxTB_TEXT)
    {
        wxClientDC dc(this);
        dc.SetFont(font);
        int w, h;
        dc.GetTextExtent(wxT("X"), & w, & h);
        m_labelHeight = h;
    }
    return true;
}

wxButtonToolBar::~wxButtonToolBar()
{
}

// ----------------------------------------------------------------------------
// wxButtonToolBar tool-related methods
// ----------------------------------------------------------------------------

wxToolBarToolBase *wxButtonToolBar::FindToolForPosition(wxCoord x, wxCoord y) const
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
        wxButtonToolBarTool *tool =  (wxButtonToolBarTool*) node->GetData();
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

void wxButtonToolBar::GetRectLimits(const wxRect& rect,
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


void wxButtonToolBar::SetToolShortHelp(int id, const wxString& help)
{
    wxToolBarToolBase *tool = FindById(id);

    wxCHECK_RET( tool, _T("SetToolShortHelp: no such tool") );

    // TODO: set tooltip/short help
    tool->SetShortHelp(help);
}

bool wxButtonToolBar::DoInsertTool(size_t WXUNUSED(pos),
                             wxToolBarToolBase * WXUNUSED(tool))
{
    return true;
}

bool wxButtonToolBar::DoDeleteTool(size_t WXUNUSED(pos),
                             wxToolBarToolBase * WXUNUSED(tool))
{
    // TODO
    return true;
}

void wxButtonToolBar::DoEnableTool(wxToolBarToolBase *WXUNUSED(tool), bool WXUNUSED(enable))
{
    // TODO
}

void wxButtonToolBar::DoToggleTool(wxToolBarToolBase *WXUNUSED(tool), bool WXUNUSED(toggle))
{
    // TODO
}

void wxButtonToolBar::DoSetToggle(wxToolBarToolBase *WXUNUSED(tool), bool WXUNUSED(toggle))
{
    // TODO
}

wxToolBarToolBase *wxButtonToolBar::CreateTool(int id,
                                         const wxString& label,
                                         const wxBitmap& bmpNormal,
                                         const wxBitmap& bmpDisabled,
                                         wxItemKind kind,
                                         wxObject *clientData,
                                         const wxString& shortHelp,
                                         const wxString& longHelp)
{
    return new wxButtonToolBarTool(this, id, label, bmpNormal, bmpDisabled, kind,
                             clientData, shortHelp, longHelp);
}

wxToolBarToolBase *wxButtonToolBar::CreateTool(wxControl *control)
{
    return new wxButtonToolBarTool(this, control);
}

// ----------------------------------------------------------------------------
// wxButtonToolBar geometry
// ----------------------------------------------------------------------------

wxRect wxButtonToolBar::GetToolRect(wxToolBarToolBase *toolBase) const
{
    const wxButtonToolBarTool *tool = (wxButtonToolBarTool *)toolBase;

    wxRect rect;

    wxCHECK_MSG( tool, rect, _T("GetToolRect: NULL tool") );

    // ensure that we always have the valid tool position
    if ( m_needsLayout )
    {
        wxConstCast(this, wxButtonToolBar)->DoLayout();
    }

    rect.x = tool->m_x - (m_toolPacking/2);
    rect.y = tool->m_y;

    if ( IsVertical() )
    {
        if (tool->IsButton())
        {
            rect.width = m_defaultWidth;
            rect.height = m_defaultHeight;
            if (tool->GetButton())
                rect.SetSize(wxSize(tool->m_width, tool->m_height));
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
            rect.width = m_defaultWidth;
            rect.height = m_defaultHeight;
            if (tool->GetButton())
                rect.SetSize(wxSize(tool->m_width, tool->m_height));
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

    rect.width += m_toolPacking;

    return rect;
}

bool wxButtonToolBar::Realize()
{
    if ( !wxToolBarBase::Realize() )
        return false;

    m_needsLayout = true;
    DoLayout();

    SetInitialSize(wxSize(m_maxWidth, m_maxHeight));

    return true;
}

void wxButtonToolBar::DoLayout()
{
    m_needsLayout = false;

    wxCoord x = m_xMargin,
            y = m_yMargin;

    int maxHeight = 0;

    const wxCoord widthTool = IsVertical() ? m_defaultHeight : m_defaultWidth;
    wxCoord margin = IsVertical() ? m_xMargin : m_yMargin;
    wxCoord *pCur = IsVertical() ? &y : &x;

    // calculate the positions of all elements
    for ( wxToolBarToolsList::compatibility_iterator node = m_tools.GetFirst();
          node;
          node = node->GetNext() )
    {
        wxButtonToolBarTool *tool = (wxButtonToolBarTool *) node->GetData();

        tool->m_x = x;
        tool->m_y = y;

        if (tool->IsButton())
        {
            if (!tool->GetButton())
            {
                wxBitmapButton* bmpButton = new wxBitmapButton(this, tool->GetId(), tool->GetNormalBitmap(), wxPoint(tool->m_x, tool->m_y), wxDefaultSize,
                                                               wxBU_AUTODRAW|wxBORDER_NONE);
                if (!tool->GetShortHelp().empty())
                    bmpButton->SetLabel(tool->GetShortHelp());

                tool->SetButton(bmpButton);
            }
            else
            {
                tool->GetButton()->Move(wxPoint(tool->m_x, tool->m_y));
            }

            int w = widthTool;
            if (tool->GetButton())
            {
                wxSize sz = tool->GetButton()->GetSize();
                w = sz.x;

                if (m_labelHeight > 0)
                {
                    sz.y += (m_labelHeight + m_labelMargin);

                    if (!tool->GetShortHelp().empty())
                    {
                        wxClientDC dc(this);
                        dc.SetFont(GetFont());
                        int tw, th;
                        dc.GetTextExtent(tool->GetShortHelp(), & tw, & th);

                        // If the label is bigger than the icon, the label width
                        // becomes the new tool width, and we need to centre the
                        // the bitmap in this box.
                        if (tw > sz.x)
                        {
                            int newX = int(tool->m_x + (tw - sz.x)/2.0);
                            tool->GetButton()->Move(newX, tool->m_y);
                            sz.x = tw;
                        }
                    }
                }

                maxHeight = wxMax(maxHeight, sz.y);

                tool->m_width = sz.x;
                tool->m_height = sz.y;
                w = sz.x;
            }

            *pCur += (w + GetToolPacking());
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

            maxHeight = wxMax(maxHeight, size.y);
        }
        *pCur += margin;
    }

    // calculate the total toolbar size
    m_maxWidth = x + 2*m_xMargin;
    m_maxHeight = maxHeight + 2*m_yMargin;

    if ((GetWindowStyle() & wxTB_NODIVIDER) == 0)
        m_maxHeight += 2;

}

wxSize wxButtonToolBar::DoGetBestClientSize() const
{
    return wxSize(m_maxWidth, m_maxHeight);
}

// receives button commands
void wxButtonToolBar::OnCommand(wxCommandEvent& event)
{
    wxButtonToolBarTool* tool = (wxButtonToolBarTool*) FindById(event.GetId());
    if (!tool)
    {
        event.Skip();
        return;
    }

    if (tool->CanBeToggled())
        tool->Toggle(tool->IsToggled());

    // TODO: handle toggle items
    OnLeftClick(event.GetId(), false);

    if (tool->GetKind() == wxITEM_RADIO)
        UnToggleRadioGroup(tool);

    if (tool->CanBeToggled())
        Refresh();
}

// paints a border
void wxButtonToolBar::OnPaint(wxPaintEvent& event)
{
    wxPaintDC dc(this);

    dc.SetFont(GetFont());
    dc.SetBackgroundMode(wxTRANSPARENT);

    for ( wxToolBarToolsList::compatibility_iterator node = m_tools.GetFirst();
          node;
          node = node->GetNext() )
    {
        wxButtonToolBarTool *tool =  (wxButtonToolBarTool*) node->GetData();
        wxRect rectTool = GetToolRect(tool);
        if (tool->IsToggled())
        {
            wxRect backgroundRect = rectTool;
            backgroundRect.y = -1; backgroundRect.height = GetClientSize().y + 1;
            wxBrush brush(wxColour(219, 219, 219));
            wxPen pen(wxColour(159, 159, 159));
            dc.SetBrush(brush);
            dc.SetPen(pen);
            dc.DrawRectangle(backgroundRect);
        }

        if (m_labelHeight > 0 && !tool->GetShortHelp().empty())
        {
            int tw, th;
            dc.GetTextExtent(tool->GetShortHelp(), & tw, & th);

            int x = tool->m_x;
            dc.DrawText(tool->GetShortHelp(), x, tool->m_y + tool->GetButton()->GetSize().y + m_labelMargin);
        }
    }

    if ((GetWindowStyle() & wxTB_NODIVIDER) == 0)
    {
        wxPen pen(wxColour(159, 159, 159));
        dc.SetPen(pen);
        int x1 = 0;
        int y1 = GetClientSize().y-1;
        int x2 = GetClientSize().x;
        int y2 = y1;
        dc.DrawLine(x1, y1, x2, y2);
    }
}

// detects mouse clicks outside buttons
void wxButtonToolBar::OnLeftUp(wxMouseEvent& event)
{
    if (m_labelHeight > 0)
    {
        wxButtonToolBarTool* tool = (wxButtonToolBarTool*) FindToolForPosition(event.GetX(), event.GetY());
        if (tool && tool->GetButton() && (event.GetY() > (tool->m_y + tool->GetButton()->GetSize().y)))
        {
            wxCommandEvent event(wxEVT_COMMAND_BUTTON_CLICKED, tool->GetId());
            event.SetEventObject(tool->GetButton());
            if (!ProcessEvent(event))
                event.Skip();
        }
    }
}

#endif // wxUSE_TOOLBAR && wxUSE_BMPBUTTON
