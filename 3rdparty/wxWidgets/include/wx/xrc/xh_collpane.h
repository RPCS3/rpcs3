/////////////////////////////////////////////////////////////////////////////
// Name:        wx/xrc/xh_collpane.h
// Purpose:     XML resource handler for wxCollapsiblePane
// Author:      Francesco Montorsi
// Created:     2006-10-27
// RCS-ID:      $Id: xh_collpane.h 52422 2008-03-10 13:55:07Z VZ $
// Copyright:   (c) 2006 Francesco Montorsi
// Licence:     wxWindows licence
/////////////////////////////////////////////////////////////////////////////

#ifndef _WX_XH_COLLPANE_H_
#define _WX_XH_COLLPANE_H_

#include "wx/xrc/xmlres.h"

#if wxUSE_XRC && wxUSE_COLLPANE

class WXDLLIMPEXP_FWD_CORE wxCollapsiblePane;

class WXDLLIMPEXP_XRC wxCollapsiblePaneXmlHandler : public wxXmlResourceHandler
{
public:
    wxCollapsiblePaneXmlHandler();
    virtual wxObject *DoCreateResource();
    virtual bool CanHandle(wxXmlNode *node);

private:
    bool m_isInside;
    wxCollapsiblePane *m_collpane;

    DECLARE_DYNAMIC_CLASS(wxCollapsiblePaneXmlHandler)
};

#endif // wxUSE_XRC && wxUSE_COLLPANE

#endif // _WX_XH_COLLPANE_H_
