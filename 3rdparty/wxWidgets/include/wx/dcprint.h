/////////////////////////////////////////////////////////////////////////////
// Name:        wx/dcprint.h
// Purpose:     wxPrinterDC base header
// Author:      Julian Smart
// Modified by:
// Created:
// Copyright:   (c) Julian Smart
// RCS-ID:      $Id: dcprint.h 41240 2006-09-15 16:45:48Z PC $
// Licence:     wxWindows Licence
/////////////////////////////////////////////////////////////////////////////

#ifndef _WX_DCPRINT_H_BASE_
#define _WX_DCPRINT_H_BASE_

#include "wx/defs.h"

#if wxUSE_PRINTING_ARCHITECTURE

#if defined(__WXPALMOS__)
#include "wx/palmos/dcprint.h"
#elif defined(__WXMSW__)
#include "wx/msw/dcprint.h"
#endif
#if defined(__WXPM__)
#include "wx/os2/dcprint.h"
#endif
#if defined(__WXMAC__)
#include "wx/mac/dcprint.h"
#endif

#endif // wxUSE_PRINTING_ARCHITECTURE
#endif
    // _WX_DCPRINT_H_BASE_
