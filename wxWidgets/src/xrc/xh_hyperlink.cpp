/////////////////////////////////////////////////////////////////////////////
// Name:        src/xrc/xh_hyperlink.cpp
// Purpose:     Hyperlink control
// Author:      David Norris <danorris@gmail.com>
// Modified by: Ryan Norton, Francesco Montorsi
// Created:     04/02/2005
// RCS-ID:      $Id: xh_hyperlink.cpp 54563 2008-07-09 14:02:19Z VZ $
// Copyright:   (c) 2005 David Norris
// Licence:     wxWindows licence
/////////////////////////////////////////////////////////////////////////////

//===========================================================================
// Declarations
//===========================================================================

//---------------------------------------------------------------------------
// Pre-compiled header stuff
//---------------------------------------------------------------------------

// For compilers that support precompilation, includes "wx.h".
#include "wx/wxprec.h"

#ifdef __BORLANDC__
#pragma hdrstop
#endif

#if wxUSE_XRC && wxUSE_HYPERLINKCTRL

//---------------------------------------------------------------------------
// Includes
//---------------------------------------------------------------------------

#include "wx/xrc/xh_hyperlink.h"

#ifndef WX_PRECOMP
#endif

#include "wx/hyperlink.h"
#include "wx/xrc/xmlres.h"

//===========================================================================
// Implementation
//===========================================================================

//---------------------------------------------------------------------------
// wxHyperlinkCtrlXmlHandler
//---------------------------------------------------------------------------

// Register with wxWindows' dynamic class subsystem.
IMPLEMENT_DYNAMIC_CLASS(wxHyperlinkCtrlXmlHandler, wxXmlResourceHandler)

wxHyperlinkCtrlXmlHandler::wxHyperlinkCtrlXmlHandler()
{
    XRC_ADD_STYLE(wxHL_CONTEXTMENU);
    XRC_ADD_STYLE(wxHL_ALIGN_LEFT);
    XRC_ADD_STYLE(wxHL_ALIGN_RIGHT);
    XRC_ADD_STYLE(wxHL_ALIGN_CENTRE);
    XRC_ADD_STYLE(wxHL_DEFAULT_STYLE);

    AddWindowStyles();
}

wxObject *wxHyperlinkCtrlXmlHandler::DoCreateResource()
{
    XRC_MAKE_INSTANCE(control, wxHyperlinkCtrl)

    control->Create
             (
                m_parentAsWindow,
                GetID(),
                GetText(wxT("label")),
                GetParamValue(wxT("url")),
                GetPosition(), GetSize(),
                GetStyle(wxT("style"), wxHL_DEFAULT_STYLE),
                GetName()
             );

    SetupWindow(control);

    return control;
}

bool wxHyperlinkCtrlXmlHandler::CanHandle(wxXmlNode *node)
{
    return IsOfClass(node, wxT("wxHyperlinkCtrl"));
}

#endif // wxUSE_XRC && wxUSE_HYPERLINKCTRL
