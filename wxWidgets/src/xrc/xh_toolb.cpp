/////////////////////////////////////////////////////////////////////////////
// Name:        src/xrc/xh_toolb.cpp
// Purpose:     XRC resource for wxToolBar
// Author:      Vaclav Slavik
// Created:     2000/08/11
// RCS-ID:      $Id: xh_toolb.cpp 55979 2008-09-30 14:37:26Z VS $
// Copyright:   (c) 2000 Vaclav Slavik
// Licence:     wxWindows licence
/////////////////////////////////////////////////////////////////////////////

// For compilers that support precompilation, includes "wx.h".
#include "wx/wxprec.h"

#ifdef __BORLANDC__
    #pragma hdrstop
#endif

#if wxUSE_XRC && wxUSE_TOOLBAR

#include "wx/xrc/xh_toolb.h"

#ifndef WX_PRECOMP
    #include "wx/frame.h"
    #include "wx/toolbar.h"
#endif

IMPLEMENT_DYNAMIC_CLASS(wxToolBarXmlHandler, wxXmlResourceHandler)

wxToolBarXmlHandler::wxToolBarXmlHandler()
: wxXmlResourceHandler(), m_isInside(false), m_toolbar(NULL)
{
    XRC_ADD_STYLE(wxTB_FLAT);
    XRC_ADD_STYLE(wxTB_DOCKABLE);
    XRC_ADD_STYLE(wxTB_VERTICAL);
    XRC_ADD_STYLE(wxTB_HORIZONTAL);
    XRC_ADD_STYLE(wxTB_3DBUTTONS);
    XRC_ADD_STYLE(wxTB_TEXT);
    XRC_ADD_STYLE(wxTB_NOICONS);
    XRC_ADD_STYLE(wxTB_NODIVIDER);
    XRC_ADD_STYLE(wxTB_NOALIGN);
    XRC_ADD_STYLE(wxTB_HORZ_LAYOUT);
    XRC_ADD_STYLE(wxTB_HORZ_TEXT);

    XRC_ADD_STYLE(wxTB_TOP);
    XRC_ADD_STYLE(wxTB_LEFT);
    XRC_ADD_STYLE(wxTB_RIGHT);
    XRC_ADD_STYLE(wxTB_BOTTOM);

    AddWindowStyles();
}

wxObject *wxToolBarXmlHandler::DoCreateResource()
{
    if (m_class == wxT("tool"))
    {
        wxCHECK_MSG(m_toolbar, NULL, wxT("Incorrect syntax of XRC resource: tool not within a toolbar!"));

        if (GetPosition() != wxDefaultPosition)
        {
            m_toolbar->AddTool(GetID(),
                               GetBitmap(wxT("bitmap"), wxART_TOOLBAR),
                               GetBitmap(wxT("bitmap2"), wxART_TOOLBAR),
                               GetBool(wxT("toggle")),
                               GetPosition().x,
                               GetPosition().y,
                               NULL,
                               GetText(wxT("tooltip")),
                               GetText(wxT("longhelp")));
        }
        else
        {
            wxItemKind kind = wxITEM_NORMAL;
            if (GetBool(wxT("radio")))
                kind = wxITEM_RADIO;
            if (GetBool(wxT("toggle")))
            {
                wxASSERT_MSG( kind == wxITEM_NORMAL,
                              _T("can't have both toggleable and radion button at once") );
                kind = wxITEM_CHECK;
            }
            m_toolbar->AddTool(GetID(),
                               GetText(wxT("label")),
                               GetBitmap(wxT("bitmap"), wxART_TOOLBAR),
                               GetBitmap(wxT("bitmap2"), wxART_TOOLBAR),
                               kind,
                               GetText(wxT("tooltip")),
                               GetText(wxT("longhelp")));

            if ( GetBool(wxT("disabled")) )
                m_toolbar->EnableTool(GetID(), false);
        }
        return m_toolbar; // must return non-NULL
    }

    else if (m_class == wxT("separator"))
    {
        wxCHECK_MSG(m_toolbar, NULL, wxT("Incorrect syntax of XRC resource: separator not within a toolbar!"));
        m_toolbar->AddSeparator();
        return m_toolbar; // must return non-NULL
    }

    else /*<object class="wxToolBar">*/
    {
        int style = GetStyle(wxT("style"), wxNO_BORDER | wxTB_HORIZONTAL);
#ifdef __WXMSW__
        if (!(style & wxNO_BORDER)) style |= wxNO_BORDER;
#endif

        XRC_MAKE_INSTANCE(toolbar, wxToolBar)

        toolbar->Create(m_parentAsWindow,
                         GetID(),
                         GetPosition(),
                         GetSize(),
                         style,
                         GetName());
        SetupWindow(toolbar);

        wxSize bmpsize = GetSize(wxT("bitmapsize"));
        if (!(bmpsize == wxDefaultSize))
            toolbar->SetToolBitmapSize(bmpsize);
        wxSize margins = GetSize(wxT("margins"));
        if (!(margins == wxDefaultSize))
            toolbar->SetMargins(margins.x, margins.y);
        long packing = GetLong(wxT("packing"), -1);
        if (packing != -1)
            toolbar->SetToolPacking(packing);
        long separation = GetLong(wxT("separation"), -1);
        if (separation != -1)
            toolbar->SetToolSeparation(separation);

        wxXmlNode *children_node = GetParamNode(wxT("object"));
        if (!children_node)
           children_node = GetParamNode(wxT("object_ref"));

        if (children_node == NULL) return toolbar;

        m_isInside = true;
        m_toolbar = toolbar;

        wxXmlNode *n = children_node;

        while (n)
        {
            if ((n->GetType() == wxXML_ELEMENT_NODE) &&
                (n->GetName() == wxT("object") || n->GetName() == wxT("object_ref")))
            {
                wxObject *created = CreateResFromNode(n, toolbar, NULL);
                wxControl *control = wxDynamicCast(created, wxControl);
                if (!IsOfClass(n, wxT("tool")) &&
                    !IsOfClass(n, wxT("separator")) &&
                    control != NULL)
                    toolbar->AddControl(control);
            }
            n = n->GetNext();
        }

        m_isInside = false;
        m_toolbar = NULL;

        toolbar->Realize();

        if (m_parentAsWindow && !GetBool(wxT("dontattachtoframe")))
        {
            wxFrame *parentFrame = wxDynamicCast(m_parent, wxFrame);
            if (parentFrame)
                parentFrame->SetToolBar(toolbar);
        }

        return toolbar;
    }
}

bool wxToolBarXmlHandler::CanHandle(wxXmlNode *node)
{
    return ((!m_isInside && IsOfClass(node, wxT("wxToolBar"))) ||
            (m_isInside && IsOfClass(node, wxT("tool"))) ||
            (m_isInside && IsOfClass(node, wxT("separator"))));
}

#endif // wxUSE_XRC && wxUSE_TOOLBAR
