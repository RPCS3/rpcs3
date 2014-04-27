/////////////////////////////////////////////////////////////////////////////
// Name:        wx/msw/dcscreen.h
// Purpose:     wxScreenDC class
// Author:      Julian Smart
// Modified by:
// Created:     01/02/97
// RCS-ID:      $Id: dcscreen.h 36565 2005-12-25 12:52:53Z VZ $
// Copyright:   (c) Julian Smart
// Licence:     wxWindows licence
/////////////////////////////////////////////////////////////////////////////

#ifndef _WX_MSW_DCSCREEN_H_
#define _WX_MSW_DCSCREEN_H_

#include "wx/dc.h"

class WXDLLEXPORT wxScreenDC : public wxDC
{
public:
    // Create a DC representing the whole screen
    wxScreenDC();

    // Compatibility with X's requirements for drawing on top of all windows:
    // they don't do anything under MSW
    static bool StartDrawingOnTop(wxWindow* WXUNUSED(window)) { return true; }
    static bool StartDrawingOnTop(wxRect* WXUNUSED(rect) = NULL) { return true; }
    static bool EndDrawingOnTop() { return true; }

protected:
    virtual void DoGetSize(int *w, int *h) const
    {
        GetDeviceSize(w, h);
    }

private:
    DECLARE_DYNAMIC_CLASS_NO_COPY(wxScreenDC)
};

#endif // _WX_MSW_DCSCREEN_H_

