/////////////////////////////////////////////////////////////////////////
// File:        src/common/taskbarcmn.cpp
// Purpose:     Common parts of wxTaskBarIcon class
// Author:      Julian Smart
// Modified by:
// Created:     04/04/2003
// RCS-ID:      $Id: taskbarcmn.cpp 44138 2007-01-07 19:44:14Z VZ $
// Copyright:   (c) Julian Smart, 2003
// Licence:     wxWindows licence
/////////////////////////////////////////////////////////////////////////

// For compilers that support precompilation, includes "wx.h".
#include "wx/wxprec.h"

#ifdef __BORLANDC__
    #pragma hdrstop
#endif

#ifdef wxHAS_TASK_BAR_ICON

#ifndef WX_PRECOMP
    #include "wx/app.h"
    #include "wx/menu.h"
#endif

// DLL options compatibility check:
WX_CHECK_BUILD_OPTIONS("wxAdvanced")

#include "wx/taskbar.h"

DEFINE_EVENT_TYPE( wxEVT_TASKBAR_MOVE )
DEFINE_EVENT_TYPE( wxEVT_TASKBAR_LEFT_DOWN )
DEFINE_EVENT_TYPE( wxEVT_TASKBAR_LEFT_UP )
DEFINE_EVENT_TYPE( wxEVT_TASKBAR_RIGHT_DOWN )
DEFINE_EVENT_TYPE( wxEVT_TASKBAR_RIGHT_UP )
DEFINE_EVENT_TYPE( wxEVT_TASKBAR_LEFT_DCLICK )
DEFINE_EVENT_TYPE( wxEVT_TASKBAR_RIGHT_DCLICK )


BEGIN_EVENT_TABLE(wxTaskBarIconBase, wxEvtHandler)
    EVT_TASKBAR_CLICK(wxTaskBarIconBase::OnRightButtonDown)
END_EVENT_TABLE()

void wxTaskBarIconBase::OnRightButtonDown(wxTaskBarIconEvent& WXUNUSED(event))
{
    wxMenu *menu = CreatePopupMenu();
    if (menu)
    {
        PopupMenu(menu);
        delete menu;
    }
}

#endif // defined(wxHAS_TASK_BAR_ICON)
