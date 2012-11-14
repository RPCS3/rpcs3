/////////////////////////////////////////////////////////////////////////////
// Name:        src/xrc/xh_slidr.cpp
// Purpose:     XRC resource for wxSlider
// Author:      Bob Mitchell
// Created:     2000/03/21
// RCS-ID:      $Id: xh_slidr.cpp 39600 2006-06-06 16:28:58Z ABX $
// Copyright:   (c) 2000 Bob Mitchell and Verant Interactive
// Licence:     wxWindows licence
/////////////////////////////////////////////////////////////////////////////

// For compilers that support precompilation, includes "wx.h".
#include "wx/wxprec.h"

#ifdef __BORLANDC__
    #pragma hdrstop
#endif

#if wxUSE_XRC && wxUSE_SLIDER

#include "wx/xrc/xh_slidr.h"

#ifndef WX_PRECOMP
    #include "wx/slider.h"
#endif

IMPLEMENT_DYNAMIC_CLASS(wxSliderXmlHandler, wxXmlResourceHandler)

wxSliderXmlHandler::wxSliderXmlHandler()
                   :wxXmlResourceHandler()
{
    XRC_ADD_STYLE(wxSL_HORIZONTAL);
    XRC_ADD_STYLE(wxSL_VERTICAL);
    XRC_ADD_STYLE(wxSL_AUTOTICKS);
    XRC_ADD_STYLE(wxSL_LABELS);
    XRC_ADD_STYLE(wxSL_LEFT);
    XRC_ADD_STYLE(wxSL_TOP);
    XRC_ADD_STYLE(wxSL_RIGHT);
    XRC_ADD_STYLE(wxSL_BOTTOM);
    XRC_ADD_STYLE(wxSL_BOTH);
    XRC_ADD_STYLE(wxSL_SELRANGE);
    XRC_ADD_STYLE(wxSL_INVERSE);
    AddWindowStyles();
}

wxObject *wxSliderXmlHandler::DoCreateResource()
{
    XRC_MAKE_INSTANCE(control, wxSlider)

    control->Create(m_parentAsWindow,
                    GetID(),
                    GetLong(wxT("value"), wxSL_DEFAULT_VALUE),
                    GetLong(wxT("min"), wxSL_DEFAULT_MIN),
                    GetLong(wxT("max"), wxSL_DEFAULT_MAX),
                    GetPosition(), GetSize(),
                    GetStyle(),
                    wxDefaultValidator,
                    GetName());

    if( HasParam(wxT("tickfreq")))
    {
        control->SetTickFreq(GetLong(wxT("tickfreq")), 0);
    }
    if( HasParam(wxT("pagesize")))
    {
        control->SetPageSize(GetLong(wxT("pagesize")));
    }
    if( HasParam(wxT("linesize")))
    {
        control->SetLineSize(GetLong(wxT("linesize")));
    }
    if( HasParam(wxT("thumb")))
    {
        control->SetThumbLength(GetLong(wxT("thumb")));
    }
    if( HasParam(wxT("tick")))
    {
        control->SetTick(GetLong(wxT("tick")));
    }
    if( HasParam(wxT("selmin")) && HasParam(wxT("selmax")))
    {
        control->SetSelection(GetLong(wxT("selmin")), GetLong(wxT("selmax")));
    }

    SetupWindow(control);

    return control;
}

bool wxSliderXmlHandler::CanHandle(wxXmlNode *node)
{
    return IsOfClass(node, wxT("wxSlider"));
}

#endif // wxUSE_XRC && wxUSE_SLIDER
