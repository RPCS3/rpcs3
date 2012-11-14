/////////////////////////////////////////////////////////////////////////////
// Name:        wx/mdi.h
// Purpose:     wxMDI base header
// Author:      Julian Smart
// Modified by:
// Created:
// Copyright:   (c) Julian Smart
// RCS-ID:      $Id: mdi.h 37066 2006-01-23 03:27:34Z MR $
// Licence:     wxWindows licence
/////////////////////////////////////////////////////////////////////////////

#ifndef _WX_MDI_H_BASE_
#define _WX_MDI_H_BASE_

#include "wx/defs.h"

#if wxUSE_MDI

#if defined(__WXUNIVERSAL__)
    #include "wx/generic/mdig.h"
#elif defined(__WXMSW__)
    #include "wx/msw/mdi.h"
#elif defined(__WXMOTIF__)
    #include "wx/motif/mdi.h"
#elif defined(__WXGTK20__)
    #include "wx/gtk/mdi.h"
#elif defined(__WXGTK__)
    #include "wx/gtk1/mdi.h"
#elif defined(__WXMAC__)
    #include "wx/mac/mdi.h"
#elif defined(__WXCOCOA__)
    #include "wx/cocoa/mdi.h"
#elif defined(__WXPM__)
    #include "wx/generic/mdig.h"
#endif

#endif // wxUSE_MDI

#endif
    // _WX_MDI_H_BASE_
