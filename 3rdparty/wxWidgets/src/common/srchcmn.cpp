/////////////////////////////////////////////////////////////////////////////
// Name:        src/common/srchcmn.cpp
// Purpose:     common (to all ports) bits of wxSearchCtrl
// Author:      Robin Dunn
// Modified by:
// Created:     19-Dec-2006
// RCS-ID:      $Id: srchcmn.cpp 43939 2006-12-11 20:32:16Z KO $
// Copyright:   (c) wxWidgets team
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

#if wxUSE_SEARCHCTRL

#include "wx/srchctrl.h"

#ifndef WX_PRECOMP
#endif

// ----------------------------------------------------------------------------

const wxChar wxSearchCtrlNameStr[] = wxT("searchCtrl");

DEFINE_EVENT_TYPE(wxEVT_COMMAND_SEARCHCTRL_CANCEL_BTN)
DEFINE_EVENT_TYPE(wxEVT_COMMAND_SEARCHCTRL_SEARCH_BTN)


#endif // wxUSE_SEARCHCTRL
