/////////////////////////////////////////////////////////////////////////////
// Name:        wx/xrc/xh_menu.h
// Purpose:     XML resource handler for menus/menubars
// Author:      Vaclav Slavik
// Created:     2000/03/05
// RCS-ID:      $Id: xh_menu.h 41590 2006-10-03 14:53:40Z VZ $
// Copyright:   (c) 2000 Vaclav Slavik
// Licence:     wxWindows licence
/////////////////////////////////////////////////////////////////////////////

#ifndef _WX_XH_MENU_H_
#define _WX_XH_MENU_H_

#include "wx/xrc/xmlres.h"

#if wxUSE_XRC && wxUSE_MENUS

class WXDLLIMPEXP_XRC wxMenuXmlHandler : public wxXmlResourceHandler
{
    DECLARE_DYNAMIC_CLASS(wxMenuXmlHandler)

public:
    wxMenuXmlHandler();
    virtual wxObject *DoCreateResource();
    virtual bool CanHandle(wxXmlNode *node);

private:
    bool m_insideMenu;
};

class WXDLLIMPEXP_XRC wxMenuBarXmlHandler : public wxXmlResourceHandler
{
    DECLARE_DYNAMIC_CLASS(wxMenuBarXmlHandler)

public:
    wxMenuBarXmlHandler();
    virtual wxObject *DoCreateResource();
    virtual bool CanHandle(wxXmlNode *node);
};

#endif // wxUSE_XRC && wxUSE_MENUS

#endif // _WX_XH_MENU_H_
