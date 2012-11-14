/////////////////////////////////////////////////////////////////////////////
// Name:        spinctrl.h
// Purpose:     wxSpinCtrlBase class
// Author:      Vadim Zeitlin
// Modified by:
// Created:     22.07.99
// RCS-ID:      $Id: spinctrl.h 37066 2006-01-23 03:27:34Z MR $
// Copyright:   (c) Vadim Zeitlin
// Licence:     wxWindows licence
/////////////////////////////////////////////////////////////////////////////

#ifndef _WX_SPINCTRL_H_
#define _WX_SPINCTRL_H_

#include "wx/defs.h"

#if wxUSE_SPINCTRL

#include "wx/spinbutt.h"        // should make wxSpinEvent visible to the app

// ----------------------------------------------------------------------------
// a spin ctrl is a text control with a spin button which is usually used to
// prompt the user for a numeric input
// ----------------------------------------------------------------------------

/* there is no generic base class for this control because it's imlpemented
   very differently under MSW and other platforms

class WXDLLEXPORT wxSpinCtrlBase : public wxControl
{
public:
    wxSpinCtrlBase() { Init(); }

    // accessors
    virtual int GetValue() const = 0;
    virtual int GetMin() const { return m_min; }
    virtual int GetMax() const { return m_max; }

    // operations
    virtual void SetValue(const wxString& value) = 0;
    virtual void SetValue(int val) = 0;
    virtual void SetRange(int minVal, int maxVal) = 0;

    // as the wxTextCtrl method
    virtual void SetSelection(long from, long to) = 0;

protected:
    // initialize m_min/max with the default values
    void Init() { m_min = 0; m_max = 100; }

    int   m_min;
    int   m_max;
};
*/

// ----------------------------------------------------------------------------
// include the platform-dependent class implementation
// ----------------------------------------------------------------------------

#if defined(__WXUNIVERSAL__)
    #include "wx/generic/spinctlg.h"
#elif defined(__WXMSW__)
    #include "wx/msw/spinctrl.h"
#elif defined(__WXPM__)
    #include "wx/os2/spinctrl.h"
#elif defined(__WXGTK20__)
    #include "wx/gtk/spinctrl.h"
#elif defined(__WXGTK__)
    #include "wx/gtk1/spinctrl.h"
#elif defined(__WXMOTIF__)
    #include "wx/generic/spinctlg.h"
#elif defined(__WXMAC__)
    #include "wx/mac/spinctrl.h"
#elif defined(__WXCOCOA__)
    #include "wx/generic/spinctlg.h"
#endif // platform

#define EVT_SPINCTRL(id, fn) \
    wx__DECLARE_EVT1(wxEVT_COMMAND_SPINCTRL_UPDATED, id, wxSpinEventHandler(fn))

#endif // wxUSE_SPINCTRL

#endif // _WX_SPINCTRL_H_
