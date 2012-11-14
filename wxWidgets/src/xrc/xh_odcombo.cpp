/////////////////////////////////////////////////////////////////////////////
// Name:        src/xrc/xh_odcombo.cpp
// Purpose:     XRC resource for wxRadioBox
// Author:      Alex Bligh - Based on src/xrc/xh_combo.cpp
// Created:     2006/06/19
// RCS-ID:      $Id: xh_odcombo.cpp 42258 2006-10-22 22:12:32Z VZ $
// Copyright:   (c) 2006 Alex Bligh
// Licence:     wxWindows licence
/////////////////////////////////////////////////////////////////////////////

// For compilers that support precompilation, includes "wx.h".
#include "wx/wxprec.h"

#ifdef __BORLANDC__
    #pragma hdrstop
#endif

#if wxUSE_XRC && wxUSE_ODCOMBOBOX

#include "wx/xrc/xh_odcombo.h"

#ifndef WX_PRECOMP
    #include "wx/intl.h"
    #include "wx/textctrl.h"
#endif

#include "wx/odcombo.h"

IMPLEMENT_DYNAMIC_CLASS(wxOwnerDrawnComboBoxXmlHandler, wxXmlResourceHandler)

wxOwnerDrawnComboBoxXmlHandler::wxOwnerDrawnComboBoxXmlHandler()
                     :wxXmlResourceHandler()
                     ,m_insideBox(false)
{
    XRC_ADD_STYLE(wxCB_SIMPLE);
    XRC_ADD_STYLE(wxCB_SORT);
    XRC_ADD_STYLE(wxCB_READONLY);
    XRC_ADD_STYLE(wxCB_DROPDOWN);
    XRC_ADD_STYLE(wxODCB_STD_CONTROL_PAINT);
    XRC_ADD_STYLE(wxODCB_DCLICK_CYCLES);
    XRC_ADD_STYLE(wxTE_PROCESS_ENTER);
    AddWindowStyles();
}

wxObject *wxOwnerDrawnComboBoxXmlHandler::DoCreateResource()
{
    if( m_class == wxT("wxOwnerDrawnComboBox"))
    {
        // find the selection
        long selection = GetLong( wxT("selection"), -1 );

        // need to build the list of strings from children
        m_insideBox = true;
        CreateChildrenPrivately(NULL, GetParamNode(wxT("content")));

        XRC_MAKE_INSTANCE(control, wxOwnerDrawnComboBox)

        control->Create(m_parentAsWindow,
                        GetID(),
                        GetText(wxT("value")),
                        GetPosition(), GetSize(),
                        strList,
                        GetStyle(),
                        wxDefaultValidator,
                        GetName());

        wxSize sizeBtn=GetSize(wxT("buttonsize"));

        if (sizeBtn != wxDefaultSize)
            control->SetButtonPosition(sizeBtn.GetWidth(), sizeBtn.GetHeight());

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

bool wxOwnerDrawnComboBoxXmlHandler::CanHandle(wxXmlNode *node)
{
#if wxCHECK_VERSION(2,7,0)

    return (IsOfClass(node, wxT("wxOwnerDrawnComboBox")) ||
           (m_insideBox && node->GetName() == wxT("item")));

#else

//  Avoid GCC bug - this fails on certain GCC 3.3 and 3.4 builds for an unknown reason
//  it is believed to be related to the fact IsOfClass is inline, and node->GetPropVal
//  gets passed an invalid "this" pointer. On 2.7, the function is out of line, so the
//  above should work fine. This code is left in here so this file can easily be used
//  in a version backported to 2.6. All we are doing here is expanding the macro

    bool fOurClass = node->GetPropVal(wxT("class"), wxEmptyString) == wxT("wxOwnerDrawnComboBox");
    return (fOurClass ||
          (m_insideBox && node->GetName() == wxT("item")));
#endif
}

#endif // wxUSE_XRC && wxUSE_ODCOMBOBOX
