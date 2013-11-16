/////////////////////////////////////////////////////////////////////////////
// Name:        src/html/htmlcell.cpp
// Purpose:     wxHtmlCell - basic element of HTML output
// Author:      Vaclav Slavik
// RCS-ID:      $Id: htmlcell.cpp 53318 2008-04-23 11:54:05Z VS $
// Copyright:   (c) 1999 Vaclav Slavik
// Licence:     wxWindows licence
/////////////////////////////////////////////////////////////////////////////

#include "wx/wxprec.h"

#ifdef __BORLANDC__
    #pragma hdrstop
#endif

#if wxUSE_HTML && wxUSE_STREAMS

#ifndef WXPRECOMP
    #include "wx/dynarray.h"
    #include "wx/brush.h"
    #include "wx/colour.h"
    #include "wx/dc.h"
    #include "wx/settings.h"
    #include "wx/module.h"
#endif

#include "wx/html/htmlcell.h"
#include "wx/html/htmlwin.h"

#include <stdlib.h>

//-----------------------------------------------------------------------------
// Helper classes
//-----------------------------------------------------------------------------

void wxHtmlSelection::Set(const wxPoint& fromPos, const wxHtmlCell *fromCell,
                          const wxPoint& toPos, const wxHtmlCell *toCell)
{
    m_fromCell = fromCell;
    m_toCell = toCell;
    m_fromPos = fromPos;
    m_toPos = toPos;
}

void wxHtmlSelection::Set(const wxHtmlCell *fromCell, const wxHtmlCell *toCell)
{
    wxPoint p1 = fromCell ? fromCell->GetAbsPos() : wxDefaultPosition;
    wxPoint p2 = toCell ? toCell->GetAbsPos() : wxDefaultPosition;
    if ( toCell )
    {
        p2.x += toCell->GetWidth();
        p2.y += toCell->GetHeight();
    }
    Set(p1, fromCell, p2, toCell);
}

wxColour
wxDefaultHtmlRenderingStyle::
GetSelectedTextColour(const wxColour& WXUNUSED(clr))
{
    return wxSystemSettings::GetColour(wxSYS_COLOUR_HIGHLIGHTTEXT);
}

wxColour
wxDefaultHtmlRenderingStyle::
GetSelectedTextBgColour(const wxColour& WXUNUSED(clr))
{
    return wxSystemSettings::GetColour(wxSYS_COLOUR_HIGHLIGHT);
}


//-----------------------------------------------------------------------------
// wxHtmlCell
//-----------------------------------------------------------------------------

IMPLEMENT_ABSTRACT_CLASS(wxHtmlCell, wxObject)

wxHtmlCell::wxHtmlCell() : wxObject()
{
    m_Next = NULL;
    m_Parent = NULL;
    m_Width = m_Height = m_Descent = 0;
    m_ScriptMode = wxHTML_SCRIPT_NORMAL;        // <sub> or <sup> mode
    m_ScriptBaseline = 0;                       // <sub> or <sup> baseline
    m_CanLiveOnPagebreak = true;
    m_Link = NULL;
}

wxHtmlCell::~wxHtmlCell()
{
    delete m_Link;
}

// Update the descent value when whe are in a <sub> or <sup>.
// prevbase is the parent base
void wxHtmlCell::SetScriptMode(wxHtmlScriptMode mode, long previousBase)
{
    m_ScriptMode = mode;

    if (mode == wxHTML_SCRIPT_SUP)
        m_ScriptBaseline = previousBase - (m_Height + 1) / 2;
    else if (mode == wxHTML_SCRIPT_SUB)
        m_ScriptBaseline = previousBase + (m_Height + 1) / 6;
    else
        m_ScriptBaseline = 0;

    m_Descent += m_ScriptBaseline;
}

#if WXWIN_COMPATIBILITY_2_6

struct wxHtmlCellOnMouseClickCompatHelper;

static wxHtmlCellOnMouseClickCompatHelper *gs_helperOnMouseClick = NULL;

// helper for routing calls to new ProcessMouseClick() method to deprecated
// OnMouseClick() method
struct wxHtmlCellOnMouseClickCompatHelper
{
    wxHtmlCellOnMouseClickCompatHelper(wxHtmlWindowInterface *window_,
                                       const wxPoint& pos_,
                                       const wxMouseEvent& event_)
        : window(window_), pos(pos_), event(event_), retval(false)
    {
    }

    bool CallOnMouseClick(wxHtmlCell *cell)
    {
        wxHtmlCellOnMouseClickCompatHelper *oldHelper = gs_helperOnMouseClick;
        gs_helperOnMouseClick = this;
        cell->OnMouseClick
              (
                window ? window->GetHTMLWindow() : NULL,
                pos.x, pos.y,
                event
              );
        gs_helperOnMouseClick = oldHelper;
        return retval;
    }

    wxHtmlWindowInterface *window;
    const wxPoint& pos;
    const wxMouseEvent& event;
    bool retval;
};
#endif // WXWIN_COMPATIBILITY_2_6

bool wxHtmlCell::ProcessMouseClick(wxHtmlWindowInterface *window,
                                   const wxPoint& pos,
                                   const wxMouseEvent& event)
{
    wxCHECK_MSG( window, false, _T("window interface must be provided") );

#if WXWIN_COMPATIBILITY_2_6
    // NB: this hack puts the body of ProcessMouseClick() into OnMouseClick()
    //     (for which it has to pass the arguments and return value via a
    //     helper variable because these two methods have different
    //     signatures), so that old code overriding OnMouseClick will continue
    //     to work
    wxHtmlCellOnMouseClickCompatHelper compat(window, pos, event);
    return compat.CallOnMouseClick(this);
}

void wxHtmlCell::OnMouseClick(wxWindow *, int, int, const wxMouseEvent& event)
{
    wxCHECK_RET( gs_helperOnMouseClick, _T("unexpected call to OnMouseClick") );
    wxHtmlWindowInterface *window = gs_helperOnMouseClick->window;
    const wxPoint& pos = gs_helperOnMouseClick->pos;
#endif // WXWIN_COMPATIBILITY_2_6

    wxHtmlLinkInfo *lnk = GetLink(pos.x, pos.y);
    bool retval = false;

    if (lnk)
    {
        wxHtmlLinkInfo lnk2(*lnk);
        lnk2.SetEvent(&event);
        lnk2.SetHtmlCell(this);

        window->OnHTMLLinkClicked(lnk2);
        retval = true;
    }

#if WXWIN_COMPATIBILITY_2_6
    gs_helperOnMouseClick->retval = retval;
#else
    return retval;
#endif // WXWIN_COMPATIBILITY_2_6
}

#if WXWIN_COMPATIBILITY_2_6
wxCursor wxHtmlCell::GetCursor() const
{
    return wxNullCursor;
}
#endif // WXWIN_COMPATIBILITY_2_6

wxCursor wxHtmlCell::GetMouseCursor(wxHtmlWindowInterface *window) const
{
#if WXWIN_COMPATIBILITY_2_6
    // NB: Older versions of wx used GetCursor() virtual method in place of
    //     GetMouseCursor(interface). This code ensures that user code that
    //     overriden GetCursor() continues to work. The trick is that the base
    //     wxHtmlCell::GetCursor() method simply returns wxNullCursor, so we
    //     know that GetCursor() was overriden iff it returns valid cursor.
    wxCursor cur = GetCursor();
    if (cur.Ok())
        return cur;
#endif // WXWIN_COMPATIBILITY_2_6

    if ( GetLink() )
    {
        return window->GetHTMLCursor(wxHtmlWindowInterface::HTMLCursor_Link);
    }
    else
    {
        return window->GetHTMLCursor(wxHtmlWindowInterface::HTMLCursor_Default);
    }
}


bool wxHtmlCell::AdjustPagebreak(int *pagebreak,
                                 wxArrayInt& WXUNUSED(known_pagebreaks)) const
{
    if ((!m_CanLiveOnPagebreak) &&
                m_PosY < *pagebreak && m_PosY + m_Height > *pagebreak)
    {
        *pagebreak = m_PosY;
        return true;
    }

    return false;
}



void wxHtmlCell::SetLink(const wxHtmlLinkInfo& link)
{
    if (m_Link) delete m_Link;
    m_Link = NULL;
    if (link.GetHref() != wxEmptyString)
        m_Link = new wxHtmlLinkInfo(link);
}


void wxHtmlCell::Layout(int WXUNUSED(w))
{
    SetPos(0, 0);
}



const wxHtmlCell* wxHtmlCell::Find(int WXUNUSED(condition), const void* WXUNUSED(param)) const
{
    return NULL;
}


wxHtmlCell *wxHtmlCell::FindCellByPos(wxCoord x, wxCoord y,
                                      unsigned flags) const
{
    if ( x >= 0 && x < m_Width && y >= 0 && y < m_Height )
    {
        return wxConstCast(this, wxHtmlCell);
    }
    else
    {
        if ((flags & wxHTML_FIND_NEAREST_AFTER) &&
                (y < 0 || (y < 0+m_Height && x < 0+m_Width)))
            return wxConstCast(this, wxHtmlCell);
        else if ((flags & wxHTML_FIND_NEAREST_BEFORE) &&
                (y >= 0+m_Height || (y >= 0 && x >= 0)))
            return wxConstCast(this, wxHtmlCell);
        else
            return NULL;
    }
}


wxPoint wxHtmlCell::GetAbsPos(wxHtmlCell *rootCell) const
{
    wxPoint p(m_PosX, m_PosY);
    for (wxHtmlCell *parent = m_Parent; parent && parent != rootCell;
         parent = parent->m_Parent)
    {
        p.x += parent->m_PosX;
        p.y += parent->m_PosY;
    }
    return p;
}

wxHtmlCell *wxHtmlCell::GetRootCell() const
{
    wxHtmlCell *c = wxConstCast(this, wxHtmlCell);
    while ( c->m_Parent )
        c = c->m_Parent;
    return c;
}

unsigned wxHtmlCell::GetDepth() const
{
    unsigned d = 0;
    for (wxHtmlCell *p = m_Parent; p; p = p->m_Parent)
        d++;
    return d;
}

bool wxHtmlCell::IsBefore(wxHtmlCell *cell) const
{
    const wxHtmlCell *c1 = this;
    const wxHtmlCell *c2 = cell;
    unsigned d1 = GetDepth();
    unsigned d2 = cell->GetDepth();

    if ( d1 > d2 )
        for (; d1 != d2; d1-- )
            c1 = c1->m_Parent;
    else if ( d1 < d2 )
        for (; d1 != d2; d2-- )
            c2 = c2->m_Parent;

    if ( cell == this )
        return true;

    while ( c1 && c2 )
    {
        if ( c1->m_Parent == c2->m_Parent )
        {
            while ( c1 )
            {
                if ( c1 == c2 )
                    return true;
                c1 = c1->GetNext();
            }
            return false;
        }
        else
        {
            c1 = c1->m_Parent;
            c2 = c2->m_Parent;
        }
    }

    wxFAIL_MSG(_T("Cells are in different trees"));
    return false;
}


//-----------------------------------------------------------------------------
// wxHtmlWordCell
//-----------------------------------------------------------------------------

IMPLEMENT_ABSTRACT_CLASS(wxHtmlWordCell, wxHtmlCell)

wxHtmlWordCell::wxHtmlWordCell(const wxString& word, const wxDC& dc) : wxHtmlCell()
{
    m_Word = word;
    dc.GetTextExtent(m_Word, &m_Width, &m_Height, &m_Descent);
    SetCanLiveOnPagebreak(false);
    m_allowLinebreak = true;
}

void wxHtmlWordCell::SetPreviousWord(wxHtmlWordCell *cell)
{
    if ( cell && m_Parent == cell->m_Parent &&
         !wxIsspace(cell->m_Word.Last()) && !wxIsspace(m_Word[0u]) )
    {
        m_allowLinebreak = false;
    }
}

// Splits m_Word into up to three parts according to selection, returns
// substring before, in and after selection and the points (in relative coords)
// where s2 and s3 start:
void wxHtmlWordCell::Split(const wxDC& dc,
                           const wxPoint& selFrom, const wxPoint& selTo,
                           unsigned& pos1, unsigned& pos2) const
{
    wxPoint pt1 = (selFrom == wxDefaultPosition) ?
                   wxDefaultPosition : selFrom - GetAbsPos();
    wxPoint pt2 = (selTo == wxDefaultPosition) ?
                   wxPoint(m_Width, wxDefaultCoord) : selTo - GetAbsPos();

    // if the selection is entirely within this cell, make sure pt1 < pt2 in
    // order to make the rest of this function simpler:
    if ( selFrom != wxDefaultPosition && selTo != wxDefaultPosition &&
         selFrom.x > selTo.x )
    {
        wxPoint tmp = pt1;
        pt1 = pt2;
        pt2 = tmp;
    }

    unsigned len = m_Word.length();
    unsigned i = 0;
    pos1 = 0;

    // adjust for cases when the start/end position is completely
    // outside the cell:
    if ( pt1.y < 0 )
        pt1.x = 0;
    if ( pt2.y >= m_Height )
        pt2.x = m_Width;

    // before selection:
    // (include character under caret only if in first half of width)
#ifdef __WXMAC__
    // implementation using PartialExtents to support fractional widths
    wxArrayInt widths ;
    dc.GetPartialTextExtents(m_Word,widths) ;
    while( i < len && pt1.x >= widths[i] )
        i++ ;
    if ( i < len )
    {
        int charW = (i > 0) ? widths[i] - widths[i-1] : widths[i];
        if ( widths[i] - pt1.x < charW/2 )
            i++;
    }
#else // !__WXMAC__
    wxCoord charW, charH;
    while ( pt1.x > 0 && i < len )
    {
        dc.GetTextExtent(m_Word[i], &charW, &charH);
        pt1.x -= charW;
        if ( pt1.x >= -charW/2 )
        {
            pos1 += charW;
            i++;
        }
    }
#endif // __WXMAC__/!__WXMAC__

    // in selection:
    // (include character under caret only if in first half of width)
    unsigned j = i;
#ifdef __WXMAC__
    while( j < len && pt2.x >= widths[j] )
        j++ ;
    if ( j < len )
    {
        int charW = (j > 0) ? widths[j] - widths[j-1] : widths[j];
        if ( widths[j] - pt2.x < charW/2 )
            j++;
    }
#else // !__WXMAC__
    pos2 = pos1;
    pt2.x -= pos2;
    while ( pt2.x > 0 && j < len )
    {
        dc.GetTextExtent(m_Word[j], &charW, &charH);
        pt2.x -= charW;
        if ( pt2.x >= -charW/2 )
        {
            pos2 += charW;
            j++;
        }
    }
#endif // __WXMAC__/!__WXMAC__

    pos1 = i;
    pos2 = j;
}

void wxHtmlWordCell::SetSelectionPrivPos(const wxDC& dc, wxHtmlSelection *s) const
{
    unsigned p1, p2;

    Split(dc,
          this == s->GetFromCell() ? s->GetFromPos() : wxDefaultPosition,
          this == s->GetToCell() ? s->GetToPos() : wxDefaultPosition,
          p1, p2);

    wxPoint p(0, m_Word.length());

    if ( this == s->GetFromCell() )
        p.x = p1; // selection starts here
    if ( this == s->GetToCell() )
        p.y = p2; // selection ends here

    if ( this == s->GetFromCell() )
        s->SetFromPrivPos(p);
    if ( this == s->GetToCell() )
        s->SetToPrivPos(p);
}


static void SwitchSelState(wxDC& dc, wxHtmlRenderingInfo& info,
                           bool toSelection)
{
    wxColour fg = info.GetState().GetFgColour();
    wxColour bg = info.GetState().GetBgColour();

    if ( toSelection )
    {
        dc.SetBackgroundMode(wxSOLID);
        dc.SetTextForeground(info.GetStyle().GetSelectedTextColour(fg));
        dc.SetTextBackground(info.GetStyle().GetSelectedTextBgColour(bg));
        dc.SetBackground(wxBrush(info.GetStyle().GetSelectedTextBgColour(bg),
                                 wxSOLID));
    }
    else
    {
        dc.SetBackgroundMode(wxTRANSPARENT);
        dc.SetTextForeground(fg);
        dc.SetTextBackground(bg);
        dc.SetBackground(wxBrush(bg, wxSOLID));
    }
}


void wxHtmlWordCell::Draw(wxDC& dc, int x, int y,
                          int WXUNUSED(view_y1), int WXUNUSED(view_y2),
                          wxHtmlRenderingInfo& info)
{
#if 0 // useful for debugging
    dc.SetPen(*wxBLACK_PEN);
    dc.DrawRectangle(x+m_PosX,y+m_PosY,m_Width /* VZ: +1? */ ,m_Height);
#endif

    bool drawSelectionAfterCell = false;

    if ( info.GetState().GetSelectionState() == wxHTML_SEL_CHANGING )
    {
        // Selection changing, we must draw the word piecewise:
        wxHtmlSelection *s = info.GetSelection();
        wxString txt;
        int w, h;
        int ofs = 0;

        wxPoint priv = (this == s->GetFromCell()) ?
                           s->GetFromPrivPos() : s->GetToPrivPos();

        // NB: this is quite a hack: in order to compute selection boundaries
        //     (in word's characters) we must know current font, which is only
        //     possible inside rendering code. Therefore we update the
        //     information here and store it in wxHtmlSelection so that
        //     ConvertToText can use it later:
        if ( priv == wxDefaultPosition )
        {
            SetSelectionPrivPos(dc, s);
            priv = (this == s->GetFromCell()) ?
                    s->GetFromPrivPos() : s->GetToPrivPos();
        }

        int part1 = priv.x;
        int part2 = priv.y;

        if ( part1 > 0 )
        {
            txt = m_Word.Mid(0, part1);
            dc.DrawText(txt, x + m_PosX, y + m_PosY);
            dc.GetTextExtent(txt, &w, &h);
            ofs += w;
        }

        SwitchSelState(dc, info, true);

        txt = m_Word.Mid(part1, part2-part1);
        dc.DrawText(txt, ofs + x + m_PosX, y + m_PosY);

        if ( (size_t)part2 < m_Word.length() )
        {
            dc.GetTextExtent(txt, &w, &h);
            ofs += w;
            SwitchSelState(dc, info, false);
            txt = m_Word.Mid(part2);
            dc.DrawText(txt, ofs + x + m_PosX, y + m_PosY);
        }
        else
            drawSelectionAfterCell = true;
    }
    else
    {
        wxHtmlSelectionState selstate = info.GetState().GetSelectionState();
        // Not changing selection state, draw the word in single mode:
        if ( selstate != wxHTML_SEL_OUT &&
             dc.GetBackgroundMode() != wxSOLID )
        {
            SwitchSelState(dc, info, true);
        }
        else if ( selstate == wxHTML_SEL_OUT &&
                  dc.GetBackgroundMode() == wxSOLID )
        {
            SwitchSelState(dc, info, false);
        }
        dc.DrawText(m_Word, x + m_PosX, y + m_PosY);
        drawSelectionAfterCell = (selstate != wxHTML_SEL_OUT);
    }

    // NB: If the text is justified then there is usually some free space
    //     between adjacent cells and drawing the selection only onto cells
    //     would result in ugly unselected spaces. The code below detects
    //     this special case and renders the selection *outside* the sell,
    //     too.
    if ( m_Parent->GetAlignHor() == wxHTML_ALIGN_JUSTIFY &&
         drawSelectionAfterCell )
    {
        wxHtmlCell *nextCell = m_Next;
        while ( nextCell && nextCell->IsFormattingCell() )
            nextCell = nextCell->GetNext();
        if ( nextCell )
        {
            int nextX = nextCell->GetPosX();
            if ( m_PosX + m_Width < nextX )
            {
                dc.SetBrush(dc.GetBackground());
                dc.SetPen(*wxTRANSPARENT_PEN);
                dc.DrawRectangle(x + m_PosX + m_Width, y + m_PosY,
                                 nextX - m_PosX - m_Width, m_Height);
            }
        }
    }
}


wxString wxHtmlWordCell::ConvertToText(wxHtmlSelection *s) const
{
    if ( s && (this == s->GetFromCell() || this == s->GetToCell()) )
    {
        wxPoint priv = this == s->GetFromCell() ? s->GetFromPrivPos()
                                                : s->GetToPrivPos();

        // VZ: we may be called before we had a chance to re-render ourselves
        //     and in this case GetFrom/ToPrivPos() is not set yet -- assume
        //     that this only happens in case of a double/triple click (which
        //     seems to be the case now) and so it makes sense to select the
        //     entire contents of the cell in this case
        //
        // TODO: but this really needs to be fixed in some better way later...
        if ( priv != wxDefaultPosition )
        {
            int part1 = priv.x;
            int part2 = priv.y;
            if ( part1 == part2 )
                return wxEmptyString;
            return m_Word.Mid(part1, part2-part1);
        }
        //else: return the whole word below
    }

    return m_Word;
}

wxCursor wxHtmlWordCell::GetMouseCursor(wxHtmlWindowInterface *window) const
{
    if ( !GetLink() )
    {
        return window->GetHTMLCursor(wxHtmlWindowInterface::HTMLCursor_Text);
    }
    else
    {
        return wxHtmlCell::GetMouseCursor(window);
    }
}


//-----------------------------------------------------------------------------
// wxHtmlContainerCell
//-----------------------------------------------------------------------------

IMPLEMENT_ABSTRACT_CLASS(wxHtmlContainerCell, wxHtmlCell)

wxHtmlContainerCell::wxHtmlContainerCell(wxHtmlContainerCell *parent) : wxHtmlCell()
{
    m_Cells = m_LastCell = NULL;
    m_Parent = parent;
    m_MaxTotalWidth = 0;
    if (m_Parent) m_Parent->InsertCell(this);
    m_AlignHor = wxHTML_ALIGN_LEFT;
    m_AlignVer = wxHTML_ALIGN_BOTTOM;
    m_IndentLeft = m_IndentRight = m_IndentTop = m_IndentBottom = 0;
    m_WidthFloat = 100; m_WidthFloatUnits = wxHTML_UNITS_PERCENT;
    m_UseBkColour = false;
    m_UseBorder = false;
    m_MinHeight = 0;
    m_MinHeightAlign = wxHTML_ALIGN_TOP;
    m_LastLayout = -1;
}

wxHtmlContainerCell::~wxHtmlContainerCell()
{
    wxHtmlCell *cell = m_Cells;
    while ( cell )
    {
        wxHtmlCell *cellNext = cell->GetNext();
        delete cell;
        cell = cellNext;
    }
}



void wxHtmlContainerCell::SetIndent(int i, int what, int units)
{
    int val = (units == wxHTML_UNITS_PIXELS) ? i : -i;
    if (what & wxHTML_INDENT_LEFT) m_IndentLeft = val;
    if (what & wxHTML_INDENT_RIGHT) m_IndentRight = val;
    if (what & wxHTML_INDENT_TOP) m_IndentTop = val;
    if (what & wxHTML_INDENT_BOTTOM) m_IndentBottom = val;
    m_LastLayout = -1;
}



int wxHtmlContainerCell::GetIndent(int ind) const
{
    if (ind & wxHTML_INDENT_LEFT) return m_IndentLeft;
    else if (ind & wxHTML_INDENT_RIGHT) return m_IndentRight;
    else if (ind & wxHTML_INDENT_TOP) return m_IndentTop;
    else if (ind & wxHTML_INDENT_BOTTOM) return m_IndentBottom;
    else return -1; /* BUG! Should not be called... */
}




int wxHtmlContainerCell::GetIndentUnits(int ind) const
{
    bool p = false;
    if (ind & wxHTML_INDENT_LEFT) p = m_IndentLeft < 0;
    else if (ind & wxHTML_INDENT_RIGHT) p = m_IndentRight < 0;
    else if (ind & wxHTML_INDENT_TOP) p = m_IndentTop < 0;
    else if (ind & wxHTML_INDENT_BOTTOM) p = m_IndentBottom < 0;
    if (p) return wxHTML_UNITS_PERCENT;
    else return wxHTML_UNITS_PIXELS;
}


bool wxHtmlContainerCell::AdjustPagebreak(int *pagebreak,
                                          wxArrayInt& known_pagebreaks) const
{
    if (!m_CanLiveOnPagebreak)
        return wxHtmlCell::AdjustPagebreak(pagebreak, known_pagebreaks);

    wxHtmlCell *c = GetFirstChild();
    bool rt = false;
    int pbrk = *pagebreak - m_PosY;

    while (c)
    {
        if (c->AdjustPagebreak(&pbrk, known_pagebreaks))
            rt = true;
        c = c->GetNext();
    }
    if (rt)
        *pagebreak = pbrk + m_PosY;
    return rt;
}


void wxHtmlContainerCell::Layout(int w)
{
    wxHtmlCell::Layout(w);

    if (m_LastLayout == w)
        return;
    m_LastLayout = w;

    // VS: Any attempt to layout with negative or zero width leads to hell,
    // but we can't ignore such attempts completely, since it sometimes
    // happen (e.g. when trying how small a table can be). The best thing we
    // can do is to set the width of child cells to zero
    if (w < 1)
    {
       m_Width = 0;
       for (wxHtmlCell *cell = m_Cells; cell; cell = cell->GetNext())
            cell->Layout(0);
            // this does two things: it recursively calls this code on all
            // child contrainers and resets children's position to (0,0)
       return;
    }

    wxHtmlCell *nextCell;
    long xpos = 0, ypos = m_IndentTop;
    int xdelta = 0, ybasicpos = 0, ydiff;
    int s_width, nextWordWidth, s_indent;
    int ysizeup = 0, ysizedown = 0;
    int MaxLineWidth = 0;
    int curLineWidth = 0;
    m_MaxTotalWidth = 0;


    /*

    WIDTH ADJUSTING :

    */

    if (m_WidthFloatUnits == wxHTML_UNITS_PERCENT)
    {
        if (m_WidthFloat < 0) m_Width = (100 + m_WidthFloat) * w / 100;
        else m_Width = m_WidthFloat * w / 100;
    }
    else
    {
        if (m_WidthFloat < 0) m_Width = w + m_WidthFloat;
        else m_Width = m_WidthFloat;
    }

    if (m_Cells)
    {
        int l = (m_IndentLeft < 0) ? (-m_IndentLeft * m_Width / 100) : m_IndentLeft;
        int r = (m_IndentRight < 0) ? (-m_IndentRight * m_Width / 100) : m_IndentRight;
        for (wxHtmlCell *cell = m_Cells; cell; cell = cell->GetNext())
            cell->Layout(m_Width - (l + r));
    }

    /*

    LAYOUTING :

    */

    // adjust indentation:
    s_indent = (m_IndentLeft < 0) ? (-m_IndentLeft * m_Width / 100) : m_IndentLeft;
    s_width = m_Width - s_indent - ((m_IndentRight < 0) ? (-m_IndentRight * m_Width / 100) : m_IndentRight);

    // my own layouting:
    wxHtmlCell *cell = m_Cells,
               *line = m_Cells;
    while (cell != NULL)
    {
        switch (m_AlignVer)
        {
            case wxHTML_ALIGN_TOP :      ybasicpos = 0; break;
            case wxHTML_ALIGN_BOTTOM :   ybasicpos = - cell->GetHeight(); break;
            case wxHTML_ALIGN_CENTER :   ybasicpos = - cell->GetHeight() / 2; break;
        }
        ydiff = cell->GetHeight() + ybasicpos;

        if (cell->GetDescent() + ydiff > ysizedown) ysizedown = cell->GetDescent() + ydiff;
        if (ybasicpos + cell->GetDescent() < -ysizeup) ysizeup = - (ybasicpos + cell->GetDescent());

        // layout nonbreakable run of cells:
        cell->SetPos(xpos, ybasicpos + cell->GetDescent());
        xpos += cell->GetWidth();
        if (!cell->IsTerminalCell())
        {
            // Container cell indicates new line
            if (curLineWidth > m_MaxTotalWidth)
                m_MaxTotalWidth = curLineWidth;

            if (wxMax(cell->GetWidth(), cell->GetMaxTotalWidth()) > m_MaxTotalWidth)
                m_MaxTotalWidth = cell->GetMaxTotalWidth();
            curLineWidth = 0;
        }
        else
            // Normal cell, add maximum cell width to line width
            curLineWidth += cell->GetMaxTotalWidth();

        cell = cell->GetNext();

        // compute length of the next word that would be added:
        nextWordWidth = 0;
        if (cell)
        {
            nextCell = cell;
            do
            {
                nextWordWidth += nextCell->GetWidth();
                nextCell = nextCell->GetNext();
            } while (nextCell && !nextCell->IsLinebreakAllowed());
        }

        // force new line if occurred:
        if ((cell == NULL) ||
            (xpos + nextWordWidth > s_width && cell->IsLinebreakAllowed()))
        {
            if (xpos > MaxLineWidth) MaxLineWidth = xpos;
            if (ysizeup < 0) ysizeup = 0;
            if (ysizedown < 0) ysizedown = 0;
            switch (m_AlignHor) {
                case wxHTML_ALIGN_LEFT :
                case wxHTML_ALIGN_JUSTIFY :
                         xdelta = 0;
                         break;
                case wxHTML_ALIGN_RIGHT :
                         xdelta = 0 + (s_width - xpos);
                         break;
                case wxHTML_ALIGN_CENTER :
                         xdelta = 0 + (s_width - xpos) / 2;
                         break;
            }
            if (xdelta < 0) xdelta = 0;
            xdelta += s_indent;

            ypos += ysizeup;

            if (m_AlignHor != wxHTML_ALIGN_JUSTIFY || cell == NULL)
            {
                while (line != cell)
                {
                    line->SetPos(line->GetPosX() + xdelta,
                                   ypos + line->GetPosY());
                    line = line->GetNext();
                }
            }
            else // align == justify
            {
                // we have to distribute the extra horz space between the cells
                // on this line

                // an added complication is that some cells have fixed size and
                // shouldn't get any increment (it so happens that these cells
                // also don't allow line break on them which provides with an
                // easy way to test for this) -- and neither should the cells
                // adjacent to them as this could result in a visible space
                // between two cells separated by, e.g. font change, cell which
                // is wrong

                int step = s_width - xpos;
                if ( step > 0 )
                {
                    // first count the cells which will get extra space
                    int total = -1;

                    const wxHtmlCell *c;
                    if ( line != cell )
                    {
                        for ( c = line; c != cell; c = c->GetNext() )
                        {
                            if ( c->IsLinebreakAllowed() )
                            {
                                total++;
                            }
                        }
                    }

                    // and now extra space to those cells which merit it
                    if ( total )
                    {
                        // first visible cell on line is not moved:
                        while (line !=cell && !line->IsLinebreakAllowed())
                        {
                            line->SetPos(line->GetPosX() + s_indent,
                                         line->GetPosY() + ypos);
                            line = line->GetNext();
                        }

                        if (line != cell)
                        {
                            line->SetPos(line->GetPosX() + s_indent,
                                         line->GetPosY() + ypos);

                            line = line->GetNext();
                        }

                        for ( int n = 0; line != cell; line = line->GetNext() )
                        {
                            if ( line->IsLinebreakAllowed() )
                            {
                                // offset the next cell relative to this one
                                // thus increasing our size
                                n++;
                            }

                            line->SetPos(line->GetPosX() + s_indent +
                                           ((n * step) / total),
                                           line->GetPosY() + ypos);
                        }
                    }
                    else
                    {
                        // this will cause the code to enter "else branch" below:
                        step = 0;
                    }
                }
                // else branch:
                if ( step <= 0 ) // no extra space to distribute
                {
                    // just set the indent properly
                    while (line != cell)
                    {
                        line->SetPos(line->GetPosX() + s_indent,
                                     line->GetPosY() + ypos);
                        line = line->GetNext();
                    }
                }
            }

            ypos += ysizedown;
            xpos = 0;
            ysizeup = ysizedown = 0;
            line = cell;
        }
    }

    // setup height & width, depending on container layout:
    m_Height = ypos + (ysizedown + ysizeup) + m_IndentBottom;

    if (m_Height < m_MinHeight)
    {
        if (m_MinHeightAlign != wxHTML_ALIGN_TOP)
        {
            int diff = m_MinHeight - m_Height;
            if (m_MinHeightAlign == wxHTML_ALIGN_CENTER) diff /= 2;
            cell = m_Cells;
            while (cell)
            {
                cell->SetPos(cell->GetPosX(), cell->GetPosY() + diff);
                cell = cell->GetNext();
            }
        }
        m_Height = m_MinHeight;
    }

    if (curLineWidth > m_MaxTotalWidth)
        m_MaxTotalWidth = curLineWidth;

    m_MaxTotalWidth += s_indent + ((m_IndentRight < 0) ? (-m_IndentRight * m_Width / 100) : m_IndentRight);
    MaxLineWidth += s_indent + ((m_IndentRight < 0) ? (-m_IndentRight * m_Width / 100) : m_IndentRight);
    if (m_Width < MaxLineWidth) m_Width = MaxLineWidth;
}

void wxHtmlContainerCell::UpdateRenderingStatePre(wxHtmlRenderingInfo& info,
                                                  wxHtmlCell *cell) const
{
    wxHtmlSelection *s = info.GetSelection();
    if (!s) return;
    if (s->GetFromCell() == cell || s->GetToCell() == cell)
    {
        info.GetState().SetSelectionState(wxHTML_SEL_CHANGING);
    }
}

void wxHtmlContainerCell::UpdateRenderingStatePost(wxHtmlRenderingInfo& info,
                                                   wxHtmlCell *cell) const
{
    wxHtmlSelection *s = info.GetSelection();
    if (!s) return;
    if (s->GetToCell() == cell)
        info.GetState().SetSelectionState(wxHTML_SEL_OUT);
    else if (s->GetFromCell() == cell)
        info.GetState().SetSelectionState(wxHTML_SEL_IN);
}

#define mMin(a, b) (((a) < (b)) ? (a) : (b))
#define mMax(a, b) (((a) < (b)) ? (b) : (a))

void wxHtmlContainerCell::Draw(wxDC& dc, int x, int y, int view_y1, int view_y2,
                               wxHtmlRenderingInfo& info)
{
#if 0 // useful for debugging
    dc.SetPen(*wxRED_PEN);
    dc.DrawRectangle(x+m_PosX,y+m_PosY,m_Width,m_Height);
#endif

    int xlocal = x + m_PosX;
    int ylocal = y + m_PosY;

    if (m_UseBkColour)
    {
        wxBrush myb = wxBrush(m_BkColour, wxSOLID);

        int real_y1 = mMax(ylocal, view_y1);
        int real_y2 = mMin(ylocal + m_Height - 1, view_y2);

        dc.SetBrush(myb);
        dc.SetPen(*wxTRANSPARENT_PEN);
        dc.DrawRectangle(xlocal, real_y1, m_Width, real_y2 - real_y1 + 1);
    }

    if (m_UseBorder)
    {
        wxPen mypen1(m_BorderColour1, 1, wxSOLID);
        wxPen mypen2(m_BorderColour2, 1, wxSOLID);

        dc.SetPen(mypen1);
        dc.DrawLine(xlocal, ylocal, xlocal, ylocal + m_Height - 1);
        dc.DrawLine(xlocal, ylocal, xlocal + m_Width, ylocal);
        dc.SetPen(mypen2);
        dc.DrawLine(xlocal + m_Width - 1, ylocal, xlocal +  m_Width - 1, ylocal + m_Height - 1);
        dc.DrawLine(xlocal, ylocal + m_Height - 1, xlocal + m_Width, ylocal + m_Height - 1);
    }

    if (m_Cells)
    {
        // draw container's contents:
        for (wxHtmlCell *cell = m_Cells; cell; cell = cell->GetNext())
        {

            // optimize drawing: don't render off-screen content:
            if ((ylocal + cell->GetPosY() <= view_y2) &&
                (ylocal + cell->GetPosY() + cell->GetHeight() > view_y1))
            {
                // the cell is visible, draw it:
                UpdateRenderingStatePre(info, cell);
                cell->Draw(dc,
                           xlocal, ylocal, view_y1, view_y2,
                           info);
                UpdateRenderingStatePost(info, cell);
            }
            else
            {
                // the cell is off-screen, proceed with font+color+etc.
                // changes only:
                cell->DrawInvisible(dc, xlocal, ylocal, info);
            }
        }
    }
}



void wxHtmlContainerCell::DrawInvisible(wxDC& dc, int x, int y,
                                        wxHtmlRenderingInfo& info)
{
    if (m_Cells)
    {
        for (wxHtmlCell *cell = m_Cells; cell; cell = cell->GetNext())
        {
            UpdateRenderingStatePre(info, cell);
            cell->DrawInvisible(dc, x + m_PosX, y + m_PosY, info);
            UpdateRenderingStatePost(info, cell);
        }
    }
}


wxColour wxHtmlContainerCell::GetBackgroundColour()
{
    if (m_UseBkColour)
        return m_BkColour;
    else
        return wxNullColour;
}



wxHtmlLinkInfo *wxHtmlContainerCell::GetLink(int x, int y) const
{
    wxHtmlCell *cell = FindCellByPos(x, y);

    // VZ: I don't know if we should pass absolute or relative coords to
    //     wxHtmlCell::GetLink()? As the base class version just ignores them
    //     anyhow, it hardly matters right now but should still be clarified
    return cell ? cell->GetLink(x, y) : NULL;
}



void wxHtmlContainerCell::InsertCell(wxHtmlCell *f)
{
    if (!m_Cells) m_Cells = m_LastCell = f;
    else
    {
        m_LastCell->SetNext(f);
        m_LastCell = f;
        if (m_LastCell) while (m_LastCell->GetNext()) m_LastCell = m_LastCell->GetNext();
    }
    f->SetParent(this);
    m_LastLayout = -1;
}



void wxHtmlContainerCell::SetAlign(const wxHtmlTag& tag)
{
    if (tag.HasParam(wxT("ALIGN")))
    {
        wxString alg = tag.GetParam(wxT("ALIGN"));
        alg.MakeUpper();
        if (alg == wxT("CENTER"))
            SetAlignHor(wxHTML_ALIGN_CENTER);
        else if (alg == wxT("LEFT"))
            SetAlignHor(wxHTML_ALIGN_LEFT);
        else if (alg == wxT("JUSTIFY"))
            SetAlignHor(wxHTML_ALIGN_JUSTIFY);
        else if (alg == wxT("RIGHT"))
            SetAlignHor(wxHTML_ALIGN_RIGHT);
        m_LastLayout = -1;
    }
}



void wxHtmlContainerCell::SetWidthFloat(const wxHtmlTag& tag, double pixel_scale)
{
    if (tag.HasParam(wxT("WIDTH")))
    {
        int wdi;
        wxString wd = tag.GetParam(wxT("WIDTH"));

        if (wd[wd.length()-1] == wxT('%'))
        {
            wxSscanf(wd.c_str(), wxT("%i%%"), &wdi);
            SetWidthFloat(wdi, wxHTML_UNITS_PERCENT);
        }
        else
        {
            wxSscanf(wd.c_str(), wxT("%i"), &wdi);
            SetWidthFloat((int)(pixel_scale * (double)wdi), wxHTML_UNITS_PIXELS);
        }
        m_LastLayout = -1;
    }
}



const wxHtmlCell* wxHtmlContainerCell::Find(int condition, const void* param) const
{
    if (m_Cells)
    {
        for (wxHtmlCell *cell = m_Cells; cell; cell = cell->GetNext())
        {
            const wxHtmlCell *r = cell->Find(condition, param);
            if (r) return r;
        }
    }
    return NULL;
}


wxHtmlCell *wxHtmlContainerCell::FindCellByPos(wxCoord x, wxCoord y,
                                               unsigned flags) const
{
    if ( flags & wxHTML_FIND_EXACT )
    {
        for ( const wxHtmlCell *cell = m_Cells; cell; cell = cell->GetNext() )
        {
            int cx = cell->GetPosX(),
                cy = cell->GetPosY();

            if ( (cx <= x) && (cx + cell->GetWidth() > x) &&
                 (cy <= y) && (cy + cell->GetHeight() > y) )
            {
                return cell->FindCellByPos(x - cx, y - cy, flags);
            }
        }
    }
    else if ( flags & wxHTML_FIND_NEAREST_AFTER )
    {
        wxHtmlCell *c;
        for ( const wxHtmlCell *cell = m_Cells; cell; cell = cell->GetNext() )
        {
            if ( cell->IsFormattingCell() )
                continue;
            int cellY = cell->GetPosY();
            if (!( y < cellY || (y < cellY + cell->GetHeight() &&
                                 x < cell->GetPosX() + cell->GetWidth()) ))
                continue;

            c = cell->FindCellByPos(x - cell->GetPosX(), y - cellY, flags);
            if (c) return c;
        }
    }
    else if ( flags & wxHTML_FIND_NEAREST_BEFORE )
    {
        wxHtmlCell *c2, *c = NULL;
        for ( const wxHtmlCell *cell = m_Cells; cell; cell = cell->GetNext() )
        {
            if ( cell->IsFormattingCell() )
                continue;
            int cellY = cell->GetPosY();
            if (!( cellY + cell->GetHeight() <= y ||
                   (y >= cellY && x >= cell->GetPosX()) ))
                break;
            c2 = cell->FindCellByPos(x - cell->GetPosX(), y - cellY, flags);
            if (c2)
                c = c2;
        }
        if (c) return c;
    }

    return NULL;
}


bool wxHtmlContainerCell::ProcessMouseClick(wxHtmlWindowInterface *window,
                                            const wxPoint& pos,
                                            const wxMouseEvent& event)
{
#if WXWIN_COMPATIBILITY_2_6
    wxHtmlCellOnMouseClickCompatHelper compat(window, pos, event);
    return compat.CallOnMouseClick(this);
}

void wxHtmlContainerCell::OnMouseClick(wxWindow*,
                                       int, int, const wxMouseEvent& event)
{
    wxCHECK_RET( gs_helperOnMouseClick, _T("unexpected call to OnMouseClick") );
    wxHtmlWindowInterface *window = gs_helperOnMouseClick->window;
    const wxPoint& pos = gs_helperOnMouseClick->pos;
#endif // WXWIN_COMPATIBILITY_2_6

    bool retval = false;
    wxHtmlCell *cell = FindCellByPos(pos.x, pos.y);
    if ( cell )
        retval = cell->ProcessMouseClick(window, pos, event);

#if WXWIN_COMPATIBILITY_2_6
    gs_helperOnMouseClick->retval = retval;
#else
    return retval;
#endif // WXWIN_COMPATIBILITY_2_6
}


wxHtmlCell *wxHtmlContainerCell::GetFirstTerminal() const
{
    if ( m_Cells )
    {
        wxHtmlCell *c2;
        for (wxHtmlCell *c = m_Cells; c; c = c->GetNext())
        {
            c2 = c->GetFirstTerminal();
            if ( c2 )
                return c2;
        }
    }
    return NULL;
}

wxHtmlCell *wxHtmlContainerCell::GetLastTerminal() const
{
    if ( m_Cells )
    {
        // most common case first:
        wxHtmlCell *c = m_LastCell->GetLastTerminal();
        if ( c )
            return c;

        wxHtmlCell *ctmp;
        wxHtmlCell *c2 = NULL;
        for (c = m_Cells; c; c = c->GetNext())
        {
            ctmp = c->GetLastTerminal();
            if ( ctmp )
                c2 = ctmp;
        }
        return c2;
    }
    else
        return NULL;
}


static bool IsEmptyContainer(wxHtmlContainerCell *cell)
{
    for ( wxHtmlCell *c = cell->GetFirstChild(); c; c = c->GetNext() )
    {
        if ( !c->IsTerminalCell() || !c->IsFormattingCell() )
            return false;
    }
    return true;
}

void wxHtmlContainerCell::RemoveExtraSpacing(bool top, bool bottom)
{
    if ( top )
        SetIndent(0, wxHTML_INDENT_TOP);
    if ( bottom )
        SetIndent(0, wxHTML_INDENT_BOTTOM);

    if ( m_Cells )
    {
        wxHtmlCell *c;
        wxHtmlContainerCell *cont;
        if ( top )
        {
            for ( c = m_Cells; c; c = c->GetNext() )
            {
                if ( c->IsTerminalCell() )
                {
                    if ( !c->IsFormattingCell() )
                        break;
                }
                else
                {
                    cont = (wxHtmlContainerCell*)c;
                    if ( IsEmptyContainer(cont) )
                    {
                        cont->SetIndent(0, wxHTML_INDENT_VERTICAL);
                    }
                    else
                    {
                        cont->RemoveExtraSpacing(true, false);
                        break;
                    }
                }
            }
        }

        if ( bottom )
        {
            wxArrayPtrVoid arr;
            for ( c = m_Cells; c; c = c->GetNext() )
                arr.Add((void*)c);

            for ( int i = arr.GetCount() - 1; i >= 0; i--)
            {
                c = (wxHtmlCell*)arr[i];
                if ( c->IsTerminalCell() )
                {
                    if ( !c->IsFormattingCell() )
                        break;
                }
                else
                {
                    cont = (wxHtmlContainerCell*)c;
                    if ( IsEmptyContainer(cont) )
                    {
                        cont->SetIndent(0, wxHTML_INDENT_VERTICAL);
                    }
                    else
                    {
                        cont->RemoveExtraSpacing(false, true);
                        break;
                    }
                }
            }
        }
    }
}




// --------------------------------------------------------------------------
// wxHtmlColourCell
// --------------------------------------------------------------------------

IMPLEMENT_ABSTRACT_CLASS(wxHtmlColourCell, wxHtmlCell)

void wxHtmlColourCell::Draw(wxDC& dc,
                            int x, int y,
                            int WXUNUSED(view_y1), int WXUNUSED(view_y2),
                            wxHtmlRenderingInfo& info)
{
    DrawInvisible(dc, x, y, info);
}

void wxHtmlColourCell::DrawInvisible(wxDC& dc,
                                     int WXUNUSED(x), int WXUNUSED(y),
                                     wxHtmlRenderingInfo& info)
{
    wxHtmlRenderingState& state = info.GetState();
    if (m_Flags & wxHTML_CLR_FOREGROUND)
    {
        state.SetFgColour(m_Colour);
        if (state.GetSelectionState() != wxHTML_SEL_IN)
            dc.SetTextForeground(m_Colour);
        else
            dc.SetTextForeground(
                    info.GetStyle().GetSelectedTextColour(m_Colour));
    }
    if (m_Flags & wxHTML_CLR_BACKGROUND)
    {
        state.SetBgColour(m_Colour);
        if (state.GetSelectionState() != wxHTML_SEL_IN)
        {
            dc.SetTextBackground(m_Colour);
            dc.SetBackground(wxBrush(m_Colour, wxSOLID));
        }
        else
        {
            wxColour c = info.GetStyle().GetSelectedTextBgColour(m_Colour);
            dc.SetTextBackground(c);
            dc.SetBackground(wxBrush(c, wxSOLID));
        }
    }
}




// ---------------------------------------------------------------------------
// wxHtmlFontCell
// ---------------------------------------------------------------------------

IMPLEMENT_ABSTRACT_CLASS(wxHtmlFontCell, wxHtmlCell)

void wxHtmlFontCell::Draw(wxDC& dc,
                          int WXUNUSED(x), int WXUNUSED(y),
                          int WXUNUSED(view_y1), int WXUNUSED(view_y2),
                          wxHtmlRenderingInfo& WXUNUSED(info))
{
    dc.SetFont(m_Font);
}

void wxHtmlFontCell::DrawInvisible(wxDC& dc, int WXUNUSED(x), int WXUNUSED(y),
                                   wxHtmlRenderingInfo& WXUNUSED(info))
{
    dc.SetFont(m_Font);
}








// ---------------------------------------------------------------------------
// wxHtmlWidgetCell
// ---------------------------------------------------------------------------

IMPLEMENT_ABSTRACT_CLASS(wxHtmlWidgetCell, wxHtmlCell)

wxHtmlWidgetCell::wxHtmlWidgetCell(wxWindow *wnd, int w)
{
    int sx, sy;
    m_Wnd = wnd;
    m_Wnd->GetSize(&sx, &sy);
    m_Width = sx, m_Height = sy;
    m_WidthFloat = w;
}


void wxHtmlWidgetCell::Draw(wxDC& WXUNUSED(dc),
                            int WXUNUSED(x), int WXUNUSED(y),
                            int WXUNUSED(view_y1), int WXUNUSED(view_y2),
                            wxHtmlRenderingInfo& WXUNUSED(info))
{
    int absx = 0, absy = 0, stx, sty;
    wxHtmlCell *c = this;

    while (c)
    {
        absx += c->GetPosX();
        absy += c->GetPosY();
        c = c->GetParent();
    }

    wxScrolledWindow *scrolwin =
        wxDynamicCast(m_Wnd->GetParent(), wxScrolledWindow);
    wxCHECK_RET( scrolwin,
                 _T("widget cells can only be placed in wxHtmlWindow") );

    scrolwin->GetViewStart(&stx, &sty);
    m_Wnd->SetSize(absx - wxHTML_SCROLL_STEP * stx,
                   absy  - wxHTML_SCROLL_STEP * sty,
                   m_Width, m_Height);
}



void wxHtmlWidgetCell::DrawInvisible(wxDC& WXUNUSED(dc),
                                     int WXUNUSED(x), int WXUNUSED(y),
                                     wxHtmlRenderingInfo& WXUNUSED(info))
{
    int absx = 0, absy = 0, stx, sty;
    wxHtmlCell *c = this;

    while (c)
    {
        absx += c->GetPosX();
        absy += c->GetPosY();
        c = c->GetParent();
    }

    ((wxScrolledWindow*)(m_Wnd->GetParent()))->GetViewStart(&stx, &sty);
    m_Wnd->SetSize(absx - wxHTML_SCROLL_STEP * stx, absy  - wxHTML_SCROLL_STEP * sty, m_Width, m_Height);
}



void wxHtmlWidgetCell::Layout(int w)
{
    if (m_WidthFloat != 0)
    {
        m_Width = (w * m_WidthFloat) / 100;
        m_Wnd->SetSize(m_Width, m_Height);
    }

    wxHtmlCell::Layout(w);
}



// ----------------------------------------------------------------------------
// wxHtmlTerminalCellsInterator
// ----------------------------------------------------------------------------

const wxHtmlCell* wxHtmlTerminalCellsInterator::operator++()
{
    if ( !m_pos )
        return NULL;

    do
    {
        if ( m_pos == m_to )
        {
            m_pos = NULL;
            return NULL;
        }

        if ( m_pos->GetNext() )
            m_pos = m_pos->GetNext();
        else
        {
            // we must go up the hierarchy until we reach container where this
            // is not the last child, and then go down to first terminal cell:
            while ( m_pos->GetNext() == NULL )
            {
                m_pos = m_pos->GetParent();
                if ( !m_pos )
                    return NULL;
            }
            m_pos = m_pos->GetNext();
        }
        while ( m_pos->GetFirstChild() != NULL )
            m_pos = m_pos->GetFirstChild();
    } while ( !m_pos->IsTerminalCell() );

    return m_pos;
}

#endif
