/////////////////////////////////////////////////////////////////////////////
// Name:        src/xrc/xh_combo.cpp
// Purpose:     XRC resource for wxComboBox
// Author:      Bob Mitchell
// Created:     2000/03/21
// RCS-ID:      $Id: xh_combo.cpp 56715 2008-11-09 12:40:07Z VZ $
// Copyright:   (c) 2000 Bob Mitchell and Verant Interactive
// Licence:     wxWindows licence
/////////////////////////////////////////////////////////////////////////////

// For compilers that support precompilation, includes "wx.h".
#include "wx/wxprec.h"

#ifdef __BORLANDC__
    #pragma hdrstop
#endif

#if wxUSE_XRC && wxUSE_COMBOBOX

#include "wx/xrc/xh_combo.h"

#ifndef WX_PRECOMP
    #include "wx/intl.h"
    #include "wx/combobox.h"
    #include "wx/textctrl.h" // for wxTE_PROCESS_ENTER
#endif

IMPLEMENT_DYNAMIC_CLASS(wxComboBoxXmlHandler, wxXmlResourceHandler)

wxComboBoxXmlHandler::wxComboBoxXmlHandler()
                     :wxXmlResourceHandler()
                     ,m_insideBox(false)
{
    XRC_ADD_STYLE(wxCB_SIMPLE);
    XRC_ADD_STYLE(wxCB_SORT);
    XRC_ADD_STYLE(wxCB_READONLY);
    XRC_ADD_STYLE(wxCB_DROPDOWN);
    XRC_ADD_STYLE(wxTE_PROCESS_ENTER);
    AddWindowStyles();
}

wxObject *wxComboBoxXmlHandler::DoCreateResource()
{
    if( m_class == wxT("wxComboBox"))
    {
        // find the selection
        long selection = GetLong( wxT("selection"), -1 );

        // need to build the list of strings from children
        m_insideBox = true;
        CreateChildrenPrivately(NULL, GetParamNode(wxT("content")));

        XRC_MAKE_INSTANCE(control, wxComboBox)

        control->Create(m_parentAsWindow,
                        GetID(),
                        GetText(wxT("value")),
                        GetPosition(), GetSize(),
                        strList,
                        GetStyle(),
                        wxDefaultValidator,
                        GetName());

        if (selection != -1)
            control->SetSelection(selection);

        SetupWindow(control);

        strList.Clear();    // dump the strings

        return control;
    }
    else
    {
        // on the inside now.
        // handle <item>Label</item>

        // add to the list
        wxString str = GetNodeContent(m_node);
        if (m_resource->GetFlags() & wxXRC_USE_LOCALE)
            str = wxGetTranslation(str, m_resource->GetDomain());
        strList.Add(str);

        return NULL;
    }
}

bool wxComboBoxXmlHandler::CanHandle(wxXmlNode *node)
{
    return (IsOfClass(node, wxT("wxComboBox")) ||
           (m_insideBox && node->GetName() == wxT("item")));
}

#endif // wxUSE_XRC && wxUSE_COMBOBOX
