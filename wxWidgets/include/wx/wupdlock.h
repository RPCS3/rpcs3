///////////////////////////////////////////////////////////////////////////////
// Name:        wx/wupdlock.h
// Purpose:     wxWindowUpdateLocker prevents window redrawing
// Author:      Vadim Zeitlin
// Created:     2006-03-06
// RCS-ID:      $Id: wupdlock.h 37842 2006-03-07 01:50:21Z VZ $
// Copyright:   (c) 2006 Vadim Zeitlin <vadim@wxwindows.org>
// Licence:     wxWindows licence
///////////////////////////////////////////////////////////////////////////////

#ifndef _WX_WUPDLOCK_H_
#define _WX_WUPDLOCK_H_

#include "wx/window.h"

// ----------------------------------------------------------------------------
// wxWindowUpdateLocker prevents updates to the window during its lifetime
// ----------------------------------------------------------------------------

class wxWindowUpdateLocker
{
public:
    // create an object preventing updates of the given window (which must have
    // a lifetime at least as great as ours)
    wxWindowUpdateLocker(wxWindow *win) : m_win(win) { win->Freeze(); }

    // dtor thaws the window to permit updates again
    ~wxWindowUpdateLocker() { m_win->Thaw(); }

private:
    wxWindow *m_win;

    DECLARE_NO_COPY_CLASS(wxWindowUpdateLocker)
};

#endif // _WX_WUPDLOCK_H_

