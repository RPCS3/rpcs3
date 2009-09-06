/////////////////////////////////////////////////////////////////////////////
// Name:        wx/xrc/xh_tree.h
// Purpose:     XML resource handler for wxTreeCtrl
// Author:      Brian Gavin
// Created:     2000/09/09
// RCS-ID:      $Id: xh_tree.h 41590 2006-10-03 14:53:40Z VZ $
// Copyright:   (c) 2000 Brian Gavin
// Licence:     wxWindows licence
/////////////////////////////////////////////////////////////////////////////

#ifndef _WX_XH_TREE_H_
#define _WX_XH_TREE_H_

#include "wx/xrc/xmlres.h"

#if wxUSE_XRC && wxUSE_TREECTRL

class WXDLLIMPEXP_XRC wxTreeCtrlXmlHandler : public wxXmlResourceHandler
{
    DECLARE_DYNAMIC_CLASS(wxTreeCtrlXmlHandler)

public:
    wxTreeCtrlXmlHandler();
    virtual wxObject *DoCreateResource();
    virtual bool CanHandle(wxXmlNode *node);
};

#endif // wxUSE_XRC && wxUSE_TREECTRL

#endif // _WX_XH_TREE_H_
