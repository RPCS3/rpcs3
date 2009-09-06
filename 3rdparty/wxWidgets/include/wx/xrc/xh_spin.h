/////////////////////////////////////////////////////////////////////////////
// Name:        wx/xrc/xh_spin.h
// Purpose:     XML resource handler for wxSpinButton and wxSpinCtrl
// Author:      Bob Mitchell
// Created:     2000/03/21
// RCS-ID:      $Id: xh_spin.h 41590 2006-10-03 14:53:40Z VZ $
// Copyright:   (c) 2000 Bob Mitchell and Verant Interactive
// Licence:     wxWindows licence
/////////////////////////////////////////////////////////////////////////////

#ifndef _WX_XH_SPIN_H_
#define _WX_XH_SPIN_H_

#include "wx/xrc/xmlres.h"

#if wxUSE_XRC 

#if wxUSE_SPINBTN

class WXDLLIMPEXP_XRC wxSpinButtonXmlHandler : public wxXmlResourceHandler
{
    DECLARE_DYNAMIC_CLASS(wxSpinButtonXmlHandler)
    enum
    {
        wxSP_DEFAULT_VALUE = 0,
        wxSP_DEFAULT_MIN = 0,
        wxSP_DEFAULT_MAX = 100
    };

public:
    wxSpinButtonXmlHandler();
    virtual wxObject *DoCreateResource();
    virtual bool CanHandle(wxXmlNode *node);
};

#endif // wxUSE_SPINBTN


#if wxUSE_SPINCTRL

class WXDLLIMPEXP_XRC wxSpinCtrlXmlHandler : public wxXmlResourceHandler
{
    DECLARE_DYNAMIC_CLASS(wxSpinCtrlXmlHandler)
    enum
    {
        wxSP_DEFAULT_VALUE = 0,
        wxSP_DEFAULT_MIN = 0,
        wxSP_DEFAULT_MAX = 100
    };

public:
    wxSpinCtrlXmlHandler();
    virtual wxObject *DoCreateResource();
    virtual bool CanHandle(wxXmlNode *node);
};

#endif // wxUSE_SPINCTRL

#endif // wxUSE_XRC

#endif // _WX_XH_SPIN_H_
