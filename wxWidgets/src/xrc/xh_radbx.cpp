/////////////////////////////////////////////////////////////////////////////
// Name:        src/xrc/xh_radbx.cpp
// Purpose:     XRC resource for wxRadioBox
// Author:      Bob Mitchell
// Created:     2000/03/21
// RCS-ID:      $Id: xh_radbx.cpp 40728 2006-08-21 18:27:15Z RD $
// Copyright:   (c) 2000 Bob Mitchell and Verant Interactive
// Licence:     wxWindows licence
/////////////////////////////////////////////////////////////////////////////

// For compilers that support precompilation, includes "wx.h".
#include "wx/wxprec.h"

#ifdef __BORLANDC__
    #pragma hdrstop
#endif

#if wxUSE_XRC && wxUSE_RADIOBOX

#include "wx/xrc/xh_radbx.h"

#ifndef WX_PRECOMP
    #include "wx/intl.h"
    #include "wx/radiobox.h"
#endif

IMPLEMENT_DYNAMIC_CLASS(wxRadioBoxXmlHandler, wxXmlResourceHandler)

wxRadioBoxXmlHandler::wxRadioBoxXmlHandler()
: wxXmlResourceHandler(), m_insideBox(false)
{
    XRC_ADD_STYLE(wxRA_SPECIFY_COLS);
    XRC_ADD_STYLE(wxRA_HORIZONTAL);
    XRC_ADD_STYLE(wxRA_SPECIFY_ROWS);
    XRC_ADD_STYLE(wxRA_VERTICAL);
    AddWindowStyles();
}

wxObject *wxRadioBoxXmlHandler::DoCreateResource()
{
    if ( m_class == wxT("wxRadioBox"))
    {
        // find the selection
        long selection = GetLong( wxT("selection"), -1 );

        // need to build the list of strings from children
        m_insideBox = true;
        CreateChildrenPrivately( NULL, GetParamNode(wxT("content")));

        wxString *strings;
        if ( !labels.empty() )
        {
            strings = new wxString[labels.size()];
            const unsigned count = labels.size();
            for( unsigned i = 0; i < count; i++ )
                strings[i] = labels[i];
        }
        else
        {
            strings = NULL;
        }

        XRC_MAKE_INSTANCE(control, wxRadioBox)

        control->Create(m_parentAsWindow,
                        GetID(),
                        GetText(wxT("label")),
                        GetPosition(), GetSize(),
                        labels.size(),
                        strings,
                        GetLong(wxT("dimension"), 1),
                        GetStyle(),
                        wxDefaultValidator,
                        GetName());

        delete[] strings;

        if (selection != -1)
            control->SetSelection(selection);

        SetupWindow(control);

        const unsigned count = labels.size();
        for( unsigned i = 0; i < count; i++ )
        {
#if wxUSE_TOOLTIPS
            if ( !tooltips[i].empty() )
                control->SetItemToolTip(i, tooltips[i]);
#endif // wxUSE_TOOLTIPS
#if wxUSE_HELP
            if ( helptextSpecified[i] )
                control->SetItemHelpText(i, helptexts[i]);
#endif // wxUSE_HELP
        }

        labels.clear();    // dump the strings

        tooltips.clear();    // dump the tooltips

        helptexts.clear();   // dump the helptexts
        helptextSpecified.clear();

        return control;
    }
    else // inside the radiobox element
    {
        // we handle handle <item tooltip="..." helptext="...">Label</item> constructs here

        wxString str = GetNodeContent(m_node);

        wxString tooltip;
        m_node->GetPropVal(wxT("tooltip"), &tooltip);

        wxString helptext;
        bool hasHelptext = m_node->GetPropVal(wxT("helptext"), &helptext);

        if (m_resource->GetFlags() & wxXRC_USE_LOCALE)
        {
            str = wxGetTranslation(str, m_resource->GetDomain());
            if ( !tooltip.empty() )
                tooltip = wxGetTranslation(tooltip, m_resource->GetDomain());
            if ( hasHelptext )
                helptext = wxGetTranslation(helptext, m_resource->GetDomain());
        }

        labels.push_back(str);
        tooltips.push_back(tooltip);
        helptexts.push_back(helptext);
        helptextSpecified.push_back(hasHelptext);

        return NULL;
    }

}

bool wxRadioBoxXmlHandler::CanHandle(wxXmlNode *node)
{
    return (IsOfClass(node, wxT("wxRadioBox")) ||
           (m_insideBox && node->GetName() == wxT("item")));
}

#endif // wxUSE_XRC && wxUSE_RADIOBOX
