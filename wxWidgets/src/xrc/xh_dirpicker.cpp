/////////////////////////////////////////////////////////////////////////////
// Name:        src/xrc/xh_dirpicker.cpp
// Purpose:     XML resource handler for wxDirPickerCtrl
// Author:      Francesco Montorsi
// Created:     2006-04-17
// RCS-ID:      $Id: xh_dirpicker.cpp 55531 2008-09-09 19:36:41Z VS $
// Copyright:   (c) 2006 Francesco Montorsi
// Licence:     wxWindows licence
/////////////////////////////////////////////////////////////////////////////

// For compilers that support precompilation, includes "wx.h".
#include "wx/wxprec.h"

#ifdef __BORLANDC__
    #pragma hdrstop
#endif

#if wxUSE_XRC && wxUSE_DIRPICKERCTRL

#include "wx/xrc/xh_dirpicker.h"
#include "wx/filepicker.h"

IMPLEMENT_DYNAMIC_CLASS(wxDirPickerCtrlXmlHandler, wxXmlResourceHandler)

wxDirPickerCtrlXmlHandler::wxDirPickerCtrlXmlHandler() : wxXmlResourceHandler()
{
    XRC_ADD_STYLE(wxDIRP_USE_TEXTCTRL);
    XRC_ADD_STYLE(wxDIRP_DIR_MUST_EXIST);
    XRC_ADD_STYLE(wxDIRP_CHANGE_DIR);
    XRC_ADD_STYLE(wxDIRP_DEFAULT_STYLE);
    AddWindowStyles();
}

wxObject *wxDirPickerCtrlXmlHandler::DoCreateResource()
{
   XRC_MAKE_INSTANCE(picker, wxDirPickerCtrl)

   picker->Create(m_parentAsWindow,
                  GetID(),
                  GetParamValue(wxT("value")),
                  GetText(wxT("message")),
                  GetPosition(), GetSize(),
                  GetStyle(_T("style"), wxDIRP_DEFAULT_STYLE),
                  wxDefaultValidator,
                  GetName());

    SetupWindow(picker);

    return picker;
}

bool wxDirPickerCtrlXmlHandler::CanHandle(wxXmlNode *node)
{
    return IsOfClass(node, wxT("wxDirPickerCtrl"));
}

#endif // wxUSE_XRC && wxUSE_DIRPICKERCTRL
