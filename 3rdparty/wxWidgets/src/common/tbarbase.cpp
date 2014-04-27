/////////////////////////////////////////////////////////////////////////////
// Name:        src/common/tbarbase.cpp
// Purpose:     wxToolBarBase implementation
// Author:      Julian Smart
// Modified by: VZ at 11.12.99 (wxScrollableToolBar split off)
// Created:     04/01/98
// RCS-ID:      $Id: tbarbase.cpp 42840 2006-10-31 13:09:08Z VZ $
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

#if wxUSE_TOOLBAR

#include "wx/toolbar.h"

#ifndef WX_PRECOMP
    #include "wx/control.h"
    #include "wx/frame.h"
    #include "wx/settings.h"
    #include "wx/image.h"
#endif

// ----------------------------------------------------------------------------
// wxWidgets macros
// ----------------------------------------------------------------------------

BEGIN_EVENT_TABLE(wxToolBarBase, wxControl)
END_EVENT_TABLE()

#include "wx/listimpl.cpp"

WX_DEFINE_LIST(wxToolBarToolsList)

// ============================================================================
// implementation
// ============================================================================

// ----------------------------------------------------------------------------
// wxToolBarToolBase
// ----------------------------------------------------------------------------

IMPLEMENT_DYNAMIC_CLASS(wxToolBarToolBase, wxObject)

bool wxToolBarToolBase::Enable(bool enable)
{
    if ( m_enabled == enable )
        return false;

    m_enabled = enable;

    return true;
}

bool wxToolBarToolBase::Toggle(bool toggle)
{
    wxASSERT_MSG( CanBeToggled(), _T("can't toggle this tool") );

    if ( m_toggled == toggle )
        return false;

    m_toggled = toggle;

    return true;
}

bool wxToolBarToolBase::SetToggle(bool toggle)
{
    wxItemKind kind = toggle ? wxITEM_CHECK : wxITEM_NORMAL;
    if ( m_kind == kind )
        return false;

    m_kind = kind;

    return true;
}

bool wxToolBarToolBase::SetShortHelp(const wxString& help)
{
    if ( m_shortHelpString == help )
        return false;

    m_shortHelpString = help;

    return true;
}

bool wxToolBarToolBase::SetLongHelp(const wxString& help)
{
    if ( m_longHelpString == help )
        return false;

    m_longHelpString = help;

    return true;
}

// ----------------------------------------------------------------------------
// wxToolBarBase adding/deleting items
// ----------------------------------------------------------------------------

wxToolBarBase::wxToolBarBase()
{
    // the list owns the pointers
    m_xMargin = m_yMargin = 0;
    m_maxRows = m_maxCols = 0;
    m_toolPacking = m_toolSeparation = 0;
    m_defaultWidth = 16;
    m_defaultHeight = 15;
}

void wxToolBarBase::FixupStyle()
{
    if ( !HasFlag(wxTB_TOP | wxTB_LEFT | wxTB_RIGHT | wxTB_BOTTOM) )
    {
        // this is the default
        m_windowStyle |= wxTB_TOP;
    }
}

wxToolBarToolBase *wxToolBarBase::DoAddTool(int id,
                                            const wxString& label,
                                            const wxBitmap& bitmap,
                                            const wxBitmap& bmpDisabled,
                                            wxItemKind kind,
                                            const wxString& shortHelp,
                                            const wxString& longHelp,
                                            wxObject *clientData,
                                            wxCoord WXUNUSED(xPos),
                                            wxCoord WXUNUSED(yPos))
{
    InvalidateBestSize();
    return InsertTool(GetToolsCount(), id, label, bitmap, bmpDisabled,
                      kind, shortHelp, longHelp, clientData);
}

wxToolBarToolBase *wxToolBarBase::InsertTool(size_t pos,
                                             int id,
                                             const wxString& label,
                                             const wxBitmap& bitmap,
                                             const wxBitmap& bmpDisabled,
                                             wxItemKind kind,
                                             const wxString& shortHelp,
                                             const wxString& longHelp,
                                             wxObject *clientData)
{
    wxCHECK_MSG( pos <= GetToolsCount(), (wxToolBarToolBase *)NULL,
                 _T("invalid position in wxToolBar::InsertTool()") );

    wxToolBarToolBase *tool = CreateTool(id, label, bitmap, bmpDisabled, kind,
                                         clientData, shortHelp, longHelp);

    if ( !InsertTool(pos, tool) )
    {
        delete tool;

        return NULL;
    }

    return tool;
}

wxToolBarToolBase *wxToolBarBase::AddTool(wxToolBarToolBase *tool)
{
    return InsertTool(GetToolsCount(), tool);
}

wxToolBarToolBase *
wxToolBarBase::InsertTool(size_t pos, wxToolBarToolBase *tool)
{
    wxCHECK_MSG( pos <= GetToolsCount(), (wxToolBarToolBase *)NULL,
                 _T("invalid position in wxToolBar::InsertTool()") );

    if ( !tool || !DoInsertTool(pos, tool) )
    {
        return NULL;
    }

    m_tools.Insert(pos, tool);

    return tool;
}

wxToolBarToolBase *wxToolBarBase::AddControl(wxControl *control)
{
    return InsertControl(GetToolsCount(), control);
}

wxToolBarToolBase *wxToolBarBase::InsertControl(size_t pos, wxControl *control)
{
    wxCHECK_MSG( control, (wxToolBarToolBase *)NULL,
                 _T("toolbar: can't insert NULL control") );

    wxCHECK_MSG( control->GetParent() == this, (wxToolBarToolBase *)NULL,
                 _T("control must have toolbar as parent") );

    wxCHECK_MSG( pos <= GetToolsCount(), (wxToolBarToolBase *)NULL,
                 _T("invalid position in wxToolBar::InsertControl()") );

    wxToolBarToolBase *tool = CreateTool(control);

    if ( !InsertTool(pos, tool) )
    {
        delete tool;

        return NULL;
    }

    return tool;
}

wxControl *wxToolBarBase::FindControl( int id )
{
    for ( wxToolBarToolsList::compatibility_iterator node = m_tools.GetFirst();
          node;
          node = node->GetNext() )
    {
        const wxToolBarToolBase * const tool = node->GetData();
        if ( tool->IsControl() )
        {
            wxControl * const control = tool->GetControl();

            if ( !control )
            {
                wxFAIL_MSG( _T("NULL control in toolbar?") );
            }
            else if ( control->GetId() == id )
            {
                // found
                return control;
            }
        }
    }

   return NULL;
}

wxToolBarToolBase *wxToolBarBase::AddSeparator()
{
    return InsertSeparator(GetToolsCount());
}

wxToolBarToolBase *wxToolBarBase::InsertSeparator(size_t pos)
{
    wxCHECK_MSG( pos <= GetToolsCount(), (wxToolBarToolBase *)NULL,
                 _T("invalid position in wxToolBar::InsertSeparator()") );

    wxToolBarToolBase *tool = CreateTool(wxID_SEPARATOR,
                                         wxEmptyString,
                                         wxNullBitmap, wxNullBitmap,
                                         wxITEM_SEPARATOR, (wxObject *)NULL,
                                         wxEmptyString, wxEmptyString);

    if ( !tool || !DoInsertTool(pos, tool) )
    {
        delete tool;

        return NULL;
    }

    m_tools.Insert(pos, tool);

    return tool;
}

wxToolBarToolBase *wxToolBarBase::RemoveTool(int id)
{
    size_t pos = 0;
    wxToolBarToolsList::compatibility_iterator node;
    for ( node = m_tools.GetFirst(); node; node = node->GetNext() )
    {
        if ( node->GetData()->GetId() == id )
            break;

        pos++;
    }

    if ( !node )
    {
        // don't give any error messages - sometimes we might call RemoveTool()
        // without knowing whether the tool is or not in the toolbar
        return (wxToolBarToolBase *)NULL;
    }

    wxToolBarToolBase *tool = node->GetData();
    if ( !DoDeleteTool(pos, tool) )
    {
        return (wxToolBarToolBase *)NULL;
    }

    m_tools.Erase(node);

    return tool;
}

bool wxToolBarBase::DeleteToolByPos(size_t pos)
{
    wxCHECK_MSG( pos < GetToolsCount(), false,
                 _T("invalid position in wxToolBar::DeleteToolByPos()") );

    wxToolBarToolsList::compatibility_iterator node = m_tools.Item(pos);

    if ( !DoDeleteTool(pos, node->GetData()) )
    {
        return false;
    }

    delete node->GetData();
    m_tools.Erase(node);

    return true;
}

bool wxToolBarBase::DeleteTool(int id)
{
    size_t pos = 0;
    wxToolBarToolsList::compatibility_iterator node;
    for ( node = m_tools.GetFirst(); node; node = node->GetNext() )
    {
        if ( node->GetData()->GetId() == id )
            break;

        pos++;
    }

    if ( !node || !DoDeleteTool(pos, node->GetData()) )
    {
        return false;
    }

    delete node->GetData();
    m_tools.Erase(node);

    return true;
}

wxToolBarToolBase *wxToolBarBase::FindById(int id) const
{
    wxToolBarToolBase *tool = (wxToolBarToolBase *)NULL;

    for ( wxToolBarToolsList::compatibility_iterator node = m_tools.GetFirst();
          node;
          node = node->GetNext() )
    {
        tool = node->GetData();
        if ( tool->GetId() == id )
        {
            // found
            break;
        }

        tool = NULL;
    }

    return tool;
}

void wxToolBarBase::UnToggleRadioGroup(wxToolBarToolBase *tool)
{
    wxCHECK_RET( tool, _T("NULL tool in wxToolBarTool::UnToggleRadioGroup") );

    if ( !tool->IsButton() || tool->GetKind() != wxITEM_RADIO )
        return;

    wxToolBarToolsList::compatibility_iterator node = m_tools.Find(tool);
    wxCHECK_RET( node, _T("invalid tool in wxToolBarTool::UnToggleRadioGroup") );

    wxToolBarToolsList::compatibility_iterator nodeNext = node->GetNext();
    while ( nodeNext )
    {
        wxToolBarToolBase *toolNext = nodeNext->GetData();

        if ( !toolNext->IsButton() || toolNext->GetKind() != wxITEM_RADIO )
            break;

        if ( toolNext->Toggle(false) )
        {
            DoToggleTool(toolNext, false);
        }

        nodeNext = nodeNext->GetNext();
    }

    wxToolBarToolsList::compatibility_iterator nodePrev = node->GetPrevious();
    while ( nodePrev )
    {
        wxToolBarToolBase *toolNext = nodePrev->GetData();

        if ( !toolNext->IsButton() || toolNext->GetKind() != wxITEM_RADIO )
            break;

        if ( toolNext->Toggle(false) )
        {
            DoToggleTool(toolNext, false);
        }

        nodePrev = nodePrev->GetPrevious();
    }
}

void wxToolBarBase::ClearTools()
{
    while ( GetToolsCount() )
    {
        DeleteToolByPos(0);
    }
}

bool wxToolBarBase::Realize()
{
    return true;
}

wxToolBarBase::~wxToolBarBase()
{
    WX_CLEAR_LIST(wxToolBarToolsList, m_tools);

    // notify the frame that it doesn't have a tool bar any longer to avoid
    // dangling pointers
    wxFrame *frame = wxDynamicCast(GetParent(), wxFrame);
    if ( frame && frame->GetToolBar() == this )
    {
        frame->SetToolBar(NULL);
    }
}

// ----------------------------------------------------------------------------
// wxToolBarBase tools state
// ----------------------------------------------------------------------------

void wxToolBarBase::EnableTool(int id, bool enable)
{
    wxToolBarToolBase *tool = FindById(id);
    if ( tool )
    {
        if ( tool->Enable(enable) )
        {
            DoEnableTool(tool, enable);
        }
    }
}

void wxToolBarBase::ToggleTool(int id, bool toggle)
{
    wxToolBarToolBase *tool = FindById(id);
    if ( tool && tool->CanBeToggled() )
    {
        if ( tool->Toggle(toggle) )
        {
            UnToggleRadioGroup(tool);
            DoToggleTool(tool, toggle);
        }
    }
}

void wxToolBarBase::SetToggle(int id, bool toggle)
{
    wxToolBarToolBase *tool = FindById(id);
    if ( tool )
    {
        if ( tool->SetToggle(toggle) )
        {
            DoSetToggle(tool, toggle);
        }
    }
}

void wxToolBarBase::SetToolShortHelp(int id, const wxString& help)
{
    wxToolBarToolBase *tool = FindById(id);
    if ( tool )
    {
        (void)tool->SetShortHelp(help);
    }
}

void wxToolBarBase::SetToolLongHelp(int id, const wxString& help)
{
    wxToolBarToolBase *tool = FindById(id);
    if ( tool )
    {
        (void)tool->SetLongHelp(help);
    }
}

wxObject *wxToolBarBase::GetToolClientData(int id) const
{
    wxToolBarToolBase *tool = FindById(id);

    return tool ? tool->GetClientData() : (wxObject *)NULL;
}

void wxToolBarBase::SetToolClientData(int id, wxObject *clientData)
{
    wxToolBarToolBase *tool = FindById(id);

    wxCHECK_RET( tool, _T("no such tool in wxToolBar::SetToolClientData") );

    tool->SetClientData(clientData);
}

int wxToolBarBase::GetToolPos(int id) const
{
    size_t pos = 0;
    wxToolBarToolsList::compatibility_iterator node;

    for ( node = m_tools.GetFirst(); node; node = node->GetNext() )
    {
        if ( node->GetData()->GetId() == id )
            return pos;

        pos++;
    }

    return wxNOT_FOUND;
}

bool wxToolBarBase::GetToolState(int id) const
{
    wxToolBarToolBase *tool = FindById(id);
    wxCHECK_MSG( tool, false, _T("no such tool") );

    return tool->IsToggled();
}

bool wxToolBarBase::GetToolEnabled(int id) const
{
    wxToolBarToolBase *tool = FindById(id);
    wxCHECK_MSG( tool, false, _T("no such tool") );

    return tool->IsEnabled();
}

wxString wxToolBarBase::GetToolShortHelp(int id) const
{
    wxToolBarToolBase *tool = FindById(id);
    wxCHECK_MSG( tool, wxEmptyString, _T("no such tool") );

    return tool->GetShortHelp();
}

wxString wxToolBarBase::GetToolLongHelp(int id) const
{
    wxToolBarToolBase *tool = FindById(id);
    wxCHECK_MSG( tool, wxEmptyString, _T("no such tool") );

    return tool->GetLongHelp();
}

// ----------------------------------------------------------------------------
// wxToolBarBase geometry
// ----------------------------------------------------------------------------

void wxToolBarBase::SetMargins(int x, int y)
{
    m_xMargin = x;
    m_yMargin = y;
}

void wxToolBarBase::SetRows(int WXUNUSED(nRows))
{
    // nothing
}

// ----------------------------------------------------------------------------
// event processing
// ----------------------------------------------------------------------------

// Only allow toggle if returns true
bool wxToolBarBase::OnLeftClick(int id, bool toggleDown)
{
    wxCommandEvent event(wxEVT_COMMAND_TOOL_CLICKED, id);
    event.SetEventObject(this);

    // we use SetInt() to make wxCommandEvent::IsChecked() return toggleDown
    event.SetInt((int)toggleDown);

    // and SetExtraLong() for backwards compatibility
    event.SetExtraLong((long)toggleDown);

    // Send events to this toolbar instead (and thence up the window hierarchy)
    GetEventHandler()->ProcessEvent(event);

    return true;
}

// Call when right button down.
void wxToolBarBase::OnRightClick(int id,
                                 long WXUNUSED(x),
                                 long WXUNUSED(y))
{
    wxCommandEvent event(wxEVT_COMMAND_TOOL_RCLICKED, id);
    event.SetEventObject(this);
    event.SetInt(id);

    GetEventHandler()->ProcessEvent(event);
}

// Called when the mouse cursor enters a tool bitmap (no button pressed).
// Argument is wxID_ANY if mouse is exiting the toolbar.
// Note that for this event, the id of the window is used,
// and the integer parameter of wxCommandEvent is used to retrieve
// the tool id.
void wxToolBarBase::OnMouseEnter(int id)
{
    wxCommandEvent event(wxEVT_COMMAND_TOOL_ENTER, GetId());
    event.SetEventObject(this);
    event.SetInt(id);

    wxFrame *frame = wxDynamicCast(GetParent(), wxFrame);
    if( frame )
    {
        wxString help;
        wxToolBarToolBase* tool = id == wxID_ANY ? (wxToolBarToolBase*)NULL : FindById(id);
        if(tool)
            help = tool->GetLongHelp();
        frame->DoGiveHelp( help, id != wxID_ANY );
    }

    (void)GetEventHandler()->ProcessEvent(event);
}

// ----------------------------------------------------------------------------
// UI updates
// ----------------------------------------------------------------------------

// Do the toolbar button updates (check for EVT_UPDATE_UI handlers)
void wxToolBarBase::UpdateWindowUI(long flags)
{
    wxWindowBase::UpdateWindowUI(flags);

    // There is no sense in updating the toolbar UI
    // if the parent window is about to get destroyed
    wxWindow *tlw = wxGetTopLevelParent( this );
    if (tlw && wxPendingDelete.Member( tlw ))
        return;

    wxEvtHandler* evtHandler = GetEventHandler() ;

    for ( wxToolBarToolsList::compatibility_iterator node = m_tools.GetFirst();
          node;
          node = node->GetNext() )
    {
        int id = node->GetData()->GetId();

        wxUpdateUIEvent event(id);
        event.SetEventObject(this);

        if ( evtHandler->ProcessEvent(event) )
        {
            if ( event.GetSetEnabled() )
                EnableTool(id, event.GetEnabled());
            if ( event.GetSetChecked() )
                ToggleTool(id, event.GetChecked());
#if 0
            if ( event.GetSetText() )
                // Set tooltip?
#endif // 0
        }
    }
}

#if wxUSE_IMAGE

/*
 * Make a greyed-out image suitable for disabled buttons.
 * This code is adapted from wxNewBitmapButton in FL.
 */

bool wxCreateGreyedImage(const wxImage& src, wxImage& dst)
{
    dst = src.Copy();

    unsigned char rBg, gBg, bBg;
    if ( src.HasMask() )
    {
        src.GetOrFindMaskColour(&rBg, &gBg, &bBg);
        dst.SetMaskColour(rBg, gBg, bBg);
    }
    else // assuming the pixels along the edges are of the background color
    {
        rBg = src.GetRed(0, 0);
        gBg = src.GetGreen(0, 0);
        bBg = src.GetBlue(0, 0);
    }

    const wxColour colBg(rBg, gBg, bBg);

    const wxColour colDark = wxSystemSettings::GetColour(wxSYS_COLOUR_3DSHADOW);
    const wxColour colLight = wxSystemSettings::GetColour(wxSYS_COLOUR_3DHIGHLIGHT);

    // Second attempt, just making things monochrome
    const int width = src.GetWidth();
    const int height = src.GetHeight();

    for ( int x = 0; x < width; x++ )
    {
        for ( int y = 0; y < height; y++ )
        {
            const int r = src.GetRed(x, y);
            const int g = src.GetGreen(x, y);
            const int b = src.GetBlue(x, y);

            if ( r == rBg && g == gBg && b == bBg )
            {
                // Leave the background colour as-is
                continue;
            }

            // Change light things to the background colour
            wxColour col;
            if ( r >= (colLight.Red() - 50) &&
                    g >= (colLight.Green() - 50) &&
                        b >= (colLight.Blue() - 50) )
            {
                col = colBg;
            }
            else // Change dark things to really dark
            {
                col = colDark;
            }

            dst.SetRGB(x, y, col.Red(), col.Green(), col.Blue());
        }
    }

    return true;
}

#endif // wxUSE_IMAGE

#endif // wxUSE_TOOLBAR
