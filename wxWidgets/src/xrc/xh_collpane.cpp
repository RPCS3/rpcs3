/////////////////////////////////////////////////////////////////////////////
// Name:        src/xrc/xh_collpane.cpp
// Purpose:     XML resource handler for wxCollapsiblePane
// Author:      Francesco Montorsi
// Created:     2006-10-27
// RCS-ID:      $Id: xh_collpane.cpp 43434 2006-11-15 19:00:16Z RR $
// Copyright:   (c) 2006 Francesco Montorsi
// Licence:     wxWindows licence
/////////////////////////////////////////////////////////////////////////////

// For compilers that support precompilation, includes "wx.h".
#include "wx/wxprec.h"

#ifdef __BORLANDC__
    #pragma hdrstop
#endif

#if wxUSE_XRC && wxUSE_COLLPANE

#ifndef WX_PRECOMP
    #include "wx/log.h"
#endif

#include "wx/collpane.h"
#include "wx/xrc/xh_collpane.h"

IMPLEMENT_DYNAMIC_CLASS(wxCollapsiblePaneXmlHandler, wxXmlResourceHandler)

wxCollapsiblePaneXmlHandler::wxCollapsiblePaneXmlHandler() 
: wxXmlResourceHandler(), m_isInside(false)
{
    XRC_ADD_STYLE(wxCP_NO_TLW_RESIZE);
    XRC_ADD_STYLE(wxCP_DEFAULT_STYLE);
    AddWindowStyles();
}

wxObject *wxCollapsiblePaneXmlHandler::DoCreateResource()
{
    if (m_class == wxT("panewindow"))   // read the XRC for the pane window
    {
        wxXmlNode *n = GetParamNode(wxT("object"));

        if ( !n )
            n = GetParamNode(wxT("object_ref"));

        if (n)
        {
            bool old_ins = m_isInside;
            m_isInside = false;
            wxObject *item = CreateResFromNode(n, m_collpane->GetPane(), NULL);
            m_isInside = old_ins;

            return item;
        }
        else
        {
            wxLogError(wxT("Error in resource: no control within collapsible pane's <panewindow> tag."));
            return NULL;
        }
    }
    else
    {
        XRC_MAKE_INSTANCE(ctrl, wxCollapsiblePane)

        wxString label = GetParamValue(wxT("label"));
        if (label.empty())
        {
            wxLogError(wxT("Error in resource: empty label for wxCollapsiblePane"));
            return NULL;
        }

        ctrl->Create(m_parentAsWindow,
                    GetID(),
                    label,
                    GetPosition(), GetSize(),
                    GetStyle(_T("style"), wxCP_DEFAULT_STYLE),
                    wxDefaultValidator,
                    GetName());

        ctrl->Collapse(GetBool(_T("collapsed")));
        SetupWindow(ctrl);

        wxCollapsiblePane *old_par = m_collpane;
        m_collpane = ctrl;
        bool old_ins = m_isInside;
        m_isInside = true;
        CreateChildren(m_collpane, true/*only this handler*/);
        m_isInside = old_ins;
        m_collpane = old_par;

        return ctrl;
    }
}

bool wxCollapsiblePaneXmlHandler::CanHandle(wxXmlNode *node)
{
    return IsOfClass(node, wxT("wxCollapsiblePane")) ||
            (m_isInside && IsOfClass(node, wxT("panewindow")));
}

#endif // wxUSE_XRC && wxUSE_COLLPANE
