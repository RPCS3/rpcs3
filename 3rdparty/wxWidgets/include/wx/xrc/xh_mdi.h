/////////////////////////////////////////////////////////////////////////////
// Name:        wx/xrc/xh_mdi.h
// Purpose:     XML resource handler for wxMDI
// Author:      David M. Falkinder & Vaclav Slavik
// Created:     14/02/2005
// RCS-ID:      $Id: xh_mdi.h 49804 2007-11-10 01:09:42Z VZ $
// Copyright:   (c) 2005 Vaclav Slavik
// Licence:     wxWindows licence
/////////////////////////////////////////////////////////////////////////////

#ifndef _WX_XH_MDI_H_
#define _WX_XH_MDI_H_

#include "wx/xrc/xmlres.h"

#if wxUSE_XRC && wxUSE_MDI

class WXDLLIMPEXP_FWD_CORE wxWindow;

class WXDLLIMPEXP_XRC wxMdiXmlHandler : public wxXmlResourceHandler
{
    DECLARE_DYNAMIC_CLASS(wxMdiXmlHandler)

public:
    wxMdiXmlHandler();
    virtual wxObject *DoCreateResource();
    virtual bool CanHandle(wxXmlNode *node);

private:
    wxWindow *CreateFrame();
};

#endif // wxUSE_XRC && wxUSE_MDI

#endif // _WX_XH_MDI_H_
