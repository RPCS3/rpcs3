/////////////////////////////////////////////////////////////////////////////
// Name:        src/xrc/xh_gdctl.cpp
// Purpose:     XRC resource for wxGenericDirCtrl
// Author:      Markus Greither
// Created:     2002/01/20
// RCS-ID:      $Id: xh_gdctl.cpp 44726 2007-03-10 17:23:26Z VZ $
// Copyright:   (c) 2002 Markus Greither
// Licence:     wxWindows licence
/////////////////////////////////////////////////////////////////////////////

// For compilers that support precompilation, includes "wx.h".
#include "wx/wxprec.h"

#ifdef __BORLANDC__
    #pragma hdrstop
#endif

#if wxUSE_XRC && wxUSE_DIRDLG

#include "wx/xrc/xh_gdctl.h"

#ifndef WX_PRECOMP
    #include "wx/textctrl.h"
#endif

#include "wx/dirctrl.h"

IMPLEMENT_DYNAMIC_CLASS(wxGenericDirCtrlXmlHandler, wxXmlResourceHandler)

wxGenericDirCtrlXmlHandler::wxGenericDirCtrlXmlHandler()
: wxXmlResourceHandler()
{
    XRC_ADD_STYLE(wxDIRCTRL_DIR_ONLY);
    XRC_ADD_STYLE(wxDIRCTRL_3D_INTERNAL);
    XRC_ADD_STYLE(wxDIRCTRL_SELECT_FIRST);
    XRC_ADD_STYLE(wxDIRCTRL_SHOW_FILTERS);
    XRC_ADD_STYLE(wxDIRCTRL_EDIT_LABELS);
    AddWindowStyles();
}

wxObject *wxGenericDirCtrlXmlHandler::DoCreateResource()
{
    XRC_MAKE_INSTANCE(ctrl, wxGenericDirCtrl)

    ctrl->Create(m_parentAsWindow,
                 GetID(),
                 GetText(wxT("defaultfolder")),
                 GetPosition(), GetSize(),
                 GetStyle(),
                 GetText(wxT("filter")),
                 (int)GetLong(wxT("defaultfilter")),
                 GetName());

    SetupWindow(ctrl);

    return ctrl;
}

bool wxGenericDirCtrlXmlHandler::CanHandle(wxXmlNode *node)
{
    return IsOfClass(node, wxT("wxGenericDirCtrl"));
}

#endif // wxUSE_XRC && wxUSE_DIRDLG
