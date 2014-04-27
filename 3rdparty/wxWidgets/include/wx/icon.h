/////////////////////////////////////////////////////////////////////////////
// Name:        wx/icon.h
// Purpose:     wxIcon base header
// Author:      Julian Smart
// Modified by:
// Created:
// Copyright:   (c) Julian Smart
// RCS-ID:      $Id: icon.h 41538 2006-09-30 20:45:15Z RR $
// Licence:     wxWindows licence
/////////////////////////////////////////////////////////////////////////////

#ifndef _WX_ICON_H_BASE_
#define _WX_ICON_H_BASE_

#include "wx/iconloc.h"

#if defined(__WXPALMOS__)
  #include "wx/generic/icon.h"
#elif defined(__WXMSW__)
  #include "wx/msw/icon.h"
#elif defined(__WXMOTIF__)
  #include "wx/motif/icon.h"
#elif defined(__WXGTK20__)
  #include "wx/generic/icon.h"
#elif defined(__WXGTK__)
  #include "wx/generic/icon.h"
#elif defined(__WXX11__)
  #include "wx/generic/icon.h"
#elif defined(__WXMGL__)
  #define wxICON_DEFAULT_BITMAP_TYPE wxBITMAP_TYPE_ICO_RESOURCE
  #include "wx/generic/icon.h"
#elif defined(__WXDFB__)
  #include "wx/generic/icon.h"
#elif defined(__WXMAC__)
  #include "wx/mac/icon.h"
#elif defined(__WXCOCOA__)
  #include "wx/cocoa/icon.h"
#elif defined(__WXPM__)
  #include "wx/os2/icon.h"
#endif

//-----------------------------------------------------------------------------
// wxVariant support
//-----------------------------------------------------------------------------

#if wxUSE_VARIANT
#include "wx/variant.h"
DECLARE_VARIANT_OBJECT_EXPORTED(wxIcon,WXDLLEXPORT)
#endif


#endif
    // _WX_ICON_H_BASE_
