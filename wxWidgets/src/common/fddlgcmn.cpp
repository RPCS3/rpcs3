/////////////////////////////////////////////////////////////////////////////
// Name:        src/common/fdrepdlg.cpp
// Purpose:     common parts of wxFindReplaceDialog implementations
// Author:      Vadim Zeitlin
// Modified by:
// Created:     01.08.01
// RCS-ID:
// Copyright:   (c) 2001 Vadim Zeitlin
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

#if wxUSE_FINDREPLDLG

#ifndef WX_PRECOMP
#endif

#include "wx/fdrepdlg.h"

// ----------------------------------------------------------------------------
// wxWin macros
// ----------------------------------------------------------------------------

IMPLEMENT_DYNAMIC_CLASS(wxFindDialogEvent, wxCommandEvent)

DEFINE_EVENT_TYPE(wxEVT_COMMAND_FIND)
DEFINE_EVENT_TYPE(wxEVT_COMMAND_FIND_NEXT)
DEFINE_EVENT_TYPE(wxEVT_COMMAND_FIND_REPLACE)
DEFINE_EVENT_TYPE(wxEVT_COMMAND_FIND_REPLACE_ALL)
DEFINE_EVENT_TYPE(wxEVT_COMMAND_FIND_CLOSE)

// ============================================================================
// implementations
// ============================================================================

// ----------------------------------------------------------------------------
// wxFindReplaceData
// ----------------------------------------------------------------------------

void wxFindReplaceData::Init()
{
    m_Flags = 0;
}

// ----------------------------------------------------------------------------
// wxFindReplaceDialogBase
// ----------------------------------------------------------------------------

wxFindReplaceDialogBase::~wxFindReplaceDialogBase()
{
}

void wxFindReplaceDialogBase::Send(wxFindDialogEvent& event)
{
    // we copy the data to dialog->GetData() as well

    m_FindReplaceData->m_Flags = event.GetFlags();
    m_FindReplaceData->m_FindWhat = event.GetFindString();
    if ( HasFlag(wxFR_REPLACEDIALOG) &&
         (event.GetEventType() == wxEVT_COMMAND_FIND_REPLACE ||
          event.GetEventType() == wxEVT_COMMAND_FIND_REPLACE_ALL) )
    {
        m_FindReplaceData->m_ReplaceWith = event.GetReplaceString();
    }

    // translate wxEVT_COMMAND_FIND_NEXT to wxEVT_COMMAND_FIND if needed
    if ( event.GetEventType() == wxEVT_COMMAND_FIND_NEXT )
    {
        if ( m_FindReplaceData->m_FindWhat != m_lastSearch )
        {
            event.SetEventType(wxEVT_COMMAND_FIND);

            m_lastSearch = m_FindReplaceData->m_FindWhat;
        }
    }

    if ( !GetEventHandler()->ProcessEvent(event) )
    {
        // the event is not propagated upwards to the parent automatically
        // because the dialog is a top level window, so do it manually as
        // in 9 cases of 10 the message must be processed by the dialog
        // owner and not the dialog itself
        (void)GetParent()->GetEventHandler()->ProcessEvent(event);
    }
}

#endif // wxUSE_FINDREPLDLG

