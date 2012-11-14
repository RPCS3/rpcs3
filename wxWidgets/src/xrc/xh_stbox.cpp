/////////////////////////////////////////////////////////////////////////////
// Name:        src/xrc/xh_stbox.cpp
// Purpose:     XRC resource for wxStaticBox
// Author:      Brian Gavin
// Created:     2000/09/09
// RCS-ID:      $Id: xh_stbox.cpp 39487 2006-05-31 18:27:51Z ABX $
// Copyright:   (c) 2000 Brian Gavin
// Licence:     wxWindows licence
/////////////////////////////////////////////////////////////////////////////

// For compilers that support precompilation, includes "wx.h".
#include "wx/wxprec.h"

#ifdef __BORLANDC__
    #pragma hdrstop
#endif

#if wxUSE_XRC && wxUSE_STATBOX

#include "wx/xrc/xh_stbox.h"

#ifndef WX_PRECOMP
    #include "wx/statbox.h"
#endif

IMPLEMENT_DYNAMIC_CLASS(wxStaticBoxXmlHandler, wxXmlResourceHandler)

wxStaticBoxXmlHandler::wxStaticBoxXmlHandler()
                      :wxXmlResourceHandler()
{
    AddWindowStyles();
}

wxObject *wxStaticBoxXmlHandler::DoCreateResource()
{
    XRC_MAKE_INSTANCE(box, wxStaticBox)

    box->Create(m_parentAsWindow,
                GetID(),
                GetText(wxT("label")),
                GetPosition(), GetSize(),
                GetStyle(),
                GetName());

    SetupWindow(box);

    return box;
}

bool wxStaticBoxXmlHandler::CanHandle(wxXmlNode *node)
{
    return IsOfClass(node, wxT("wxStaticBox"));
}

#endif // wxUSE_XRC && wxUSE_STATBOX
