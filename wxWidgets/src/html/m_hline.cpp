/////////////////////////////////////////////////////////////////////////////
// Name:        src/html/m_hline.cpp
// Purpose:     wxHtml module for horizontal line (HR tag)
// Author:      Vaclav Slavik
// RCS-ID:      $Id: m_hline.cpp 38788 2006-04-18 08:11:26Z ABX $
// Copyright:   (c) 1999 Vaclav Slavik
// Licence:     wxWindows licence
/////////////////////////////////////////////////////////////////////////////

#include "wx/wxprec.h"

#ifdef __BORLANDC__
    #pragma hdrstop
#endif

#if wxUSE_HTML && wxUSE_STREAMS

#ifndef WXPRECOMP
    #include "wx/brush.h"
    #include "wx/pen.h"
    #include "wx/dc.h"
#endif

#include "wx/html/forcelnk.h"
#include "wx/html/m_templ.h"

#include "wx/html/htmlcell.h"

FORCE_LINK_ME(m_hline)


//-----------------------------------------------------------------------------
// wxHtmlLineCell
//-----------------------------------------------------------------------------

class wxHtmlLineCell : public wxHtmlCell
{
    public:
        wxHtmlLineCell(int size, bool shading) : wxHtmlCell() {m_Height = size; m_HasShading = shading;}
        void Draw(wxDC& dc, int x, int y, int view_y1, int view_y2,
                  wxHtmlRenderingInfo& info);
        void Layout(int w)
            { m_Width = w; wxHtmlCell::Layout(w); }

    private:
        // Should we draw 3-D shading or not
      bool m_HasShading;

      DECLARE_NO_COPY_CLASS(wxHtmlLineCell)
};


void wxHtmlLineCell::Draw(wxDC& dc, int x, int y,
                          int WXUNUSED(view_y1), int WXUNUSED(view_y2),
                          wxHtmlRenderingInfo& WXUNUSED(info))
{
    wxBrush mybrush(wxT("GREY"), (m_HasShading) ? wxTRANSPARENT : wxSOLID);
    wxPen mypen(wxT("GREY"), 1, wxSOLID);
    dc.SetBrush(mybrush);
    dc.SetPen(mypen);
    dc.DrawRectangle(x + m_PosX, y + m_PosY, m_Width, m_Height);
}




//-----------------------------------------------------------------------------
// The list handler:
//-----------------------------------------------------------------------------


TAG_HANDLER_BEGIN(HR, "HR")
    TAG_HANDLER_CONSTR(HR) { }

    TAG_HANDLER_PROC(tag)
    {
        wxHtmlContainerCell *c;
        int sz;
        bool HasShading;

        m_WParser->CloseContainer();
        c = m_WParser->OpenContainer();

        c->SetIndent(m_WParser->GetCharHeight(), wxHTML_INDENT_VERTICAL);
        c->SetAlignHor(wxHTML_ALIGN_CENTER);
        c->SetAlign(tag);
        c->SetWidthFloat(tag);
        sz = 1;
        tag.GetParamAsInt(wxT("SIZE"), &sz);
        HasShading = !(tag.HasParam(wxT("NOSHADE")));
        c->InsertCell(new wxHtmlLineCell((int)((double)sz * m_WParser->GetPixelScale()), HasShading));

        m_WParser->CloseContainer();
        m_WParser->OpenContainer();

        return false;
    }

TAG_HANDLER_END(HR)





TAGS_MODULE_BEGIN(HLine)

    TAGS_MODULE_ADD(HR)

TAGS_MODULE_END(HLine)

#endif
