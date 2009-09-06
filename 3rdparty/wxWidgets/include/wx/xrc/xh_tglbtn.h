/////////////////////////////////////////////////////////////////////////////
// Name:        wx/xrc/xh_tglbtn.h
// Purpose:     XML resource handler for wxToggleButton
// Author:      Julian Smart
// Created:     2004-08-30
// RCS-ID:      $Id: xh_tglbtn.h 41590 2006-10-03 14:53:40Z VZ $
// Copyright:   (c) 2004 Julian Smart
// Licence:     wxWindows licence
/////////////////////////////////////////////////////////////////////////////

#ifndef _WX_XH_TGLBTN_H_
#define _WX_XH_TGLBTN_H_

#include "wx/xrc/xmlres.h"

#if wxUSE_XRC && wxUSE_TOGGLEBTN

class WXDLLIMPEXP_XRC wxToggleButtonXmlHandler : public wxXmlResourceHandler
{
    DECLARE_DYNAMIC_CLASS(wxToggleButtonXmlHandler)

public:
    wxToggleButtonXmlHandler();
    virtual wxObject *DoCreateResource();
    virtual bool CanHandle(wxXmlNode *node);
};

#endif // wxUSE_XRC && wxUSE_TOGGLEBTN

#endif // _WX_XH_TGLBTN_H_
