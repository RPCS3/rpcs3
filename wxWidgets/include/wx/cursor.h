/////////////////////////////////////////////////////////////////////////////
// Name:        wx/cursor.h
// Purpose:     wxCursor base header
// Author:      Julian Smart
// Modified by:
// Created:
// Copyright:   (c) Julian Smart
// RCS-ID:      $Id: cursor.h 40865 2006-08-27 09:42:42Z VS $
// Licence:     wxWindows Licence
/////////////////////////////////////////////////////////////////////////////

#ifndef _WX_CURSOR_H_BASE_
#define _WX_CURSOR_H_BASE_

#include "wx/defs.h"

#if defined(__WXPALMOS__)
    #include "wx/palmos/cursor.h"
#elif defined(__WXMSW__)
    #include "wx/msw/cursor.h"
#elif defined(__WXMOTIF__)
    #include "wx/motif/cursor.h"
#elif defined(__WXGTK20__)
    #include "wx/gtk/cursor.h"
#elif defined(__WXGTK__)
    #include "wx/gtk1/cursor.h"
#elif defined(__WXX11__)
    #include "wx/x11/cursor.h"
#elif defined(__WXMGL__)
    #include "wx/mgl/cursor.h"
#elif defined(__WXDFB__)
    #include "wx/dfb/cursor.h"
#elif defined(__WXMAC__)
    #include "wx/mac/cursor.h"
#elif defined(__WXCOCOA__)
    #include "wx/cocoa/cursor.h"
#elif defined(__WXPM__)
    #include "wx/os2/cursor.h"
#endif

#include "wx/utils.h"

/* This is a small class which can be used by all ports
   to temporarily suspend the busy cursor. Useful in modal
   dialogs.

   Actually that is not (any longer) quite true..  currently it is
   only used in wxGTK Dialog::ShowModal() and now uses static
   wxBusyCursor methods that are only implemented for wxGTK so far.
   The BusyCursor handling code should probably be implemented in
   common code somewhere instead of the separate implementations we
   currently have.  Also the name BusyCursorSuspender is a little
   misleading since it doesn't actually suspend the BusyCursor, just
   masks one that is already showing.
   If another call to wxBeginBusyCursor is made while this is active
   the Busy Cursor will again be shown.  But at least now it doesn't
   interfere with the state of wxIsBusy() -- RL

*/
class wxBusyCursorSuspender
{
public:
    wxBusyCursorSuspender()
    {
        if( wxIsBusy() )
        {
            wxSetCursor( wxBusyCursor::GetStoredCursor() );
        }
    }
    ~wxBusyCursorSuspender()
    {
        if( wxIsBusy() )
        {
            wxSetCursor( wxBusyCursor::GetBusyCursor() );
        }
    }
};
#endif
    // _WX_CURSOR_H_BASE_
