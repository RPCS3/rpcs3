/////////////////////////////////////////////////////////////////////////////
// Name:        xh_tree.cpp
// Purpose:     XRC resource for wxTreeCtrl
// Author:      Brian Gavin
// Created:     2000/09/09
// RCS-ID:      $Id: xh_tree.cpp 35650 2005-09-23 12:56:45Z MR $
// Copyright:   (c) 2000 Brian Gavin
// Licence:     wxWindows licence
/////////////////////////////////////////////////////////////////////////////

// For compilers that support precompilation, includes "wx.h".
#include "wx/wxprec.h"

#ifdef __BORLANDC__
    #pragma hdrstop
#endif

#if wxUSE_XRC && wxUSE_TREECTRL

#include "wx/xrc/xh_tree.h"
#include "wx/treectrl.h"

IMPLEMENT_DYNAMIC_CLASS(wxTreeCtrlXmlHandler, wxXmlResourceHandler)

wxTreeCtrlXmlHandler::wxTreeCtrlXmlHandler()
: wxXmlResourceHandler()
{
    XRC_ADD_STYLE(wxTR_EDIT_LABELS);
    XRC_ADD_STYLE(wxTR_NO_BUTTONS);
    XRC_ADD_STYLE(wxTR_HAS_BUTTONS);
    XRC_ADD_STYLE(wxTR_TWIST_BUTTONS);
    XRC_ADD_STYLE(wxTR_NO_LINES);
    XRC_ADD_STYLE(wxTR_FULL_ROW_HIGHLIGHT);
    XRC_ADD_STYLE(wxTR_LINES_AT_ROOT);
    XRC_ADD_STYLE(wxTR_HIDE_ROOT);
    XRC_ADD_STYLE(wxTR_ROW_LINES);
    XRC_ADD_STYLE(wxTR_HAS_VARIABLE_ROW_HEIGHT);
    XRC_ADD_STYLE(wxTR_SINGLE);
    XRC_ADD_STYLE(wxTR_MULTIPLE);
    XRC_ADD_STYLE(wxTR_EXTENDED);
    XRC_ADD_STYLE(wxTR_DEFAULT_STYLE);
    AddWindowStyles();
}

wxObject *wxTreeCtrlXmlHandler::DoCreateResource()
{
    XRC_MAKE_INSTANCE(tree, wxTreeCtrl)

    tree->Create(m_parentAsWindow,
                GetID(),
                GetPosition(), GetSize(),
                GetStyle(_T("style"), wxTR_DEFAULT_STYLE),
                wxDefaultValidator,
                GetName());

    SetupWindow(tree);

    return tree;
}

bool wxTreeCtrlXmlHandler::CanHandle(wxXmlNode *node)
{
    return IsOfClass(node, wxT("wxTreeCtrl"));
}

#endif // wxUSE_XRC && wxUSE_TREECTRL
