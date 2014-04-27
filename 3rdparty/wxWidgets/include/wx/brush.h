/////////////////////////////////////////////////////////////////////////////
// Name:        wx/brush.h
// Purpose:     Includes platform-specific wxBrush file
// Author:      Julian Smart
// Modified by:
// Created:
// RCS-ID:      $Id: brush.h 40865 2006-08-27 09:42:42Z VS $
// Copyright:   Julian Smart
// Licence:     wxWindows Licence
/////////////////////////////////////////////////////////////////////////////

#ifndef _WX_BRUSH_H_BASE_
#define _WX_BRUSH_H_BASE_

#include "wx/defs.h"
#include "wx/object.h"
#include "wx/gdiobj.h"

// wxBrushBase
class WXDLLEXPORT wxBrushBase: public wxGDIObject
{
public:
    virtual ~wxBrushBase() { }

    virtual int GetStyle() const = 0;

    virtual bool IsHatch() const
        { return (GetStyle()>=wxFIRST_HATCH) && (GetStyle()<=wxLAST_HATCH); }
};

#if defined(__WXPALMOS__)
    #include "wx/palmos/brush.h"
#elif defined(__WXMSW__)
    #include "wx/msw/brush.h"
#elif defined(__WXMOTIF__) || defined(__WXX11__)
    #include "wx/x11/brush.h"
#elif defined(__WXGTK20__)
    #include "wx/gtk/brush.h"
#elif defined(__WXGTK__)
    #include "wx/gtk1/brush.h"
#elif defined(__WXMGL__)
    #include "wx/mgl/brush.h"
#elif defined(__WXDFB__)
    #include "wx/dfb/brush.h"
#elif defined(__WXMAC__)
    #include "wx/mac/brush.h"
#elif defined(__WXCOCOA__)
    #include "wx/cocoa/brush.h"
#elif defined(__WXPM__)
    #include "wx/os2/brush.h"
#endif

#endif
    // _WX_BRUSH_H_BASE_
