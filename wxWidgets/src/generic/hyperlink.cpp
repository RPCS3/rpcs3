/////////////////////////////////////////////////////////////////////////////
// Name:        src/generic/hyperlink.cpp
// Purpose:     Hyperlink control
// Author:      David Norris <danorris@gmail.com>, Otto Wyss
// Modified by: Ryan Norton, Francesco Montorsi
// Created:     04/02/2005
// RCS-ID:      $Id: hyperlink.cpp 62092 2009-09-24 16:46:50Z JS $
// Copyright:   (c) 2005 David Norris
// Licence:     wxWindows licence
/////////////////////////////////////////////////////////////////////////////

// ============================================================================
// declarations
// ============================================================================

//---------------------------------------------------------------------------
// Pre-compiled header stuff
//---------------------------------------------------------------------------

// For compilers that support precompilation, includes "wx.h".
#include "wx/wxprec.h"

#ifdef __BORLANDC__
    #pragma hdrstop
#endif

#if wxUSE_HYPERLINKCTRL

//---------------------------------------------------------------------------
// Includes
//---------------------------------------------------------------------------

#include "wx/hyperlink.h"

#ifndef WX_PRECOMP
    #include "wx/utils.h" // for wxLaunchDefaultBrowser
    #include "wx/dcclient.h"
    #include "wx/menu.h"
    #include "wx/log.h"
    #include "wx/dataobj.h"
#endif

#include "wx/clipbrd.h"

// ============================================================================
// implementation
// ============================================================================

IMPLEMENT_DYNAMIC_CLASS(wxHyperlinkCtrl, wxControl)
IMPLEMENT_DYNAMIC_CLASS(wxHyperlinkEvent, wxCommandEvent)
DEFINE_EVENT_TYPE(wxEVT_COMMAND_HYPERLINK)

// reserved for internal use only
#define wxHYPERLINKCTRL_POPUP_COPY_ID           16384

const wxChar wxHyperlinkCtrlNameStr[] = wxT("hyperlink");

// ----------------------------------------------------------------------------
// wxHyperlinkCtrl
// ----------------------------------------------------------------------------

BEGIN_EVENT_TABLE(wxHyperlinkCtrl, wxControl)
    EVT_PAINT(wxHyperlinkCtrl::OnPaint)
    EVT_LEFT_DOWN(wxHyperlinkCtrl::OnLeftDown)
    EVT_LEFT_UP(wxHyperlinkCtrl::OnLeftUp)
    EVT_RIGHT_UP(wxHyperlinkCtrl::OnRightUp)
    EVT_MOTION(wxHyperlinkCtrl::OnMotion)
    EVT_LEAVE_WINDOW(wxHyperlinkCtrl::OnLeaveWindow)
    EVT_SIZE(wxHyperlinkCtrl::OnSize)

    // for the context menu
    EVT_MENU(wxHYPERLINKCTRL_POPUP_COPY_ID, wxHyperlinkCtrl::OnPopUpCopy)
END_EVENT_TABLE()

bool wxHyperlinkCtrl::Create(wxWindow *parent, wxWindowID id,
    const wxString& label, const wxString& url, const wxPoint& pos,
    const wxSize& size, long style, const wxString& name)
{
    wxASSERT_MSG(!url.empty() || !label.empty(),
                 wxT("Both URL and label are empty ?"));

#ifdef __WXDEBUG__
    int alignment = (int)((style & wxHL_ALIGN_LEFT) != 0) +
                    (int)((style & wxHL_ALIGN_CENTRE) != 0) +
                    (int)((style & wxHL_ALIGN_RIGHT) != 0);
    wxASSERT_MSG(alignment == 1,
        wxT("Specify exactly one align flag!"));
#endif

    if (!wxControl::Create(parent, id, pos, size, style, wxDefaultValidator, name))
        return false;

    // set to non empty strings both the url and the label
    if(url.empty())
        SetURL(label);
    else
        SetURL(url);

    if(label.empty())
        SetLabel(url);
    else
        SetLabel(label);

    m_rollover = false;
    m_clicking = false;
    m_visited = false;

    // colours
    m_normalColour = *wxBLUE;
    m_hoverColour = *wxRED;
    m_visitedColour = wxColour(wxT("#551a8b"));
    SetForegroundColour(m_normalColour);

    // by default the font of an hyperlink control is underlined
    wxFont f = GetFont();
    f.SetUnderlined(true);
    SetFont(f);

    SetInitialSize(size);

    return true;
}

wxSize wxHyperlinkCtrl::DoGetBestSize() const
{
    int w, h;

    wxClientDC dc((wxWindow *)this);
    dc.SetFont(GetFont());
    dc.GetTextExtent(GetLabel(), &w, &h);

    wxSize best(w, h);
    CacheBestSize(best);
    return best;
}


void wxHyperlinkCtrl::SetNormalColour(const wxColour &colour)
{
    m_normalColour = colour;
    if (!m_visited)
    {
        SetForegroundColour(m_normalColour);
        Refresh();
    }
}

void wxHyperlinkCtrl::SetVisitedColour(const wxColour &colour)
{
    m_visitedColour = colour;
    if (m_visited)
    {
        SetForegroundColour(m_visitedColour);
        Refresh();
    }
}

void wxHyperlinkCtrl::DoContextMenu(const wxPoint &pos)
{
    wxMenu *menuPopUp = new wxMenu(wxEmptyString, wxMENU_TEAROFF);
    menuPopUp->Append(wxHYPERLINKCTRL_POPUP_COPY_ID, _("&Copy URL"));
    PopupMenu( menuPopUp, pos );
    delete menuPopUp;
}

wxRect wxHyperlinkCtrl::GetLabelRect() const
{
    // our best size is always the size of the label without borders
    wxSize c(GetClientSize()), b(GetBestSize());
    wxPoint offset;

    // the label is always centered vertically
    offset.y = (c.GetHeight()-b.GetHeight())/2;

    if (HasFlag(wxHL_ALIGN_CENTRE))
        offset.x = (c.GetWidth()-b.GetWidth())/2;
    else if (HasFlag(wxHL_ALIGN_RIGHT))
        offset.x = c.GetWidth()-b.GetWidth();
    else if (HasFlag(wxHL_ALIGN_LEFT))
        offset.x = 0;
    return wxRect(offset, b);
}



// ----------------------------------------------------------------------------
// wxHyperlinkCtrl - event handlers
// ----------------------------------------------------------------------------

void wxHyperlinkCtrl::OnPaint(wxPaintEvent& WXUNUSED(event))
{
    wxPaintDC dc(this);
    dc.SetFont(GetFont());
    dc.SetTextForeground(GetForegroundColour());
    dc.SetTextBackground(GetBackgroundColour());

    dc.DrawText(GetLabel(), GetLabelRect().GetTopLeft());
}

void wxHyperlinkCtrl::OnLeftDown(wxMouseEvent& event)
{
    // the left click must start from the hyperlink rect
    m_clicking = GetLabelRect().Contains(event.GetPosition());
}

void wxHyperlinkCtrl::OnLeftUp(wxMouseEvent& event)
{
    // the click must be started and ended in the hyperlink rect
    if (!m_clicking || !GetLabelRect().Contains(event.GetPosition()))
        return;

    SetForegroundColour(m_visitedColour);
    m_visited = true;
    m_clicking = false;

    // send the event
    wxHyperlinkEvent linkEvent(this, GetId(), m_url);
    if (!GetEventHandler()->ProcessEvent(linkEvent))     // was the event skipped ?
        if (!wxLaunchDefaultBrowser(m_url))
            wxLogWarning(wxT("Could not launch the default browser with url '%s' !"), m_url.c_str());
}

void wxHyperlinkCtrl::OnRightUp(wxMouseEvent& event)
{
    if( GetWindowStyle() & wxHL_CONTEXTMENU )
        if ( GetLabelRect().Contains(event.GetPosition()) )
            DoContextMenu(wxPoint(event.m_x, event.m_y));
}

void wxHyperlinkCtrl::OnMotion(wxMouseEvent& event)
{
    wxRect textrc = GetLabelRect();

    if (textrc.Contains(event.GetPosition()))
    {
        SetCursor(wxCursor(wxCURSOR_HAND));
        SetForegroundColour(m_hoverColour);
        m_rollover = true;
        Refresh();
    }
    else if (m_rollover)
    {
        SetCursor(*wxSTANDARD_CURSOR);
        SetForegroundColour(!m_visited ? m_normalColour : m_visitedColour);
        m_rollover = false;
        Refresh();
    }
}

void wxHyperlinkCtrl::OnLeaveWindow(wxMouseEvent& WXUNUSED(event) )
{
    // NB: when the label rect and the client size rect have the same
    //     height this function is indispensable to remove the "rollover"
    //     effect as the OnMotion() event handler could not be called
    //     in that case moving the mouse out of the label vertically...

    if (m_rollover)
    {
        SetCursor(*wxSTANDARD_CURSOR);
        SetForegroundColour(!m_visited ? m_normalColour : m_visitedColour);
        m_rollover = false;
        Refresh();
    }
}

void wxHyperlinkCtrl::OnPopUpCopy( wxCommandEvent& WXUNUSED(event) )
{
#if wxUSE_CLIPBOARD
    if (!wxTheClipboard->Open())
        return;

    wxTextDataObject *data = new wxTextDataObject( m_url );
    wxTheClipboard->SetData( data );
    wxTheClipboard->Close();
#endif // wxUSE_CLIPBOARD
}

void wxHyperlinkCtrl::OnSize(wxSizeEvent& WXUNUSED(event))
{
    // update the position of the label in the screen respecting
    // the selected align flag
    Refresh();
}

#endif // wxUSE_HYPERLINKCTRL
