/////////////////////////////////////////////////////////////////////////////
// Name:        src/html/m_links.cpp
// Purpose:     wxHtml module for links & anchors
// Author:      Vaclav Slavik
// RCS-ID:      $Id: m_links.cpp 38788 2006-04-18 08:11:26Z ABX $
// Copyright:   (c) 1999 Vaclav Slavik
// Licence:     wxWindows licence
/////////////////////////////////////////////////////////////////////////////

#include "wx/wxprec.h"

#ifdef __BORLANDC__
    #pragma hdrstop
#endif

#if wxUSE_HTML && wxUSE_STREAMS

#ifndef WXPRECOMP
#endif

#include "wx/html/forcelnk.h"
#include "wx/html/m_templ.h"


FORCE_LINK_ME(m_links)


class wxHtmlAnchorCell : public wxHtmlCell
{
private:
    wxString m_AnchorName;

public:
    wxHtmlAnchorCell(const wxString& name) : wxHtmlCell()
        { m_AnchorName = name; }
    void Draw(wxDC& WXUNUSED(dc),
              int WXUNUSED(x), int WXUNUSED(y),
              int WXUNUSED(view_y1), int WXUNUSED(view_y2),
              wxHtmlRenderingInfo& WXUNUSED(info)) {}

    virtual const wxHtmlCell* Find(int condition, const void* param) const
    {
        if ((condition == wxHTML_COND_ISANCHOR) &&
            (m_AnchorName == (*((const wxString*)param))))
        {
            return this;
        }
        else
        {
            return wxHtmlCell::Find(condition, param);
        }
    }

    DECLARE_NO_COPY_CLASS(wxHtmlAnchorCell)
};



TAG_HANDLER_BEGIN(A, "A")
    TAG_HANDLER_CONSTR(A) { }

    TAG_HANDLER_PROC(tag)
    {
        if (tag.HasParam( wxT("NAME") ))
        {
            m_WParser->GetContainer()->InsertCell(new wxHtmlAnchorCell(tag.GetParam( wxT("NAME") )));
        }

        if (tag.HasParam( wxT("HREF") ))
        {
            wxHtmlLinkInfo oldlnk = m_WParser->GetLink();
            wxColour oldclr = m_WParser->GetActualColor();
            int oldund = m_WParser->GetFontUnderlined();
            wxString name(tag.GetParam( wxT("HREF") )), target;

            if (tag.HasParam( wxT("TARGET") )) target = tag.GetParam( wxT("TARGET") );
            m_WParser->SetActualColor(m_WParser->GetLinkColor());
            m_WParser->GetContainer()->InsertCell(new wxHtmlColourCell(m_WParser->GetLinkColor()));
            m_WParser->SetFontUnderlined(true);
            m_WParser->GetContainer()->InsertCell(new wxHtmlFontCell(m_WParser->CreateCurrentFont()));
            m_WParser->SetLink(wxHtmlLinkInfo(name, target));

            ParseInner(tag);

            m_WParser->SetLink(oldlnk);
            m_WParser->SetFontUnderlined(oldund);
            m_WParser->GetContainer()->InsertCell(new wxHtmlFontCell(m_WParser->CreateCurrentFont()));
            m_WParser->SetActualColor(oldclr);
            m_WParser->GetContainer()->InsertCell(new wxHtmlColourCell(oldclr));

            return true;
        }
        else return false;
    }

TAG_HANDLER_END(A)



TAGS_MODULE_BEGIN(Links)

    TAGS_MODULE_ADD(A)

TAGS_MODULE_END(Links)


#endif
