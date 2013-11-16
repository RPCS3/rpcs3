/////////////////////////////////////////////////////////////////////////////
// Name:        src/html/m_layout.cpp
// Purpose:     wxHtml module for basic paragraphs/layout handling
// Author:      Vaclav Slavik
// RCS-ID:      $Id: m_layout.cpp 55881 2008-09-25 17:19:30Z VS $
// Copyright:   (c) 1999 Vaclav Slavik
// Licence:     wxWindows licence
/////////////////////////////////////////////////////////////////////////////

#include "wx/wxprec.h"

#ifdef __BORLANDC__
    #pragma hdrstop
#endif

#if wxUSE_HTML && wxUSE_STREAMS

#ifndef WXPRECOMP
    #include "wx/image.h"
#endif

#include "wx/html/forcelnk.h"
#include "wx/html/m_templ.h"

#include "wx/html/htmlwin.h"

FORCE_LINK_ME(m_layout)

#ifdef __WXWINCE__
    #include "wx/msw/wince/missing.h"       // for bsearch()
#else
    #include <stdlib.h>                     // bsearch()
#endif

//-----------------------------------------------------------------------------
// wxHtmlPageBreakCell
//-----------------------------------------------------------------------------

// Since html isn't a page-layout language, it doesn't support page
// page breaks directly--that requires CSS2 support. But a page-break
// facility is handy, and has been requested more than once on the
// mailing lists. This wxHtml tag handler implements just enough of
// CSS2 to support a page break by recognizing only
//   <div style="page-break-before:always">
//
// wxHtml maintains page breaks in wxHtmlPrintout::m_PageBreaks. The
// tag handler below adds appropriate offsets to that array member.
// wxHtmlDCRenderer::Render() accesses that array and makes a new page
// begin after each page-break tag.

// The page-break handler does all its work in AdjustPagebreak(). For
// all tag handlers, that function adjusts the page-break position.
// For other tags, it determines whether the html element can fit on
// the remainder of the page; if it cannot fit, but must not be split,
// then the function moves the page break provided in the argument up,
// and returns 'true' to inform the caller that the argument was
// modified.
//
// Due to its special purpose, the page-break facility differs from
// other tags. It takes up no space, but it behaves as though there is
// never enough room to fit it on the remainder of the page--it always
// forces a page break. Therefore, unlike other elements that trigger
// a page break, it would never 'fit' on the following page either.
// Therefore it's necessary to compare each pagebreak candidate to the
// array wxHtmlPrintout::m_PageBreaks of pagebreaks already set, and
// set a new one only if it's not in that array.

class wxHtmlPageBreakCell : public wxHtmlCell
{
public:
    wxHtmlPageBreakCell() {}

    bool AdjustPagebreak(int* pagebreak,
                         wxArrayInt& known_pagebreaks) const;

    void Draw(wxDC& WXUNUSED(dc),
              int WXUNUSED(x), int WXUNUSED(y),
              int WXUNUSED(view_y1), int WXUNUSED(view_y2),
              wxHtmlRenderingInfo& WXUNUSED(info)) {}

private:
    DECLARE_NO_COPY_CLASS(wxHtmlPageBreakCell)
};

// Comparison routine for bsearch into an int* array of pagebreaks.
extern "C" int wxCMPFUNC_CONV wxInteger_compare(void const* i0, void const* i1)
{
    return *(int*)i0 - *(int*)i1;
}

bool wxHtmlPageBreakCell::AdjustPagebreak(int* pagebreak, wxArrayInt& known_pagebreaks) const
{
    // When we are counting pages, 'known_pagebreaks' is non-NULL.
    // That's the only time we change 'pagebreak'. Otherwise, pages
    // were already counted, 'known_pagebreaks' is NULL, and we don't
    // do anything except return false.
    //
    // We also simply return false if the 'pagebreak' argument is
    // less than (vertically above) or the same as the current
    // vertical position. Otherwise we'd be setting a pagebreak above
    // the current cell, which is incorrect, or duplicating a
    // pagebreak that has already been set.
    if( known_pagebreaks.Count() == 0 || *pagebreak <= m_PosY)
    {
        return false;
    }

    // m_PosY is only the vertical offset from the parent. The pagebreak
    // required here is the total page offset, so m_PosY must be added
    // to the parent's offset and height.
    int total_height = m_PosY;
    for ( wxHtmlCell *parent = GetParent(); parent; parent = parent->GetParent() )
    {
        total_height += parent->GetPosY();
    }


    // Search the array of pagebreaks to see whether we've already set
    // a pagebreak here. The standard bsearch() function is appropriate
    // because the array of pagebreaks through known_pagebreaks[number_of_pages]
    // is known to be sorted in strictly increasing order. '1 + number_of_pages'
    // is used as a bsearch() argument because the array contains a leading
    // zero plus one element for each page.
    int where = known_pagebreaks.Index( total_height);
    // Add a pagebreak only if there isn't one already set here.
    if( wxNOT_FOUND != where)
    {
        return false;
    }
    else
    {
        *pagebreak = m_PosY;
        return true;
    }
}



TAG_HANDLER_BEGIN(P, "P")
    TAG_HANDLER_CONSTR(P) { }

    TAG_HANDLER_PROC(tag)
    {
        if (m_WParser->GetContainer()->GetFirstChild() != NULL)
        {
            m_WParser->CloseContainer();
            m_WParser->OpenContainer();
        }
        m_WParser->GetContainer()->SetIndent(m_WParser->GetCharHeight(), wxHTML_INDENT_TOP);
        m_WParser->GetContainer()->SetAlign(tag);
        return false;
    }

TAG_HANDLER_END(P)



TAG_HANDLER_BEGIN(BR, "BR")
    TAG_HANDLER_CONSTR(BR) { }

    TAG_HANDLER_PROC(tag)
    {
        int al = m_WParser->GetContainer()->GetAlignHor();
        wxHtmlContainerCell *c;

        m_WParser->CloseContainer();
        c = m_WParser->OpenContainer();
        c->SetAlignHor(al);
        c->SetAlign(tag);
        c->SetMinHeight(m_WParser->GetCharHeight());
        return false;
    }

TAG_HANDLER_END(BR)



TAG_HANDLER_BEGIN(CENTER, "CENTER")
    TAG_HANDLER_CONSTR(CENTER) { }

    TAG_HANDLER_PROC(tag)
    {
        int old = m_WParser->GetAlign();
        wxHtmlContainerCell *c = m_WParser->GetContainer();

        m_WParser->SetAlign(wxHTML_ALIGN_CENTER);
        if (c->GetFirstChild() != NULL)
        {
            m_WParser->CloseContainer();
            m_WParser->OpenContainer();
        }
        else
            c->SetAlignHor(wxHTML_ALIGN_CENTER);

        if (tag.HasEnding())
        {
            ParseInner(tag);

            m_WParser->SetAlign(old);
            if (c->GetFirstChild() != NULL)
            {
                m_WParser->CloseContainer();
                m_WParser->OpenContainer();
            }
            else
                c->SetAlignHor(old);

            return true;
        }
        else return false;
    }

TAG_HANDLER_END(CENTER)



TAG_HANDLER_BEGIN(DIV, "DIV")
    TAG_HANDLER_CONSTR(DIV) { }

    TAG_HANDLER_PROC(tag)
    {
        if(tag.HasParam(wxT("STYLE")))
        {
            if(tag.GetParam(wxT("STYLE")).IsSameAs(wxT("PAGE-BREAK-BEFORE:ALWAYS"), false))
            {
                m_WParser->CloseContainer();
                m_WParser->OpenContainer()->InsertCell(new wxHtmlPageBreakCell);
                m_WParser->CloseContainer();
                m_WParser->OpenContainer();
                return false;
            }
            else
            {
                // Treat other STYLE parameters here when they're supported.
                return false;
            }
        }
        else if(tag.HasParam(wxT("ALIGN")))
        {
            int old = m_WParser->GetAlign();
            wxHtmlContainerCell *c = m_WParser->GetContainer();
            if (c->GetFirstChild() != NULL)
            {
                m_WParser->CloseContainer();
                m_WParser->OpenContainer();
                c = m_WParser->GetContainer();
                c->SetAlign(tag);
                m_WParser->SetAlign(c->GetAlignHor());
            }
            else
            {
                c->SetAlign(tag);
                m_WParser->SetAlign(c->GetAlignHor());
            }

            ParseInner(tag);

            m_WParser->SetAlign(old);
            if (c->GetFirstChild() != NULL)
            {
                m_WParser->CloseContainer();
                m_WParser->OpenContainer();
            }
            else
                c->SetAlignHor(old);

            return true;
        }
        else
        {
            // Same as BR
            int al = m_WParser->GetContainer()->GetAlignHor();
            wxHtmlContainerCell *c;

            m_WParser->CloseContainer();
            c = m_WParser->OpenContainer();
            c->SetAlignHor(al);
            c->SetAlign(tag);
            c->SetMinHeight(m_WParser->GetCharHeight());
            return false;
        }
    }

TAG_HANDLER_END(DIV)




TAG_HANDLER_BEGIN(TITLE, "TITLE")
    TAG_HANDLER_CONSTR(TITLE) { }

    TAG_HANDLER_PROC(tag)
    {
        wxHtmlWindowInterface *winIface = m_WParser->GetWindowInterface();
        if (winIface)
        {
            wxString title = m_WParser->GetSource()->Mid(
                                    tag.GetBeginPos(),
                                    tag.GetEndPos1()-tag.GetBeginPos());
#if !wxUSE_UNICODE && wxUSE_WCHAR_T
            const wxFontEncoding enc = m_WParser->GetInputEncoding();
            if ( enc != wxFONTENCODING_DEFAULT )
            {
                // need to convert to the current one
                title = wxString(title.wc_str(wxCSConv(enc)), wxConvLocal);
            }
#endif // !wxUSE_UNICODE

            title = m_WParser->GetEntitiesParser()->Parse(title);

            winIface->SetHTMLWindowTitle(title);
        }
        return true;
    }

TAG_HANDLER_END(TITLE)




TAG_HANDLER_BEGIN(BODY, "BODY")
    TAG_HANDLER_CONSTR(BODY) { }

    TAG_HANDLER_PROC(tag)
    {
        wxColour clr;

        if (tag.GetParamAsColour(wxT("TEXT"), &clr))
        {
            m_WParser->SetActualColor(clr);
            m_WParser->GetContainer()->InsertCell(new wxHtmlColourCell(clr));
        }

        if (tag.GetParamAsColour(wxT("LINK"), &clr))
            m_WParser->SetLinkColor(clr);

        wxHtmlWindowInterface *winIface = m_WParser->GetWindowInterface();
        // the rest of this function requires a window:
        if ( !winIface )
            return false;

        if (tag.HasParam(wxT("BACKGROUND")))
        {
            wxFSFile *fileBgImage = m_WParser->OpenURL
                                               (
                                                wxHTML_URL_IMAGE,
                                                tag.GetParam(wxT("BACKGROUND"))
                                               );
            if ( fileBgImage )
            {
                wxInputStream *is = fileBgImage->GetStream();
                if ( is )
                {
#if !defined(__WXMSW__) || wxUSE_WXDIB
                    wxImage image(*is);
                    if ( image.Ok() )
                        winIface->SetHTMLBackgroundImage(image);
#endif
                }

                delete fileBgImage;
            }
        }

        if (tag.GetParamAsColour(wxT("BGCOLOR"), &clr))
        {
            m_WParser->GetContainer()->InsertCell(
                new wxHtmlColourCell(clr, wxHTML_CLR_BACKGROUND));
            winIface->SetHTMLBackgroundColour(clr);
        }

        return false;
    }

TAG_HANDLER_END(BODY)



TAG_HANDLER_BEGIN(BLOCKQUOTE, "BLOCKQUOTE")
    TAG_HANDLER_CONSTR(BLOCKQUOTE) { }

    TAG_HANDLER_PROC(tag)
    {
        wxHtmlContainerCell *c;

        m_WParser->CloseContainer();
        c = m_WParser->OpenContainer();

        if (c->GetAlignHor() == wxHTML_ALIGN_RIGHT)
            c->SetIndent(5 * m_WParser->GetCharWidth(), wxHTML_INDENT_RIGHT);
        else
            c->SetIndent(5 * m_WParser->GetCharWidth(), wxHTML_INDENT_LEFT);

        c->SetIndent(m_WParser->GetCharHeight(), wxHTML_INDENT_TOP);
        m_WParser->OpenContainer();
        ParseInner(tag);
        c = m_WParser->CloseContainer();
        c->SetIndent(m_WParser->GetCharHeight(), wxHTML_INDENT_BOTTOM);
        m_WParser->CloseContainer();
        m_WParser->OpenContainer();
        return true;
    }

TAG_HANDLER_END(BLOCKQUOTE)



TAG_HANDLER_BEGIN(SUBSUP, "SUB,SUP")

    TAG_HANDLER_PROC(tag)
    {
        bool issub = (tag.GetName() == wxT("SUB"));
        wxHtmlScriptMode oldmode = m_WParser->GetScriptMode();
        int oldbase = m_WParser->GetScriptBaseline();
        int oldsize = m_WParser->GetFontSize();

        wxHtmlContainerCell *cont = m_WParser->GetContainer();
        wxHtmlCell *c = cont->GetLastChild();

        m_WParser->SetScriptMode(issub ? wxHTML_SCRIPT_SUB : wxHTML_SCRIPT_SUP);
        m_WParser->SetScriptBaseline(
                oldbase + c ? c->GetScriptBaseline() : 0);

        // select smaller font
        m_WParser->SetFontSize(m_WParser->GetFontSize()-2);
        cont->InsertCell(new wxHtmlFontCell(m_WParser->CreateCurrentFont()));

        ParseInner(tag);

        // restore font size
        m_WParser->SetFontSize(oldsize);
        m_WParser->GetContainer()->InsertCell(
            new wxHtmlFontCell(m_WParser->CreateCurrentFont()));

        // restore base and alignment
        m_WParser->SetScriptBaseline(oldbase);
        m_WParser->SetScriptMode(oldmode);

        return true;
    }

TAG_HANDLER_END(SUBSUP)


// Tag handler for tags that we have to ignore, otherwise non-text data
// would show up as text:
TAG_HANDLER_BEGIN(DoNothing, "SCRIPT")
    TAG_HANDLER_CONSTR(DoNothing) { }

    TAG_HANDLER_PROC(WXUNUSED(tag))
    {
        return true;
    }
TAG_HANDLER_END(DoNothing)





TAGS_MODULE_BEGIN(Layout)

    TAGS_MODULE_ADD(P)
    TAGS_MODULE_ADD(BR)
    TAGS_MODULE_ADD(CENTER)
    TAGS_MODULE_ADD(DIV)
    TAGS_MODULE_ADD(TITLE)
    TAGS_MODULE_ADD(BODY)
    TAGS_MODULE_ADD(BLOCKQUOTE)
    TAGS_MODULE_ADD(SUBSUP)
    TAGS_MODULE_ADD(DoNothing)

TAGS_MODULE_END(Layout)

#endif
