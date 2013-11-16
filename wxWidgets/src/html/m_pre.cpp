/////////////////////////////////////////////////////////////////////////////
// Name:        src/html/m_pre.cpp
// Purpose:     wxHtml module for <PRE> ... </PRE> tag (code citation)
// Author:      Vaclav Slavik
// RCS-ID:      $Id: m_pre.cpp 56547 2008-10-28 10:06:32Z VS $
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

#include "wx/html/htmlcell.h"
#include "wx/tokenzr.h"
#include "wx/encconv.h"

FORCE_LINK_ME(m_pre)

// replaces '\t', ' ' and '\n' with HTML markup:
static wxString LINKAGEMODE HtmlizeLinebreaks(const wxString& str)
{
    wxString out;
    out.reserve(str.length()); // we'll certainly need at least that

    size_t len = str.Len();

    for (size_t i = 0; i < len; i++)
    {
        switch (str[i])
        {
            case wxT('<'):
                while (i < len && str[i] != wxT('>'))
                {
                    out << str[i++];
                }
                out << wxT('>');
                break;

            // We need to translate any line break into exactly one <br>.
            // Quoting HTML spec: "A line break is defined to be a carriage
            // return (&#x000D;), a line feed (&#x000A;), or a carriage
            // return/line feed pair."
            case wxT('\r'):
                {
                    size_t j = i + 1;
                    if ( j < len && str[j] == wxT('\n') )
                        i = j;
                }
                // fall through
            case wxT('\n'):
                out << wxT("<br>");
                break;

            default:
                out << str[i];
                break;
        }
    }
    return out;
}


//-----------------------------------------------------------------------------
// The list handler:
//-----------------------------------------------------------------------------


TAG_HANDLER_BEGIN(PRE, "PRE")
    TAG_HANDLER_CONSTR(PRE) { }

    TAG_HANDLER_PROC(tag)
    {
        wxHtmlContainerCell *c;

        const int fixed = m_WParser->GetFontFixed();
        const int italic = m_WParser->GetFontItalic();
        const int underlined = m_WParser->GetFontUnderlined();
        const int bold = m_WParser->GetFontBold();
        const int fsize = m_WParser->GetFontSize();
        const wxHtmlWinParser::WhitespaceMode whitespace =
            m_WParser->GetWhitespaceMode();

        c = m_WParser->GetContainer();
        m_WParser->SetWhitespaceMode(wxHtmlWinParser::Whitespace_Pre);
        m_WParser->SetFontUnderlined(false);
        m_WParser->SetFontBold(false);
        m_WParser->SetFontItalic(false);
        m_WParser->SetFontFixed(true);
        m_WParser->SetFontSize(3);
        c->InsertCell(new wxHtmlFontCell(m_WParser->CreateCurrentFont()));

        m_WParser->CloseContainer();
        c = m_WParser->OpenContainer();
        c->SetWidthFloat(tag);
        c = m_WParser->OpenContainer();
        c->SetAlignHor(wxHTML_ALIGN_LEFT);
        c->SetIndent(m_WParser->GetCharHeight(), wxHTML_INDENT_TOP);

        wxString srcMid = m_WParser->GetInnerSource(tag);

        // setting Whitespace_Pre mode takes care of spaces and TABs, but
        // not linebreaks, so we have to translate them into <br> by
        // calling HtmlizeLinebreaks() here
        ParseInnerSource(HtmlizeLinebreaks(srcMid));

        m_WParser->CloseContainer();
        m_WParser->CloseContainer();
        c = m_WParser->OpenContainer();

        m_WParser->SetWhitespaceMode(whitespace);
        m_WParser->SetFontUnderlined(underlined);
        m_WParser->SetFontBold(bold);
        m_WParser->SetFontItalic(italic);
        m_WParser->SetFontFixed(fixed);
        m_WParser->SetFontSize(fsize);
        c->InsertCell(new wxHtmlFontCell(m_WParser->CreateCurrentFont()));

        return true;
    }

TAG_HANDLER_END(PRE)





TAGS_MODULE_BEGIN(Pre)

    TAGS_MODULE_ADD(PRE)

TAGS_MODULE_END(Pre)

#endif
