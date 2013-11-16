/////////////////////////////////////////////////////////////////////////////
// Name:        src/xrc/xh_richtext.cpp
// Purpose:     XRC resource for wxRichTextCtrl
// Author:      Julian Smart
// Created:     2006-11-08
// RCS-ID:      $Id: xh_richtext.cpp 43219 2006-11-09 10:40:41Z JS $
// Copyright:   (c) 2006 Julian Smart
// Licence:     wxWindows licence
/////////////////////////////////////////////////////////////////////////////

// For compilers that support precompilation, includes "wx.h".
#include "wx/wxprec.h"

#ifdef __BORLANDC__
    #pragma hdrstop
#endif

#if wxUSE_XRC && wxUSE_RICHTEXT && wxUSE_RICHTEXT_XML_HANDLER

#include "wx/xrc/xh_richtext.h"

#include "wx/richtext/richtextctrl.h"

IMPLEMENT_DYNAMIC_CLASS(wxRichTextCtrlXmlHandler, wxXmlResourceHandler)

wxRichTextCtrlXmlHandler::wxRichTextCtrlXmlHandler() : wxXmlResourceHandler()
{
    XRC_ADD_STYLE(wxTE_PROCESS_ENTER);
    XRC_ADD_STYLE(wxTE_PROCESS_TAB);
    XRC_ADD_STYLE(wxTE_MULTILINE);
    XRC_ADD_STYLE(wxTE_READONLY);
    XRC_ADD_STYLE(wxTE_AUTO_URL);

    AddWindowStyles();
}

wxObject *wxRichTextCtrlXmlHandler::DoCreateResource()
{
    XRC_MAKE_INSTANCE(text, wxRichTextCtrl)

    text->Create(m_parentAsWindow,
                 GetID(),
                 GetText(wxT("value")),
                 GetPosition(), GetSize(),
                 GetStyle(),
                 wxDefaultValidator,
                 GetName());

    SetupWindow(text);

    if (HasParam(wxT("maxlength")))
        text->SetMaxLength(GetLong(wxT("maxlength")));

    return text;
}

bool wxRichTextCtrlXmlHandler::CanHandle(wxXmlNode *node)
{
    return IsOfClass(node, wxT("wxRichTextCtrl"));
}

#endif // wxUSE_XRC && wxUSE_RICHTEXT

