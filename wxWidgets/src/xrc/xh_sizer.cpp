/////////////////////////////////////////////////////////////////////////////
// Name:        src/xrc/xh_sizer.cpp
// Purpose:     XRC resource for wxBoxSizer
// Author:      Vaclav Slavik
// Created:     2000/03/21
// RCS-ID:      $Id: xh_sizer.cpp 52334 2008-03-05 15:14:37Z VS $
// Copyright:   (c) 2000 Vaclav Slavik
// Licence:     wxWindows licence
/////////////////////////////////////////////////////////////////////////////

// For compilers that support precompilation, includes "wx.h".
#include "wx/wxprec.h"

#ifdef __BORLANDC__
    #pragma hdrstop
#endif

#if wxUSE_XRC

#include "wx/xrc/xh_sizer.h"

#ifndef WX_PRECOMP
    #include "wx/log.h"
    #include "wx/panel.h"
    #include "wx/statbox.h"
    #include "wx/sizer.h"
    #include "wx/frame.h"
    #include "wx/dialog.h"
    #include "wx/button.h"
    #include "wx/scrolwin.h"
#endif

#include "wx/gbsizer.h"
#include "wx/notebook.h"
#include "wx/tokenzr.h"


//-----------------------------------------------------------------------------
// wxSizerXmlHandler
//-----------------------------------------------------------------------------

IMPLEMENT_DYNAMIC_CLASS(wxSizerXmlHandler, wxXmlResourceHandler)

wxSizerXmlHandler::wxSizerXmlHandler()
                  :wxXmlResourceHandler(),
                   m_isInside(false),
                   m_isGBS(false),
                   m_parentSizer(NULL)
{
    XRC_ADD_STYLE(wxHORIZONTAL);
    XRC_ADD_STYLE(wxVERTICAL);

    // and flags
    XRC_ADD_STYLE(wxLEFT);
    XRC_ADD_STYLE(wxRIGHT);
    XRC_ADD_STYLE(wxTOP);
    XRC_ADD_STYLE(wxBOTTOM);
    XRC_ADD_STYLE(wxNORTH);
    XRC_ADD_STYLE(wxSOUTH);
    XRC_ADD_STYLE(wxEAST);
    XRC_ADD_STYLE(wxWEST);
    XRC_ADD_STYLE(wxALL);

    XRC_ADD_STYLE(wxGROW);
    XRC_ADD_STYLE(wxEXPAND);
    XRC_ADD_STYLE(wxSHAPED);
    XRC_ADD_STYLE(wxSTRETCH_NOT);

    XRC_ADD_STYLE(wxALIGN_CENTER);
    XRC_ADD_STYLE(wxALIGN_CENTRE);
    XRC_ADD_STYLE(wxALIGN_LEFT);
    XRC_ADD_STYLE(wxALIGN_TOP);
    XRC_ADD_STYLE(wxALIGN_RIGHT);
    XRC_ADD_STYLE(wxALIGN_BOTTOM);
    XRC_ADD_STYLE(wxALIGN_CENTER_HORIZONTAL);
    XRC_ADD_STYLE(wxALIGN_CENTRE_HORIZONTAL);
    XRC_ADD_STYLE(wxALIGN_CENTER_VERTICAL);
    XRC_ADD_STYLE(wxALIGN_CENTRE_VERTICAL);

    XRC_ADD_STYLE(wxADJUST_MINSIZE);
    XRC_ADD_STYLE(wxFIXED_MINSIZE);
    XRC_ADD_STYLE(wxRESERVE_SPACE_EVEN_IF_HIDDEN);
}



bool wxSizerXmlHandler::CanHandle(wxXmlNode *node)
{
    return ( (!m_isInside && IsSizerNode(node)) ||
             (m_isInside && IsOfClass(node, wxT("sizeritem"))) ||
             (m_isInside && IsOfClass(node, wxT("spacer")))
        );
}


wxObject* wxSizerXmlHandler::DoCreateResource()
{
    if (m_class == wxT("sizeritem"))
        return Handle_sizeritem();

    else if (m_class == wxT("spacer"))
        return Handle_spacer();

    else
        return Handle_sizer();
}




bool wxSizerXmlHandler::IsSizerNode(wxXmlNode *node)
{
    return (IsOfClass(node, wxT("wxBoxSizer"))) ||
           (IsOfClass(node, wxT("wxStaticBoxSizer"))) ||
           (IsOfClass(node, wxT("wxGridSizer"))) ||
           (IsOfClass(node, wxT("wxFlexGridSizer"))) ||
           (IsOfClass(node, wxT("wxGridBagSizer")));
}


wxObject* wxSizerXmlHandler::Handle_sizeritem()
{
    // find the item to be managed by this sizeritem
    wxXmlNode *n = GetParamNode(wxT("object"));
    if ( !n )
        n = GetParamNode(wxT("object_ref"));

    // did we find one?
    if (n)
    {
        // create a sizer item for it
        wxSizerItem* sitem = MakeSizerItem();

        // now fetch the item to be managed
        bool old_gbs = m_isGBS;
        bool old_ins = m_isInside;
        wxSizer *old_par = m_parentSizer;
        m_isInside = false;
        if (!IsSizerNode(n)) m_parentSizer = NULL;
        wxObject *item = CreateResFromNode(n, m_parent, NULL);
        m_isInside = old_ins;
        m_parentSizer = old_par;
        m_isGBS = old_gbs;

        // and figure out what type it is
        wxSizer *sizer = wxDynamicCast(item, wxSizer);
        wxWindow *wnd = wxDynamicCast(item, wxWindow);

        if (sizer)
            sitem->SetSizer(sizer);
        else if (wnd)
            sitem->SetWindow(wnd);
        else
            wxLogError(wxT("Error in resource."));

        // finally, set other wxSizerItem attributes
        SetSizerItemAttributes(sitem);

        AddSizerItem(sitem);
        return item;
    }
    else /*n == NULL*/
    {
        wxLogError(wxT("Error in resource: no window/sizer/spacer within sizeritem object."));
        return NULL;
    }
}


wxObject* wxSizerXmlHandler::Handle_spacer()
{
    wxCHECK_MSG(m_parentSizer, NULL, wxT("Incorrect syntax of XRC resource: spacer not within sizer!"));

    wxSizerItem* sitem = MakeSizerItem();
    SetSizerItemAttributes(sitem);
    sitem->SetSpacer(GetSize());
    AddSizerItem(sitem);
    return NULL;
}


wxObject* wxSizerXmlHandler::Handle_sizer()
{
    wxSizer *sizer = NULL;

    wxXmlNode *parentNode = m_node->GetParent();

    wxCHECK_MSG(m_parentSizer != NULL ||
                (parentNode && parentNode->GetType() == wxXML_ELEMENT_NODE &&
                 m_parentAsWindow), NULL,
                wxT("Sizer must have a window parent node"));

    if (m_class == wxT("wxBoxSizer"))
        sizer = Handle_wxBoxSizer();

#if wxUSE_STATBOX
    else if (m_class == wxT("wxStaticBoxSizer"))
        sizer = Handle_wxStaticBoxSizer();
#endif

    else if (m_class == wxT("wxGridSizer"))
        sizer = Handle_wxGridSizer();

    else if (m_class == wxT("wxFlexGridSizer"))
        sizer = Handle_wxFlexGridSizer();

    else if (m_class == wxT("wxGridBagSizer"))
        sizer = Handle_wxGridBagSizer();

    if ( !sizer )
    {
        wxLogError(_T("Failed to create size of class \"%s\""), m_class.c_str());
        return NULL;
    }

    wxSize minsize = GetSize(wxT("minsize"));
    if (!(minsize == wxDefaultSize))
        sizer->SetMinSize(minsize);

    // save state
    wxSizer *old_par = m_parentSizer;
    bool old_ins = m_isInside;

    // set new state
    m_parentSizer = sizer;
    m_isInside = true;
    m_isGBS = (m_class == wxT("wxGridBagSizer"));

    CreateChildren(m_parent, true/*only this handler*/);

    // restore state
    m_isInside = old_ins;
    m_parentSizer = old_par;

    if (m_parentSizer == NULL) // setup window:
    {
        m_parentAsWindow->SetSizer(sizer);

        wxXmlNode *nd = m_node;
        m_node = parentNode;
        if (GetSize() == wxDefaultSize)
        {
            if ( wxDynamicCast(m_parentAsWindow, wxScrolledWindow) != NULL )
            {
                sizer->FitInside(m_parentAsWindow);
            }
            else
            {
                sizer->Fit(m_parentAsWindow);
            }
        }
        m_node = nd;

        if (m_parentAsWindow->GetWindowStyle() & (wxMAXIMIZE_BOX | wxRESIZE_BORDER))
            sizer->SetSizeHints(m_parentAsWindow);
    }

    return sizer;
}


wxSizer*  wxSizerXmlHandler::Handle_wxBoxSizer()
{
    return new wxBoxSizer(GetStyle(wxT("orient"), wxHORIZONTAL));
}

#if wxUSE_STATBOX
wxSizer*  wxSizerXmlHandler::Handle_wxStaticBoxSizer()
{
    return new wxStaticBoxSizer(
            new wxStaticBox(m_parentAsWindow,
                            GetID(),
                            GetText(wxT("label")),
                            wxDefaultPosition, wxDefaultSize,
                            0/*style*/,
                            GetName()),
            GetStyle(wxT("orient"), wxHORIZONTAL));
}
#endif // wxUSE_STATBOX

wxSizer*  wxSizerXmlHandler::Handle_wxGridSizer()
{
    return new wxGridSizer(GetLong(wxT("rows")), GetLong(wxT("cols")),
                           GetDimension(wxT("vgap")), GetDimension(wxT("hgap")));
}


wxSizer*  wxSizerXmlHandler::Handle_wxFlexGridSizer()
{
    wxFlexGridSizer *sizer =
        new wxFlexGridSizer(GetLong(wxT("rows")), GetLong(wxT("cols")),
                            GetDimension(wxT("vgap")), GetDimension(wxT("hgap")));
    SetGrowables(sizer, wxT("growablerows"), true);
    SetGrowables(sizer, wxT("growablecols"), false);
    return sizer;
}


wxSizer*  wxSizerXmlHandler::Handle_wxGridBagSizer()
{
    wxGridBagSizer *sizer =
        new wxGridBagSizer(GetDimension(wxT("vgap")), GetDimension(wxT("hgap")));
    SetGrowables(sizer, wxT("growablerows"), true);
    SetGrowables(sizer, wxT("growablecols"), false);
    return sizer;
}




void wxSizerXmlHandler::SetGrowables(wxFlexGridSizer* sizer,
                                     const wxChar* param,
                                     bool rows)
{
    wxStringTokenizer tkn;
    unsigned long l;
    tkn.SetString(GetParamValue(param), wxT(","));
    while (tkn.HasMoreTokens())
    {
        if (!tkn.GetNextToken().ToULong(&l))
            wxLogError(wxT("growable[rows|cols] must be comma-separated list of row numbers"));
        else {
            if (rows)
                sizer->AddGrowableRow(l);
            else
                sizer->AddGrowableCol(l);
        }
    }
}


wxGBPosition wxSizerXmlHandler::GetGBPos(const wxString& param)
{
    wxSize sz = GetSize(param);
    if (sz.x < 0) sz.x = 0;
    if (sz.y < 0) sz.y = 0;
    return wxGBPosition(sz.x, sz.y);
}

wxGBSpan wxSizerXmlHandler::GetGBSpan(const wxString& param)
{
    wxSize sz = GetSize(param);
    if (sz.x < 1) sz.x = 1;
    if (sz.y < 1) sz.y = 1;
    return wxGBSpan(sz.x, sz.y);
}



wxSizerItem* wxSizerXmlHandler::MakeSizerItem()
{
    if (m_isGBS)
        return new wxGBSizerItem();
    else
        return new wxSizerItem();
}

void wxSizerXmlHandler::SetSizerItemAttributes(wxSizerItem* sitem)
{
    sitem->SetProportion(GetLong(wxT("option")));  // Should this check for "proportion" too?
    sitem->SetFlag(GetStyle(wxT("flag")));
    sitem->SetBorder(GetDimension(wxT("border")));
    wxSize sz = GetSize(wxT("minsize"));
    if (!(sz == wxDefaultSize))
        sitem->SetMinSize(sz);
    sz = GetSize(wxT("ratio"));
    if (!(sz == wxDefaultSize))
        sitem->SetRatio(sz);

    if (m_isGBS)
    {
        wxGBSizerItem* gbsitem = (wxGBSizerItem*)sitem;
        gbsitem->SetPos(GetGBPos(wxT("cellpos")));
        gbsitem->SetSpan(GetGBSpan(wxT("cellspan")));
    }
}

void wxSizerXmlHandler::AddSizerItem(wxSizerItem* sitem)
{
    if (m_isGBS)
        ((wxGridBagSizer*)m_parentSizer)->Add((wxGBSizerItem*)sitem);
    else
        m_parentSizer->Add(sitem);
}



//-----------------------------------------------------------------------------
// wxStdDialogButtonSizerXmlHandler
//-----------------------------------------------------------------------------
#if wxUSE_BUTTON

IMPLEMENT_DYNAMIC_CLASS(wxStdDialogButtonSizerXmlHandler, wxXmlResourceHandler)

wxStdDialogButtonSizerXmlHandler::wxStdDialogButtonSizerXmlHandler()
    : m_isInside(false), m_parentSizer(NULL)
{
}

wxObject *wxStdDialogButtonSizerXmlHandler::DoCreateResource()
{
    if (m_class == wxT("wxStdDialogButtonSizer"))
    {
        wxASSERT( !m_parentSizer );

        wxSizer *s = m_parentSizer = new wxStdDialogButtonSizer;
        m_isInside = true;

        CreateChildren(m_parent, true/*only this handler*/);

        m_parentSizer->Realize();

        m_isInside = false;
        m_parentSizer = NULL;

        return s;
    }
    else // m_class == "button"
    {
        wxASSERT( m_parentSizer );

        // find the item to be managed by this sizeritem
        wxXmlNode *n = GetParamNode(wxT("object"));
        if ( !n )
            n = GetParamNode(wxT("object_ref"));

        // did we find one?
        if (n)
        {
            wxObject *item = CreateResFromNode(n, m_parent, NULL);
            wxButton *button = wxDynamicCast(item, wxButton);

            if (button)
                m_parentSizer->AddButton(button);
            else
                wxLogError(wxT("Error in resource - expected button."));

            return item;
        }
        else /*n == NULL*/
        {
            wxLogError(wxT("Error in resource: no button within wxStdDialogButtonSizer."));
            return NULL;
        }
    }
}

bool wxStdDialogButtonSizerXmlHandler::CanHandle(wxXmlNode *node)
{
    return (!m_isInside && IsOfClass(node, wxT("wxStdDialogButtonSizer"))) ||
           (m_isInside && IsOfClass(node, wxT("button")));
}
#endif // wxUSE_BUTTON

#endif // wxUSE_XRC
