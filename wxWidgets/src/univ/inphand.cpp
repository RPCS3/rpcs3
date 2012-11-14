///////////////////////////////////////////////////////////////////////////////
// Name:        univ/inphand.cpp
// Purpose:     (trivial) wxInputHandler implementation
// Author:      Vadim Zeitlin
// Modified by:
// Created:     18.08.00
// RCS-ID:      $Id: inphand.cpp 35650 2005-09-23 12:56:45Z MR $
// Copyright:   (c) 2000 SciTech Software, Inc. (www.scitechsoft.com)
// Licence:     wxWindows licence
///////////////////////////////////////////////////////////////////////////////

// ===========================================================================
// declarations
// ===========================================================================

// ---------------------------------------------------------------------------
// headers
// ---------------------------------------------------------------------------

// For compilers that support precompilation, includes "wx.h".
#include "wx/wxprec.h"

#ifdef __BORLANDC__
    #pragma hdrstop
#endif

#ifndef WX_PRECOMP
#endif // WX_PRECOMP

#include "wx/univ/inphand.h"

// ============================================================================
// implementation
// ============================================================================

// ----------------------------------------------------------------------------
// wxInputHandler
// ----------------------------------------------------------------------------

bool wxInputHandler::HandleMouseMove(wxInputConsumer * WXUNUSED(consumer),
                                     const wxMouseEvent& WXUNUSED(event))
{
    return false;
}

bool wxInputHandler::HandleFocus(wxInputConsumer *WXUNUSED(consumer),
                                 const wxFocusEvent& WXUNUSED(event))
{
    return false;
}

bool wxInputHandler::HandleActivation(wxInputConsumer *WXUNUSED(consumer),
                                      bool WXUNUSED(activated))
{
    return false;
}

wxInputHandler::~wxInputHandler()
{
}

