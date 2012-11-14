/////////////////////////////////////////////////////////////////////////////
// Name:        wx/splitter.h
// Purpose:     Base header for wxSplitterWindow
// Author:      Julian Smart
// Modified by:
// Created:
// RCS-ID:      $Id: splitter.h 38717 2006-04-14 17:01:16Z ABX $
// Copyright:   (c) Julian Smart
// Licence:     wxWindows Licence
/////////////////////////////////////////////////////////////////////////////

#ifndef _WX_SPLITTER_H_BASE_
#define _WX_SPLITTER_H_BASE_

#include "wx/event.h"

// ----------------------------------------------------------------------------
// wxSplitterWindow flags
// ----------------------------------------------------------------------------

#define wxSP_NOBORDER         0x0000
#define wxSP_NOSASH           0x0010
#define wxSP_PERMIT_UNSPLIT   0x0040
#define wxSP_LIVE_UPDATE      0x0080
#define wxSP_3DSASH           0x0100
#define wxSP_3DBORDER         0x0200
#define wxSP_NO_XP_THEME      0x0400
#define wxSP_BORDER           wxSP_3DBORDER
#define wxSP_3D               (wxSP_3DBORDER | wxSP_3DSASH)

#if WXWIN_COMPATIBILITY_2_6
    // obsolete styles, don't do anything
    #define wxSP_SASH_AQUA        0
    #define wxSP_FULLSASH         0
#endif // WXWIN_COMPATIBILITY_2_6

BEGIN_DECLARE_EVENT_TYPES()
    DECLARE_EVENT_TYPE(wxEVT_COMMAND_SPLITTER_SASH_POS_CHANGED, 850)
    DECLARE_EVENT_TYPE(wxEVT_COMMAND_SPLITTER_SASH_POS_CHANGING, 851)
    DECLARE_EVENT_TYPE(wxEVT_COMMAND_SPLITTER_DOUBLECLICKED, 852)
    DECLARE_EVENT_TYPE(wxEVT_COMMAND_SPLITTER_UNSPLIT, 853)
END_DECLARE_EVENT_TYPES()

#include "wx/generic/splitter.h"

#endif
    // _WX_SPLITTER_H_BASE_
