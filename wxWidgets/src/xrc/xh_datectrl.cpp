/////////////////////////////////////////////////////////////////////////////
// Name:        xh_datectrl.cpp
// Purpose:     XML resource handler for wxDatePickerCtrl
// Author:      Vaclav Slavik
// Created:     2005-02-07
// RCS-ID:      $Id: xh_datectrl.cpp 35650 2005-09-23 12:56:45Z MR $
// Copyright:   (c) 2005 Vaclav Slavik
// Licence:     wxWindows licence
/////////////////////////////////////////////////////////////////////////////

// For compilers that support precompilation, includes "wx.h".
#include "wx/wxprec.h"

#ifdef __BORLANDC__
    #pragma hdrstop
#endif

#if wxUSE_XRC && wxUSE_DATEPICKCTRL

#include "wx/xrc/xh_datectrl.h"
#include "wx/datectrl.h"

IMPLEMENT_DYNAMIC_CLASS(wxDateCtrlXmlHandler, wxXmlResourceHandler)

wxDateCtrlXmlHandler::wxDateCtrlXmlHandler() : wxXmlResourceHandler()
{
    XRC_ADD_STYLE(wxDP_DEFAULT);
    XRC_ADD_STYLE(wxDP_SPIN);
    XRC_ADD_STYLE(wxDP_DROPDOWN);
    XRC_ADD_STYLE(wxDP_ALLOWNONE);
    XRC_ADD_STYLE(wxDP_SHOWCENTURY);
    AddWindowStyles();
}

wxObject *wxDateCtrlXmlHandler::DoCreateResource()
{
   XRC_MAKE_INSTANCE(picker, wxDatePickerCtrl)

   picker->Create(m_parentAsWindow,
                  GetID(),
                  wxDefaultDateTime,
                  GetPosition(), GetSize(),
                  GetStyle(_T("style"), wxDP_DEFAULT | wxDP_SHOWCENTURY),
                  wxDefaultValidator,
                  GetName());

    SetupWindow(picker);

    return picker;
}

bool wxDateCtrlXmlHandler::CanHandle(wxXmlNode *node)
{
    return IsOfClass(node, wxT("wxDatePickerCtrl"));
}

#endif // wxUSE_XRC && wxUSE_DATEPICKCTRL
