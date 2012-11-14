/////////////////////////////////////////////////////////////////////////////
// Name:        gbsizer.cpp
// Purpose:     wxGridBagSizer:  A sizer that can lay out items in a grid,
//              with items at specified cells, and with the option of row
//              and/or column spanning
//
// Author:      Robin Dunn
// Created:     03-Nov-2003
// RCS-ID:      $Id: gbsizer.cpp 54568 2008-07-10 01:32:23Z RD $
// Copyright:   (c) Robin Dunn
// Licence:     wxWindows licence
/////////////////////////////////////////////////////////////////////////////

// For compilers that support precompilation, includes "wx.h".
#include "wx/wxprec.h"

#ifdef __BORLANDC__
    #pragma hdrstop
#endif

#include "wx/gbsizer.h"

//---------------------------------------------------------------------------

IMPLEMENT_DYNAMIC_CLASS(wxGBSizerItem, wxSizerItem)
IMPLEMENT_CLASS(wxGridBagSizer, wxFlexGridSizer)

const wxGBSpan wxDefaultSpan;

//---------------------------------------------------------------------------
// wxGBSizerItem
//---------------------------------------------------------------------------

wxGBSizerItem::wxGBSizerItem( int width,
                              int height,
                              const wxGBPosition& pos,
                              const wxGBSpan& span,
                              int flag,
                              int border,
                              wxObject* userData)
    : wxSizerItem(width, height, 0, flag, border, userData),
      m_pos(pos),
      m_span(span),
      m_gbsizer(NULL)
{
}


wxGBSizerItem::wxGBSizerItem( wxWindow *window,
                              const wxGBPosition& pos,
                              const wxGBSpan& span,
                              int flag,
                              int border,
                              wxObject* userData )
    : wxSizerItem(window, 0, flag, border, userData),
      m_pos(pos),
      m_span(span),
      m_gbsizer(NULL)
{
}


wxGBSizerItem::wxGBSizerItem( wxSizer *sizer,
                              const wxGBPosition& pos,
                              const wxGBSpan& span,
                              int flag,
                              int border,
                              wxObject* userData )
    : wxSizerItem(sizer, 0, flag, border, userData),
      m_pos(pos),
      m_span(span),
      m_gbsizer(NULL)
{
}

wxGBSizerItem::wxGBSizerItem()
    : wxSizerItem(),
      m_pos(-1,-1),
      m_span(-1,-1),
      m_gbsizer(NULL)
{
}

//---------------------------------------------------------------------------


void wxGBSizerItem::GetPos(int& row, int& col) const
{
    row = m_pos.GetRow();
    col = m_pos.GetCol();
}

void wxGBSizerItem::GetSpan(int& rowspan, int& colspan) const
{
    rowspan = m_span.GetRowspan();
    colspan = m_span.GetColspan();
}


bool wxGBSizerItem::SetPos( const wxGBPosition& pos )
{
    if (m_gbsizer)
    {
        wxCHECK_MSG( !m_gbsizer->CheckForIntersection(pos, m_span, this), false,
                 wxT("An item is already at that position") );
    }
    m_pos = pos;
    return true;
}

bool wxGBSizerItem::SetSpan( const wxGBSpan& span )
{
    if (m_gbsizer)
    {
        wxCHECK_MSG( !m_gbsizer->CheckForIntersection(m_pos, span, this), false,
                 wxT("An item is already at that position") );
    }
    m_span = span;
    return true;
}


inline bool InRange(int val, int min, int max)
{
    return (val >= min && val <= max);
}

bool wxGBSizerItem::Intersects(const wxGBSizerItem& other)
{
    return Intersects(other.GetPos(), other.GetSpan());
}

bool wxGBSizerItem::Intersects(const wxGBPosition& pos, const wxGBSpan& span)
{

    int row, col, endrow, endcol;
    int otherrow, othercol, otherendrow, otherendcol;

    GetPos(row, col);
    GetEndPos(endrow, endcol);

    otherrow = pos.GetRow();
    othercol = pos.GetCol();
    otherendrow = otherrow + span.GetRowspan() - 1;
    otherendcol = othercol + span.GetColspan() - 1;

    // is the other item's start or end in the range of this one?
    if (( InRange(otherrow, row, endrow) && InRange(othercol, col, endcol) ) ||
        ( InRange(otherendrow, row, endrow) && InRange(otherendcol, col, endcol) ))
        return true;

    // is this item's start or end in the range of the other one?
    if (( InRange(row, otherrow, otherendrow) && InRange(col, othercol, otherendcol) ) ||
        ( InRange(endrow, otherrow, otherendrow) && InRange(endcol, othercol, otherendcol) ))
        return true;

    return false;
}


void wxGBSizerItem::GetEndPos(int& row, int& col)
{
    row = m_pos.GetRow() + m_span.GetRowspan() - 1;
    col = m_pos.GetCol() + m_span.GetColspan() - 1;
}


//---------------------------------------------------------------------------
// wxGridBagSizer
//---------------------------------------------------------------------------

wxGridBagSizer::wxGridBagSizer(int vgap, int hgap )
    : wxFlexGridSizer(1, vgap, hgap),
      m_emptyCellSize(10,20)

{
}


wxSizerItem* wxGridBagSizer::Add( wxWindow *window,
                                  const wxGBPosition& pos, const wxGBSpan& span,
                                  int flag, int border,  wxObject* userData )
{
    wxGBSizerItem* item = new wxGBSizerItem(window, pos, span, flag, border, userData);
    if ( Add(item) )
        return item;
    else
    {
        delete item;
        return (wxSizerItem*)NULL;
    }
}

wxSizerItem* wxGridBagSizer::Add( wxSizer *sizer,
                          const wxGBPosition& pos, const wxGBSpan& span,
                          int flag, int border,  wxObject* userData )
{
    wxGBSizerItem* item = new wxGBSizerItem(sizer, pos, span, flag, border, userData);
    if ( Add(item) )
        return item;
    else
    {
        delete item;
        return (wxSizerItem*)NULL;
    }
}

wxSizerItem* wxGridBagSizer::Add( int width, int height,
                          const wxGBPosition& pos, const wxGBSpan& span,
                          int flag, int border,  wxObject* userData )
{
    wxGBSizerItem* item = new wxGBSizerItem(width, height, pos, span, flag, border, userData);
    if ( Add(item) )
        return item;
    else
    {
        delete item;
        return (wxSizerItem*)NULL;
    }
}

wxSizerItem* wxGridBagSizer::Add( wxGBSizerItem *item )
{
    wxCHECK_MSG( !CheckForIntersection(item), NULL,
                 wxT("An item is already at that position") );
    m_children.Append(item);
    item->SetGBSizer(this);
    if ( item->GetWindow() )
        item->GetWindow()->SetContainingSizer( this );

    return item;
}



//---------------------------------------------------------------------------

wxSize wxGridBagSizer::GetCellSize(int row, int col) const
{
    wxCHECK_MSG( (row < m_rows) && (col < m_cols),
                 wxDefaultSize,
                 wxT("Invalid cell."));
    return wxSize( m_colWidths[col], m_rowHeights[row] );
}


wxGBPosition wxGridBagSizer::GetItemPosition(wxWindow *window)
{
    wxGBPosition badpos(-1,-1);
    wxGBSizerItem* item = FindItem(window);
    wxCHECK_MSG(item, badpos, wxT("Failed to find item."));
    return item->GetPos();
}


wxGBPosition wxGridBagSizer::GetItemPosition(wxSizer *sizer)
{
    wxGBPosition badpos(-1,-1);
    wxGBSizerItem* item = FindItem(sizer);
    wxCHECK_MSG(item, badpos, wxT("Failed to find item."));
    return item->GetPos();
}


wxGBPosition wxGridBagSizer::GetItemPosition(size_t index)
{
    wxGBPosition badpos(-1,-1);
    wxSizerItemList::compatibility_iterator node = m_children.Item( index );
    wxCHECK_MSG( node, badpos, _T("Failed to find item.") );
    wxGBSizerItem* item = (wxGBSizerItem*)node->GetData();
    return item->GetPos();
}



bool wxGridBagSizer::SetItemPosition(wxWindow *window, const wxGBPosition& pos)
{
    wxGBSizerItem* item = FindItem(window);
    wxCHECK_MSG(item, false, wxT("Failed to find item."));
    return item->SetPos(pos);
}


bool wxGridBagSizer::SetItemPosition(wxSizer *sizer, const wxGBPosition& pos)
{
    wxGBSizerItem* item = FindItem(sizer);
    wxCHECK_MSG(item, false, wxT("Failed to find item."));
    return item->SetPos(pos);
}


bool wxGridBagSizer::SetItemPosition(size_t index, const wxGBPosition& pos)
{
    wxSizerItemList::compatibility_iterator node = m_children.Item( index );
    wxCHECK_MSG( node, false, _T("Failed to find item.") );
    wxGBSizerItem* item = (wxGBSizerItem*)node->GetData();
    return item->SetPos(pos);
}



wxGBSpan wxGridBagSizer::GetItemSpan(wxWindow *window)
{
    wxGBSpan badspan(-1,-1);
    wxGBSizerItem* item = FindItem(window);
    wxCHECK_MSG( item, badspan, _T("Failed to find item.") );
    return item->GetSpan();
}


wxGBSpan wxGridBagSizer::GetItemSpan(wxSizer *sizer)
{
    wxGBSpan badspan(-1,-1);
    wxGBSizerItem* item = FindItem(sizer);
    wxCHECK_MSG( item, badspan, _T("Failed to find item.") );
    return item->GetSpan();
}


wxGBSpan wxGridBagSizer::GetItemSpan(size_t index)
{
    wxGBSpan badspan(-1,-1);
    wxSizerItemList::compatibility_iterator node = m_children.Item( index );
    wxCHECK_MSG( node, badspan, _T("Failed to find item.") );
    wxGBSizerItem* item = (wxGBSizerItem*)node->GetData();
    return item->GetSpan();
}



bool wxGridBagSizer::SetItemSpan(wxWindow *window, const wxGBSpan& span)
{
    wxGBSizerItem* item = FindItem(window);
    wxCHECK_MSG(item, false, wxT("Failed to find item."));
    return item->SetSpan(span);
}


bool wxGridBagSizer::SetItemSpan(wxSizer *sizer, const wxGBSpan& span)
{
    wxGBSizerItem* item = FindItem(sizer);
    wxCHECK_MSG(item, false, wxT("Failed to find item."));
    return item->SetSpan(span);
}


bool wxGridBagSizer::SetItemSpan(size_t index, const wxGBSpan& span)
{
    wxSizerItemList::compatibility_iterator node = m_children.Item( index );
    wxCHECK_MSG( node, false, _T("Failed to find item.") );
    wxGBSizerItem* item = (wxGBSizerItem*)node->GetData();
    return item->SetSpan(span);
}




wxGBSizerItem* wxGridBagSizer::FindItem(wxWindow* window)
{
    wxSizerItemList::compatibility_iterator node = m_children.GetFirst();
    while (node)
    {
        wxGBSizerItem* item = (wxGBSizerItem*)node->GetData();
        if ( item->GetWindow() == window )
            return item;
        node = node->GetNext();
    }
    return NULL;
}


wxGBSizerItem* wxGridBagSizer::FindItem(wxSizer* sizer)
{
    wxSizerItemList::compatibility_iterator node = m_children.GetFirst();
    while (node)
    {
        wxGBSizerItem* item = (wxGBSizerItem*)node->GetData();
        if ( item->GetSizer() == sizer )
            return item;
        node = node->GetNext();
    }
    return NULL;
}




wxGBSizerItem* wxGridBagSizer::FindItemAtPosition(const wxGBPosition& pos)
{
    wxSizerItemList::compatibility_iterator node = m_children.GetFirst();
    while (node)
    {
        wxGBSizerItem* item = (wxGBSizerItem*)node->GetData();
        if ( item->Intersects(pos, wxDefaultSpan) )
            return item;
        node = node->GetNext();
    }
    return NULL;
}




wxGBSizerItem* wxGridBagSizer::FindItemAtPoint(const wxPoint& pt)
{
    wxSizerItemList::compatibility_iterator node = m_children.GetFirst();
    while (node)
    {
        wxGBSizerItem* item = (wxGBSizerItem*)node->GetData();
        wxRect rect(item->GetPosition(), item->GetSize());
        rect.Inflate(m_hgap, m_vgap);
        if ( rect.Contains(pt) )
            return item;
        node = node->GetNext();
    }
    return NULL;
}




wxGBSizerItem* wxGridBagSizer::FindItemWithData(const wxObject* userData)
{
    wxSizerItemList::compatibility_iterator node = m_children.GetFirst();
    while (node)
    {
        wxGBSizerItem* item = (wxGBSizerItem*)node->GetData();
        if ( item->GetUserData() == userData )
            return item;
        node = node->GetNext();
    }
    return NULL;
}




//---------------------------------------------------------------------------

// Figure out what all the min row heights and col widths are, and calculate
// min size from that.
wxSize wxGridBagSizer::CalcMin()
{
    int idx;

    if (m_children.GetCount() == 0)
        return m_emptyCellSize;

    m_rowHeights.Empty();
    m_colWidths.Empty();

    wxSizerItemList::compatibility_iterator node = m_children.GetFirst();
    while (node)
    {
        wxGBSizerItem* item = (wxGBSizerItem*)node->GetData();
        if ( item->ShouldAccountFor() )
        {
            int row, col, endrow, endcol;

            item->GetPos(row, col);
            item->GetEndPos(endrow, endcol);

            // fill heights and widths upto this item if needed
            while ( m_rowHeights.GetCount() <= (size_t)endrow )
                m_rowHeights.Add(m_emptyCellSize.GetHeight());
            while ( m_colWidths.GetCount() <= (size_t)endcol )
                m_colWidths.Add(m_emptyCellSize.GetWidth());

            // See if this item increases the size of its row(s) or col(s)
            wxSize size(item->CalcMin());
            for (idx=row; idx <= endrow; idx++)
                m_rowHeights[idx] = wxMax(m_rowHeights[idx], size.GetHeight() / (endrow-row+1));
            for (idx=col; idx <= endcol; idx++)
                m_colWidths[idx] = wxMax(m_colWidths[idx], size.GetWidth() / (endcol-col+1));
        }
        node = node->GetNext();
    }

    AdjustForOverflow();
    AdjustForFlexDirection();

    // Now traverse the heights and widths arrays calcing the totals, including gaps
    int width = 0;
    m_cols = m_colWidths.GetCount();
    for (idx=0; idx < m_cols; idx++)
        width += m_colWidths[idx] + ( idx == m_cols-1 ? 0 : m_hgap );

    int height = 0;
    m_rows = m_rowHeights.GetCount();
    for (idx=0; idx < m_rows; idx++)
        height += m_rowHeights[idx] + ( idx == m_rows-1 ? 0 : m_vgap );

    m_calculatedMinSize = wxSize(width, height);
    return m_calculatedMinSize;
}



void wxGridBagSizer::RecalcSizes()
{
    if (m_children.GetCount() == 0)
        return;

    wxPoint pt( GetPosition() );
    wxSize  sz( GetSize() );

    m_rows = m_rowHeights.GetCount();
    m_cols = m_colWidths.GetCount();
    int idx, width, height;

    AdjustForGrowables(sz, m_calculatedMinSize, m_rows, m_cols);

    // Find the start positions on the window of the rows and columns
    wxArrayInt rowpos;
    rowpos.Add(0, m_rows);
    int y = pt.y;
    for (idx=0; idx < m_rows; idx++)
    {
        height = m_rowHeights[idx] + m_vgap;
        rowpos[idx] = y;
        y += height;
    }

    wxArrayInt colpos;
    colpos.Add(0, m_cols);
    int x = pt.x;
    for (idx=0; idx < m_cols; idx++)
    {
        width = m_colWidths[idx] + m_hgap;
        colpos[idx] = x;
        x += width;
    }


    // Now iterate the children, setting each child's dimensions
    wxSizerItemList::compatibility_iterator node = m_children.GetFirst();
    while (node)
    {
        int row, col, endrow, endcol;
        wxGBSizerItem* item = (wxGBSizerItem*)node->GetData();

        if ( item->ShouldAccountFor() )
        {
            item->GetPos(row, col);
            item->GetEndPos(endrow, endcol);

            height = 0;
            for(idx=row; idx <= endrow; idx++)
                height += m_rowHeights[idx];
            height += (endrow - row) * m_vgap; // add a vgap for every row spanned

            width = 0;
            for (idx=col; idx <= endcol; idx++)
                width += m_colWidths[idx];
            width += (endcol - col) * m_hgap; // add a hgap for every col spanned

            SetItemBounds(item, colpos[col], rowpos[row], width, height);
        }

        node = node->GetNext();
    }
}


// Sometimes CalcMin can result in some rows or cols having too much space in
// them because as it traverses the items it makes some assumptions when
// items span to other cells.  But those assumptions can become invalid later
// on when other items are fitted into the same rows or columns that the
// spanning item occupies. This method tries to find those situations and
// fixes them.
void wxGridBagSizer::AdjustForOverflow()
{
    int row, col;
    
    for (row=0; row<(int)m_rowHeights.GetCount(); row++)
    {
        int rowExtra=INT_MAX;
        int rowHeight = m_rowHeights[row];
        for (col=0; col<(int)m_colWidths.GetCount(); col++)
        {
            wxGBPosition pos(row,col);
            wxGBSizerItem* item = FindItemAtPosition(pos);
            if ( !item || !item->ShouldAccountFor() )
                continue;

            int endrow, endcol;
            item->GetEndPos(endrow, endcol);
            
            // If the item starts in this position and doesn't span rows, then
            // just look at the whole item height
            if ( item->GetPos() == pos && endrow == row )
            {
                int itemHeight = item->CalcMin().GetHeight();
                rowExtra = wxMin(rowExtra, rowHeight - itemHeight);
                continue;
            }

            // Otherwise, only look at spanning items if they end on this row
            if ( endrow == row )
            {
                // first deduct the portions of the item that are on prior rows
                int itemHeight = item->CalcMin().GetHeight();
                for (int r=item->GetPos().GetRow(); r<row; r++)
                    itemHeight -= (m_rowHeights[r] + GetHGap());

                if ( itemHeight < 0 )
                    itemHeight = 0;
                
                // and check how much is left
                rowExtra = wxMin(rowExtra, rowHeight - itemHeight);
            }
        }
        if ( rowExtra && rowExtra != INT_MAX )
            m_rowHeights[row] -= rowExtra;
    }

    // Now do the same thing for columns
    for (col=0; col<(int)m_colWidths.GetCount(); col++)
    {
        int colExtra=INT_MAX;
        int colWidth = m_colWidths[col];
        for (row=0; row<(int)m_rowHeights.GetCount(); row++)
        {
            wxGBPosition pos(row,col);
            wxGBSizerItem* item = FindItemAtPosition(pos);
            if ( !item || !item->ShouldAccountFor() )
                continue;

            int endrow, endcol;
            item->GetEndPos(endrow, endcol);
            
            if ( item->GetPos() == pos && endcol == col )
            {
                int itemWidth = item->CalcMin().GetWidth();
                colExtra = wxMin(colExtra, colWidth - itemWidth);
                continue;
            }

            if ( endcol == col )
            {
                int itemWidth = item->CalcMin().GetWidth();
                for (int c=item->GetPos().GetCol(); c<col; c++)
                    itemWidth -= (m_colWidths[c] + GetVGap());

                if ( itemWidth < 0 )
                    itemWidth = 0;
                
                colExtra = wxMin(colExtra, colWidth - itemWidth);
            }
        }
        if ( colExtra && colExtra != INT_MAX )
            m_colWidths[col] -= colExtra;
    }
}

//---------------------------------------------------------------------------

bool wxGridBagSizer::CheckForIntersection(wxGBSizerItem* item, wxGBSizerItem* excludeItem)
{
    return CheckForIntersection(item->GetPos(), item->GetSpan(), excludeItem);
}

bool wxGridBagSizer::CheckForIntersection(const wxGBPosition& pos, const wxGBSpan& span, wxGBSizerItem* excludeItem)
{
    wxSizerItemList::compatibility_iterator node = m_children.GetFirst();
    while (node)
    {
        wxGBSizerItem* item = (wxGBSizerItem*)node->GetData();
        node = node->GetNext();

        if ( excludeItem && item == excludeItem )
            continue;

        if ( item->Intersects(pos, span) )
            return true;

    }
    return false;
}


// Assumes a 10x10 grid, and returns the first empty cell found.  This is
// really stupid but it is only used by the Add methods that match the base
// class virtuals, which should normally not be used anyway...
wxGBPosition wxGridBagSizer::FindEmptyCell()
{
    int row, col;

    for (row=0; row<10; row++)
        for (col=0; col<10; col++)
        {
            wxGBPosition pos(row, col);
            if ( !CheckForIntersection(pos, wxDefaultSpan) )
                return pos;
        }
    return wxGBPosition(-1, -1);
}


//---------------------------------------------------------------------------

// The Add base class virtuals should not be used with this class, but
// we'll try to make them automatically select a location for the item
// anyway.

wxSizerItem* wxGridBagSizer::Add( wxWindow *window, int, int flag, int border, wxObject* userData )
{
    return Add(window, FindEmptyCell(), wxDefaultSpan, flag, border, userData);
}

wxSizerItem* wxGridBagSizer::Add( wxSizer *sizer, int, int flag, int border, wxObject* userData )
{
    return Add(sizer, FindEmptyCell(), wxDefaultSpan, flag, border, userData);
}

wxSizerItem* wxGridBagSizer::Add( int width, int height, int, int flag, int border, wxObject* userData )
{
    return Add(width, height, FindEmptyCell(), wxDefaultSpan, flag, border, userData);
}



// The Insert nad Prepend base class virtuals that are not appropriate for
// this class and should not be used.  Their implementation in this class
// simply fails.

wxSizerItem* wxGridBagSizer::Add( wxSizerItem * )
{
    wxFAIL_MSG(wxT("Invalid Add form called."));
    return (wxSizerItem*)NULL;
}

wxSizerItem* wxGridBagSizer::Prepend( wxWindow *, int, int, int, wxObject*  )
{
    wxFAIL_MSG(wxT("Prepend should not be used with wxGridBagSizer."));
    return (wxSizerItem*)NULL;
}

wxSizerItem* wxGridBagSizer::Prepend( wxSizer *, int, int, int, wxObject*  )
{
    wxFAIL_MSG(wxT("Prepend should not be used with wxGridBagSizer."));
    return (wxSizerItem*)NULL;
}

wxSizerItem* wxGridBagSizer::Prepend( int, int, int, int, int, wxObject*  )
{
    wxFAIL_MSG(wxT("Prepend should not be used with wxGridBagSizer."));
    return (wxSizerItem*)NULL;
}

wxSizerItem* wxGridBagSizer::Prepend( wxSizerItem * )
{
    wxFAIL_MSG(wxT("Prepend should not be used with wxGridBagSizer."));
    return (wxSizerItem*)NULL;
}


wxSizerItem* wxGridBagSizer::Insert( size_t, wxWindow *, int, int, int, wxObject*  )
{
    wxFAIL_MSG(wxT("Insert should not be used with wxGridBagSizer."));
    return (wxSizerItem*)NULL;
}

wxSizerItem* wxGridBagSizer::Insert( size_t, wxSizer *, int, int, int, wxObject*  )
{
    wxFAIL_MSG(wxT("Insert should not be used with wxGridBagSizer."));
    return (wxSizerItem*)NULL;
}

wxSizerItem* wxGridBagSizer::Insert( size_t, int, int, int, int, int, wxObject*  )
{
    wxFAIL_MSG(wxT("Insert should not be used with wxGridBagSizer."));
    return (wxSizerItem*)NULL;
}

wxSizerItem* wxGridBagSizer::Insert( size_t, wxSizerItem * )
{
    wxFAIL_MSG(wxT("Insert should not be used with wxGridBagSizer."));
    return (wxSizerItem*)NULL;
}


//---------------------------------------------------------------------------
//---------------------------------------------------------------------------
