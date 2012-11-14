/////////////////////////////////////////////////////////////////////////////
// Name:        src/univ/inpcons.cpp
// Purpose:     wxInputConsumer: mix-in class for input handling
// Author:      Vadim Zeitlin
// Modified by:
// Created:     14.08.00
// RCS-ID:      $Id: inpcons.cpp 42732 2006-10-30 17:05:42Z ABX $
// Copyright:   (c) 2000 SciTech Software, Inc. (www.scitechsoft.com)
// Licence:     wxWindows licence
/////////////////////////////////////////////////////////////////////////////

// ============================================================================
// declarations
// ============================================================================

// ----------------------------------------------------------------------------
// headers
// ----------------------------------------------------------------------------

#include "wx/wxprec.h"

#ifdef __BORLANDC__
    #pragma hdrstop
#endif

#ifndef WX_PRECOMP
    #include "wx/window.h"
#endif // WX_PRECOMP

#include "wx/univ/renderer.h"
#include "wx/univ/inphand.h"
#include "wx/univ/theme.h"

// ============================================================================
// implementation
// ============================================================================

// ----------------------------------------------------------------------------
// focus/activation handling
// ----------------------------------------------------------------------------

void wxInputConsumer::OnFocus(wxFocusEvent& event)
{
    if ( m_inputHandler && m_inputHandler->HandleFocus(this, event) )
        GetInputWindow()->Refresh();
    else
        event.Skip();
}

void wxInputConsumer::OnActivate(wxActivateEvent& event)
{
    if ( m_inputHandler && m_inputHandler->HandleActivation(this, event.GetActive()) )
        GetInputWindow()->Refresh();
    else
        event.Skip();
}

// ----------------------------------------------------------------------------
// input processing
// ----------------------------------------------------------------------------

wxInputHandler *
wxInputConsumer::DoGetStdInputHandler(wxInputHandler * WXUNUSED(handlerDef))
{
    return NULL;
}

void wxInputConsumer::CreateInputHandler(const wxString& inphandler)
{
    m_inputHandler = wxTheme::Get()->GetInputHandler(inphandler, this);
}

void wxInputConsumer::OnKeyDown(wxKeyEvent& event)
{
    if ( !m_inputHandler || !m_inputHandler->HandleKey(this, event, true) )
        event.Skip();
}

void wxInputConsumer::OnKeyUp(wxKeyEvent& event)
{
    if ( !m_inputHandler || !m_inputHandler->HandleKey(this, event, false) )
        event.Skip();
}

void wxInputConsumer::OnMouse(wxMouseEvent& event)
{
    if ( m_inputHandler )
    {
        if ( event.Moving() || event.Dragging() ||
                event.Entering() || event.Leaving() )
        {
            if ( m_inputHandler->HandleMouseMove(this, event) )
                return;
        }
        else // a click action
        {
            if ( m_inputHandler->HandleMouse(this, event) )
                return;
        }
    }

    event.Skip();
}

// ----------------------------------------------------------------------------
// the actions
// ----------------------------------------------------------------------------

bool wxInputConsumer::PerformAction(const wxControlAction& WXUNUSED(action),
                                    long WXUNUSED(numArg),
                                    const wxString& WXUNUSED(strArg))
{
    return false;
}
