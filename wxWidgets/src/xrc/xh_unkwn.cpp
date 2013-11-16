/////////////////////////////////////////////////////////////////////////////
// Name:        src/xrc/xh_unkwn.cpp
// Purpose:     XRC resource for unknown widget
// Author:      Vaclav Slavik
// Created:     2000/09/09
// RCS-ID:      $Id: xh_unkwn.cpp 44457 2007-02-11 02:34:57Z VZ $
// Copyright:   (c) 2000 Vaclav Slavik
// Licence:     wxWindows licence
/////////////////////////////////////////////////////////////////////////////

// For compilers that support precompilation, includes "wx.h".
#include "wx/wxprec.h"

#ifdef __BORLANDC__
    #pragma hdrstop
#endif

#if wxUSE_XRC

#include "wx/xrc/xh_unkwn.h"

#ifndef WX_PRECOMP
    #include "wx/log.h"
    #include "wx/window.h"
    #include "wx/panel.h"
    #include "wx/sizer.h"
#endif


class wxUnknownControlContainer : public wxPanel
{
public:
    wxUnknownControlContainer(wxWindow *parent,
                              const wxString& controlName,
                              wxWindowID id = wxID_ANY,
                              const wxPoint& pos = wxDefaultPosition,
                              const wxSize& size = wxDefaultSize,
                              long style = 0)
        // Always add the wxTAB_TRAVERSAL and wxNO_BORDER styles to what comes
        // from the XRC if anything.
        : wxPanel(parent, id, pos, size, style | wxTAB_TRAVERSAL | wxNO_BORDER,
                  controlName + wxT("_container")),
          m_controlName(controlName), m_controlAdded(false)
    {
        m_bg = GetBackgroundColour();
        SetBackgroundColour(wxColour(255, 0, 255));
    }

    virtual void AddChild(wxWindowBase *child);
    virtual void RemoveChild(wxWindowBase *child);

protected:
    wxString m_controlName;
    bool m_controlAdded;
    wxColour m_bg;
};

void wxUnknownControlContainer::AddChild(wxWindowBase *child)
{
    wxASSERT_MSG( !m_controlAdded, wxT("Couldn't add two unknown controls to the same container!") );

    wxPanel::AddChild(child);

    SetBackgroundColour(m_bg);
    child->SetName(m_controlName);
    child->SetId(wxXmlResource::GetXRCID(m_controlName));
    m_controlAdded = true;

    wxSizer *sizer = new wxBoxSizer(wxHORIZONTAL);
    sizer->Add((wxWindow*)child, 1, wxEXPAND);
    SetSizerAndFit(sizer);
}

void wxUnknownControlContainer::RemoveChild(wxWindowBase *child)
{
    wxPanel::RemoveChild(child);
    m_controlAdded = false;
    GetSizer()->Detach((wxWindow*)child);
}


IMPLEMENT_DYNAMIC_CLASS(wxUnknownWidgetXmlHandler, wxXmlResourceHandler)

wxUnknownWidgetXmlHandler::wxUnknownWidgetXmlHandler()
: wxXmlResourceHandler()
{
    XRC_ADD_STYLE(wxNO_FULL_REPAINT_ON_RESIZE);
}

wxObject *wxUnknownWidgetXmlHandler::DoCreateResource()
{
    wxASSERT_MSG( m_instance == NULL,
                  _T("'unknown' controls can't be subclassed, use wxXmlResource::AttachUnknownControl") );

    wxPanel *panel =
        new wxUnknownControlContainer(m_parentAsWindow,
                                      GetName(), wxID_ANY,
                                      GetPosition(), GetSize(),
                                      GetStyle(wxT("style")));
    SetupWindow(panel);
    return panel;
}

bool wxUnknownWidgetXmlHandler::CanHandle(wxXmlNode *node)
{
    return IsOfClass(node, wxT("unknown"));
}

#endif // wxUSE_XRC
