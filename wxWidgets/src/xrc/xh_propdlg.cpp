/////////////////////////////////////////////////////////////////////////////
// Name:        src/xrc/xh_propdlg.cpp
// Purpose:     XRC resource handler for wxPropertySheetDialog
// Author:      Sander Berents
// Created:     2007/07/12
// RCS-ID:      $Id: xh_propdlg.cpp 48088 2007-08-15 11:43:32Z VZ $
// Copyright:   (c) 2007 Sander Berents
// Licence:     wxWindows licence
/////////////////////////////////////////////////////////////////////////////

// For compilers that support precompilation, includes "wx.h".
#include "wx/wxprec.h"

#ifdef __BORLANDC__
    #pragma hdrstop
#endif

#if wxUSE_XRC && wxUSE_BOOKCTRL

#include "wx/xrc/xh_propdlg.h"

#ifndef WX_PRECOMP
    #include "wx/log.h"
    #include "wx/sizer.h"
    #include "wx/frame.h"
#endif

#include "wx/bookctrl.h"
#include "wx/propdlg.h"
#include "wx/imaglist.h"

IMPLEMENT_DYNAMIC_CLASS(wxPropertySheetDialogXmlHandler, wxXmlResourceHandler)

wxPropertySheetDialogXmlHandler::wxPropertySheetDialogXmlHandler()
                     :wxXmlResourceHandler(),
                      m_isInside(false),
                      m_dialog(NULL)
{
    XRC_ADD_STYLE(wxSTAY_ON_TOP);
    XRC_ADD_STYLE(wxCAPTION);
    XRC_ADD_STYLE(wxDEFAULT_DIALOG_STYLE);
    XRC_ADD_STYLE(wxSYSTEM_MENU);
    XRC_ADD_STYLE(wxRESIZE_BORDER);
    XRC_ADD_STYLE(wxCLOSE_BOX);
    XRC_ADD_STYLE(wxDIALOG_NO_PARENT);

    XRC_ADD_STYLE(wxTAB_TRAVERSAL);
    XRC_ADD_STYLE(wxWS_EX_VALIDATE_RECURSIVELY);
    XRC_ADD_STYLE(wxDIALOG_EX_METAL);
    XRC_ADD_STYLE(wxMAXIMIZE_BOX);
    XRC_ADD_STYLE(wxMINIMIZE_BOX);
    XRC_ADD_STYLE(wxFRAME_SHAPED);
    XRC_ADD_STYLE(wxDIALOG_EX_CONTEXTHELP);

    AddWindowStyles();
}

wxObject *wxPropertySheetDialogXmlHandler::DoCreateResource()
{
    if (m_class == wxT("propertysheetpage"))
    {
        wxXmlNode *n = GetParamNode(wxT("object"));

        if (!n) n = GetParamNode(wxT("object_ref"));

        if (n)
        {
            wxBookCtrlBase *bookctrl = m_dialog->GetBookCtrl();
            bool old_ins = m_isInside;
            m_isInside = false;
            wxObject *item = CreateResFromNode(n, bookctrl, NULL);
            m_isInside = old_ins;
            wxWindow *wnd = wxDynamicCast(item, wxWindow);

            if (wnd)
            {
                bookctrl->AddPage(wnd, GetText(wxT("label")), GetBool(wxT("selected")));
                if (HasParam(wxT("bitmap")))
                {
                    wxBitmap bmp = GetBitmap(wxT("bitmap"), wxART_OTHER);
                    wxImageList *imgList = bookctrl->GetImageList();
                    if (imgList == NULL)
                    {
                        imgList = new wxImageList(bmp.GetWidth(), bmp.GetHeight());
                        bookctrl->AssignImageList(imgList);
                    }
                    int imgIndex = imgList->Add(bmp);
                    bookctrl->SetPageImage(bookctrl->GetPageCount()-1, imgIndex);
                }
            }
            else
                wxLogError(wxT("Error in resource."));
            return wnd;
        }
        else
        {
            wxLogError(wxT("Error in resource: no control within wxPropertySheetDialog's <page> tag."));
            return NULL;
        }
    }

    else
    {
        XRC_MAKE_INSTANCE(dlg, wxPropertySheetDialog)

        dlg->Create(m_parentAsWindow,
                   GetID(),
                   GetText(wxT("title")),
                   GetPosition(),
                   GetSize(),
                   GetStyle(),
                   GetName());

        if (HasParam(wxT("icon"))) dlg->SetIcon(GetIcon(wxT("icon"), wxART_FRAME_ICON));

        SetupWindow(dlg);

        wxPropertySheetDialog *old_par = m_dialog;
        m_dialog = dlg;
        bool old_ins = m_isInside;
        m_isInside = true;
        CreateChildren(m_dialog, true/*only this handler*/);
        m_isInside = old_ins;
        m_dialog = old_par;

        if (GetBool(wxT("centered"), false)) dlg->Centre();
        wxString buttons = GetText(wxT("buttons"));
        if (!buttons.IsEmpty())
        {
            int flags = 0;
            if (buttons.Find(wxT("wxOK"))         != wxNOT_FOUND) flags |= wxOK;
            if (buttons.Find(wxT("wxCANCEL"))     != wxNOT_FOUND) flags |= wxCANCEL;
            if (buttons.Find(wxT("wxYES"))        != wxNOT_FOUND) flags |= wxYES;
            if (buttons.Find(wxT("wxNO"))         != wxNOT_FOUND) flags |= wxNO;
            if (buttons.Find(wxT("wxHELP"))       != wxNOT_FOUND) flags |= wxHELP;
            if (buttons.Find(wxT("wxNO_DEFAULT")) != wxNOT_FOUND) flags |= wxNO_DEFAULT;
            dlg->CreateButtons(flags);
        }

        return dlg;
    }
}

bool wxPropertySheetDialogXmlHandler::CanHandle(wxXmlNode *node)
{
    return ((!m_isInside && IsOfClass(node, wxT("wxPropertySheetDialog"))) ||
            (m_isInside && IsOfClass(node, wxT("propertysheetpage"))));
}

#endif // wxUSE_XRC && wxUSE_BOOKCTRL
