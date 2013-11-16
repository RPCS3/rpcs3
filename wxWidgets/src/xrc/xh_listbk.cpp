/////////////////////////////////////////////////////////////////////////////
// Name:        src/xrc/xh_listbk.cpp
// Purpose:     XRC resource for wxListbook
// Author:      Vaclav Slavik
// Created:     2000/03/21
// RCS-ID:      $Id: xh_listbk.cpp 39627 2006-06-08 06:57:39Z ABX $
// Copyright:   (c) 2000 Vaclav Slavik
// Licence:     wxWindows licence
/////////////////////////////////////////////////////////////////////////////

// For compilers that support precompilation, includes "wx.h".
#include "wx/wxprec.h"

#ifdef __BORLANDC__
    #pragma hdrstop
#endif

#if wxUSE_XRC && wxUSE_LISTBOOK

#include "wx/xrc/xh_listbk.h"

#ifndef WX_PRECOMP
    #include "wx/log.h"
    #include "wx/sizer.h"
#endif

#include "wx/listbook.h"
#include "wx/imaglist.h"

IMPLEMENT_DYNAMIC_CLASS(wxListbookXmlHandler, wxXmlResourceHandler)

wxListbookXmlHandler::wxListbookXmlHandler()
                     :wxXmlResourceHandler(),
                      m_isInside(false),
                      m_listbook(NULL)
{
    XRC_ADD_STYLE(wxBK_DEFAULT);
    XRC_ADD_STYLE(wxBK_LEFT);
    XRC_ADD_STYLE(wxBK_RIGHT);
    XRC_ADD_STYLE(wxBK_TOP);
    XRC_ADD_STYLE(wxBK_BOTTOM);

#if WXWIN_COMPATIBILITY_2_6
    XRC_ADD_STYLE(wxLB_DEFAULT);
    XRC_ADD_STYLE(wxLB_LEFT);
    XRC_ADD_STYLE(wxLB_RIGHT);
    XRC_ADD_STYLE(wxLB_TOP);
    XRC_ADD_STYLE(wxLB_BOTTOM);
#endif

    AddWindowStyles();
}

wxObject *wxListbookXmlHandler::DoCreateResource()
{
    if (m_class == wxT("listbookpage"))
    {
        wxXmlNode *n = GetParamNode(wxT("object"));

        if ( !n )
            n = GetParamNode(wxT("object_ref"));

        if (n)
        {
            bool old_ins = m_isInside;
            m_isInside = false;
            wxObject *item = CreateResFromNode(n, m_listbook, NULL);
            m_isInside = old_ins;
            wxWindow *wnd = wxDynamicCast(item, wxWindow);

            if (wnd)
            {
                m_listbook->AddPage(wnd, GetText(wxT("label")),
                                         GetBool(wxT("selected")));
                if ( HasParam(wxT("bitmap")) )
                {
                    wxBitmap bmp = GetBitmap(wxT("bitmap"), wxART_OTHER);
                    wxImageList *imgList = m_listbook->GetImageList();
                    if ( imgList == NULL )
                    {
                        imgList = new wxImageList( bmp.GetWidth(), bmp.GetHeight() );
                        m_listbook->AssignImageList( imgList );
                    }
                    int imgIndex = imgList->Add(bmp);
                    m_listbook->SetPageImage(m_listbook->GetPageCount()-1, imgIndex );
                }
            }
            else
                wxLogError(wxT("Error in resource."));
            return wnd;
        }
        else
        {
            wxLogError(wxT("Error in resource: no control within listbook's <page> tag."));
            return NULL;
        }
    }

    else
    {
        XRC_MAKE_INSTANCE(nb, wxListbook)

        nb->Create(m_parentAsWindow,
                   GetID(),
                   GetPosition(), GetSize(),
                   GetStyle(wxT("style")),
                   GetName());

        wxListbook *old_par = m_listbook;
        m_listbook = nb;
        bool old_ins = m_isInside;
        m_isInside = true;
        CreateChildren(m_listbook, true/*only this handler*/);
        m_isInside = old_ins;
        m_listbook = old_par;

        return nb;
    }
}

bool wxListbookXmlHandler::CanHandle(wxXmlNode *node)
{
    return ((!m_isInside && IsOfClass(node, wxT("wxListbook"))) ||
            (m_isInside && IsOfClass(node, wxT("listbookpage"))));
}

#endif // wxUSE_XRC && wxUSE_LISTBOOK
