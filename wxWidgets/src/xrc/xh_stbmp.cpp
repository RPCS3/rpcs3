/////////////////////////////////////////////////////////////////////////////
// Name:        src/xrc/xh_stbmp.cpp
// Purpose:     XRC resource for wxStaticBitmap
// Author:      Vaclav Slavik
// Created:     2000/04/22
// RCS-ID:      $Id: xh_stbmp.cpp 39481 2006-05-31 09:34:05Z ABX $
// Copyright:   (c) 2000 Vaclav Slavik
// Licence:     wxWindows licence
/////////////////////////////////////////////////////////////////////////////

// For compilers that support precompilation, includes "wx.h".
#include "wx/wxprec.h"

#ifdef __BORLANDC__
    #pragma hdrstop
#endif

#if wxUSE_XRC && wxUSE_STATBMP

#include "wx/xrc/xh_stbmp.h"

#ifndef  WX_PRECOMP
    #include "wx/statbmp.h"
#endif

IMPLEMENT_DYNAMIC_CLASS(wxStaticBitmapXmlHandler, wxXmlResourceHandler)

wxStaticBitmapXmlHandler::wxStaticBitmapXmlHandler()
                         :wxXmlResourceHandler()
{
    AddWindowStyles();
}

wxObject *wxStaticBitmapXmlHandler::DoCreateResource()
{
    XRC_MAKE_INSTANCE(bmp, wxStaticBitmap)

    bmp->Create(m_parentAsWindow,
                GetID(),
                GetBitmap(wxT("bitmap"), wxART_OTHER, GetSize()),
                GetPosition(), GetSize(),
                GetStyle(),
                GetName());

    SetupWindow(bmp);

    return bmp;
}

bool wxStaticBitmapXmlHandler::CanHandle(wxXmlNode *node)
{
    return IsOfClass(node, wxT("wxStaticBitmap"));
}

#endif // wxUSE_XRC && wxUSE_STATBMP
