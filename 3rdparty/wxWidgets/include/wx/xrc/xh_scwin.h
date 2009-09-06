/////////////////////////////////////////////////////////////////////////////
// Name:        wx/xrc/xh_scwin.h
// Purpose:     XML resource handler for wxScrolledWindow
// Author:      Vaclav Slavik
// Created:     2002/10/18
// RCS-ID:      $Id: xh_scwin.h 41590 2006-10-03 14:53:40Z VZ $
// Copyright:   (c) 2002 Vaclav Slavik
// Licence:     wxWindows licence
/////////////////////////////////////////////////////////////////////////////

#ifndef _WX_XH_SCWIN_H_
#define _WX_XH_SCWIN_H_

#include "wx/xrc/xmlres.h"

#if wxUSE_XRC

class WXDLLIMPEXP_XRC wxScrolledWindowXmlHandler : public wxXmlResourceHandler
{
    DECLARE_DYNAMIC_CLASS(wxScrolledWindowXmlHandler)

public:
    wxScrolledWindowXmlHandler();
    virtual wxObject *DoCreateResource();
    virtual bool CanHandle(wxXmlNode *node);
};

#endif // wxUSE_XRC

#endif // _WX_XH_SCWIN_H_
