/////////////////////////////////////////////////////////////////////////////
// Name:        src/html/m_span.cpp
// Purpose:     wxHtml module for span handling
// Author:      Nigel Paton
// Copyright:   wxWidgets team
// Licence:     wxWindows licence
/////////////////////////////////////////////////////////////////////////////

#include "wx/wxprec.h"

#ifdef __BORLANDC__
    #pragma hdrstop
#endif

#if wxUSE_HTML

#include "wx/html/forcelnk.h"
#include "wx/html/m_templ.h"
#include "wx/fontenum.h"
#include "wx/tokenzr.h"
#include "wx/html/styleparams.h"

FORCE_LINK_ME(m_span)


TAG_HANDLER_BEGIN(SPAN, "SPAN" )

    TAG_HANDLER_VARS
        wxArrayString m_Faces;

    TAG_HANDLER_CONSTR(SPAN) { }

    TAG_HANDLER_PROC(tag)
    {
        wxColour oldclr = m_WParser->GetActualColor();
        wxColour oldbackclr = m_WParser->GetActualBackgroundColor();
        int oldbackmode = m_WParser->GetActualBackgroundMode();
        int oldsize = m_WParser->GetFontSize();
        int oldbold = m_WParser->GetFontBold();
        int olditalic = m_WParser->GetFontItalic();
        int oldunderlined = m_WParser->GetFontUnderlined();
        wxString oldfontface = m_WParser->GetFontFace();

        // Load any style parameters
        wxHtmlStyleParams styleParams(tag);

        ApplyStyle(styleParams);

        ParseInner(tag);

        m_WParser->SetFontSize(oldsize);
        m_WParser->SetFontBold(oldbold);
        m_WParser->SetFontUnderlined(oldunderlined);
        m_WParser->SetFontFace(oldfontface);
        m_WParser->SetFontItalic(olditalic);
        m_WParser->GetContainer()->InsertCell(
                new wxHtmlFontCell(m_WParser->CreateCurrentFont()));

        if (oldclr != m_WParser->GetActualColor())
        {
            m_WParser->SetActualColor(oldclr);
            m_WParser->GetContainer()->InsertCell(
                new wxHtmlColourCell(oldclr));
        }

        if (oldbackmode != m_WParser->GetActualBackgroundMode() ||
            oldbackclr != m_WParser->GetActualBackgroundColor())
        {
            m_WParser->SetActualBackgroundMode(oldbackmode);
            m_WParser->SetActualBackgroundColor(oldbackclr);
            m_WParser->GetContainer()->InsertCell(
                new wxHtmlColourCell(oldbackclr, oldbackmode == wxTRANSPARENT ? wxHTML_CLR_TRANSPARENT_BACKGROUND : wxHTML_CLR_BACKGROUND));
        }

        return true;
    }

TAG_HANDLER_END(SPAN)


TAGS_MODULE_BEGIN(Spans)

    TAGS_MODULE_ADD(SPAN)

TAGS_MODULE_END(Spans)

#endif // wxUSE_HTML
