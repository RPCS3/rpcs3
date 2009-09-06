///////////////////////////////////////////////////////////////////////////////
// Name:        src/common/nbkbase.cpp
// Purpose:     common wxNotebook methods
// Author:      Vadim Zeitlin
// Modified by:
// Created:     02.07.01
// RCS-ID:      $Id: nbkbase.cpp 41764 2006-10-08 23:41:52Z VZ $
// Copyright:   (c) 2001 Vadim Zeitlin
// Licence:     wxWindows licence
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

#if wxUSE_NOTEBOOK

#ifndef WX_PRECOMP
#endif //WX_PRECOMP

#include "wx/notebook.h"

// ============================================================================
// implementation
// ============================================================================

// ----------------------------------------------------------------------------
// geometry
// ----------------------------------------------------------------------------

wxSize wxNotebookBase::CalcSizeFromPage(const wxSize& sizePage) const
{
    // this is, of course, totally bogus -- but we must do something by
    // default because not all ports implement this
    wxSize sizeTotal = sizePage;

    if ( HasFlag(wxBK_LEFT) || HasFlag(wxBK_RIGHT) )
    {
        sizeTotal.x += 90;
        sizeTotal.y += 10;
    }
    else // tabs on top/bottom side
    {
        sizeTotal.x += 10;
        sizeTotal.y += 40;
    }

    return sizeTotal;
}

// ----------------------------------------------------------------------------
// events
// ----------------------------------------------------------------------------

bool wxNotebookBase::SendPageChangingEvent(int nPage)
{
    wxNotebookEvent event(wxEVT_COMMAND_NOTEBOOK_PAGE_CHANGING, GetId());
    event.SetSelection(nPage);
    event.SetOldSelection(GetSelection());
    event.SetEventObject(this);
    return !GetEventHandler()->ProcessEvent(event) || event.IsAllowed();
}

void wxNotebookBase::SendPageChangedEvent(int nPageOld, int nPageNew)
{
    wxNotebookEvent event(wxEVT_COMMAND_NOTEBOOK_PAGE_CHANGED, GetId());
    event.SetSelection(nPageNew == -1 ? GetSelection() : nPageNew);
    event.SetOldSelection(nPageOld);
    event.SetEventObject(this);
    GetEventHandler()->ProcessEvent(event);
}

#endif // wxUSE_NOTEBOOK
