///////////////////////////////////////////////////////////////////////////
// Name:        src/generic/gridsel.cpp
// Purpose:     wxGridSelection
// Author:      Stefan Neis
// Modified by:
// Created:     20/02/1999
// RCS-ID:      $Id: gridsel.cpp 38788 2006-04-18 08:11:26Z ABX $
// Copyright:   (c) Stefan Neis (Stefan.Neis@t-online.de)
// Licence:     wxWindows licence
/////////////////////////////////////////////////////////////////////////////

// For compilers that support precompilation, includes "wx/wx.h".
#include "wx/wxprec.h"

#ifdef __BORLANDC__
    #pragma hdrstop
#endif

// ============================================================================
// declarations
// ============================================================================

// ----------------------------------------------------------------------------
// headers
// ----------------------------------------------------------------------------

#if wxUSE_GRID

#include "wx/generic/gridsel.h"


// Some explanation for the members of the class:
// m_cellSelection stores individual selected cells
//   -- this is only used if m_selectionMode == wxGridSelectCells
// m_blockSelectionTopLeft and m_blockSelectionBottomRight
//   store the upper left and lower right corner of selected Blocks
// m_rowSelection and m_colSelection store individual selected
//   rows and columns; maybe those are superfluous and should be
//   treated as blocks?

wxGridSelection::wxGridSelection( wxGrid * grid,
                                  wxGrid::wxGridSelectionModes sel )
{
    m_grid = grid;
    m_selectionMode = sel;
}

bool wxGridSelection::IsSelection()
{
  return ( m_cellSelection.GetCount() || m_blockSelectionTopLeft.GetCount() ||
           m_rowSelection.GetCount() || m_colSelection.GetCount() );
}

bool wxGridSelection::IsInSelection( int row, int col )
{
    size_t count;

    // First check whether the given cell is individually selected
    // (if m_selectionMode is wxGridSelectCells).
    if ( m_selectionMode == wxGrid::wxGridSelectCells )
    {
        count = m_cellSelection.GetCount();
        for ( size_t n = 0; n < count; n++ )
        {
            wxGridCellCoords& coords = m_cellSelection[n];
            if ( row == coords.GetRow() && col == coords.GetCol() )
                return true;
        }
    }

    // Now check whether the given cell is
    // contained in one of the selected blocks.
    count = m_blockSelectionTopLeft.GetCount();
    for ( size_t n = 0; n < count; n++ )
    {
        wxGridCellCoords& coords1 = m_blockSelectionTopLeft[n];
        wxGridCellCoords& coords2 = m_blockSelectionBottomRight[n];
        if ( BlockContainsCell(coords1.GetRow(), coords1.GetCol(),
                               coords2.GetRow(), coords2.GetCol(),
                               row, col ) )
            return true;
    }

    // Now check whether the given cell is
    // contained in one of the selected rows
    // (unless we are in column selection mode).
    if ( m_selectionMode != wxGrid::wxGridSelectColumns )
    {
        count = m_rowSelection.GetCount();
        for ( size_t n = 0; n < count; n++ )
        {
            if ( row == m_rowSelection[n] )
              return true;
        }
    }

    // Now check whether the given cell is
    // contained in one of the selected columns
    // (unless we are in row selection mode).
    if ( m_selectionMode != wxGrid::wxGridSelectRows )
    {
        count = m_colSelection.GetCount();
        for ( size_t n = 0; n < count; n++ )
        {
            if ( col == m_colSelection[n] )
              return true;
        }
    }

    return false;
}

// Change the selection mode
void wxGridSelection::SetSelectionMode( wxGrid::wxGridSelectionModes selmode )
{
    // if selection mode is unchanged return immediately
    if (selmode == m_selectionMode)
        return;

    if ( m_selectionMode != wxGrid::wxGridSelectCells )
    {
        // if changing form row to column selection
        // or vice versa, clear the selection.
        if ( selmode != wxGrid::wxGridSelectCells )
            ClearSelection();

        m_selectionMode = selmode;
    }
    else
    {
        // if changing from cell selection to something else,
        // promote selected cells/blocks to whole rows/columns.
        size_t n;
        while ( ( n = m_cellSelection.GetCount() ) > 0 )
        {
            n--;
            wxGridCellCoords& coords = m_cellSelection[n];
            int row = coords.GetRow();
            int col = coords.GetCol();
            m_cellSelection.RemoveAt(n);
            if (selmode == wxGrid::wxGridSelectRows)
                SelectRow( row );
            else // selmode == wxGridSelectColumns)
                SelectCol( col );
        }

        // Note that m_blockSelectionTopLeft's size may be changing!
        for (n = 0; n < m_blockSelectionTopLeft.GetCount(); n++)
        {
            wxGridCellCoords& coords = m_blockSelectionTopLeft[n];
            int topRow = coords.GetRow();
            int leftCol = coords.GetCol();
            coords = m_blockSelectionBottomRight[n];
            int bottomRow = coords.GetRow();
            int rightCol = coords.GetCol();

            if (selmode == wxGrid::wxGridSelectRows)
            {
                if (leftCol != 0 || rightCol != m_grid->GetNumberCols() - 1 )
                {
                    m_blockSelectionTopLeft.RemoveAt(n);
                    m_blockSelectionBottomRight.RemoveAt(n);
                    SelectBlock( topRow, 0,
                                 bottomRow, m_grid->GetNumberCols() - 1,
                                 false, false, false, false, false );
                }
            }
            else // selmode == wxGridSelectColumns)
            {
                if (topRow != 0 || bottomRow != m_grid->GetNumberRows() - 1 )
                {
                    m_blockSelectionTopLeft.RemoveAt(n);
                    m_blockSelectionBottomRight.RemoveAt(n);
                    SelectBlock( 0, leftCol,
                                 m_grid->GetNumberRows() - 1, rightCol,
                                 false, false, false, false, false );
                }
            }
        }

        m_selectionMode = selmode;
    }
}

void wxGridSelection::SelectRow( int row,
                                 bool ControlDown,  bool ShiftDown,
                                 bool AltDown, bool MetaDown )
{
    if ( m_selectionMode == wxGrid::wxGridSelectColumns )
        return;

    size_t count, n;

    // Remove single cells contained in newly selected block.
    if ( m_selectionMode == wxGrid::wxGridSelectCells )
    {
        count = m_cellSelection.GetCount();
        for ( n = 0; n < count; n++ )
        {
            wxGridCellCoords& coords = m_cellSelection[n];
            if ( BlockContainsCell( row, 0, row, m_grid->GetNumberCols() - 1,
                                    coords.GetRow(), coords.GetCol() ) )
            {
                m_cellSelection.RemoveAt(n);
                n--;
                count--;
            }
        }
    }

    // Simplify list of selected blocks (if possible)
    count = m_blockSelectionTopLeft.GetCount();
    bool done = false;

    for ( n = 0; n < count; n++ )
    {
        wxGridCellCoords& coords1 = m_blockSelectionTopLeft[n];
        wxGridCellCoords& coords2 = m_blockSelectionBottomRight[n];

        // Remove block if it is a subset of the row
        if ( coords1.GetRow() == row && row == coords2.GetRow() )
        {
            m_blockSelectionTopLeft.RemoveAt(n);
            m_blockSelectionBottomRight.RemoveAt(n);
            n--;
            count--;
        }
        else if ( coords1.GetCol() == 0  &&
                  coords2.GetCol() == m_grid->GetNumberCols() - 1 )
        {
            // silently return, if row is contained in block
            if ( coords1.GetRow() <= row && row <= coords2.GetRow() )
                return;
            // expand block, if it touched row
            else if ( coords1.GetRow() == row + 1)
            {
                coords1.SetRow(row);
                done = true;
            }
            else if ( coords2.GetRow() == row - 1)
            {
                coords2.SetRow(row);
                done = true;
            }
        }
    }

    // Unless we successfully handled the row,
    // check whether row is already selected.
    if ( !done )
    {
        count = m_rowSelection.GetCount();
        for ( n = 0; n < count; n++ )
        {
            if ( row == m_rowSelection[n] )
                return;
        }

        // Add row to selection
        m_rowSelection.Add(row);
    }

    // Update View:
    if ( !m_grid->GetBatchCount() )
    {
        wxRect r = m_grid->BlockToDeviceRect( wxGridCellCoords( row, 0 ),
                                              wxGridCellCoords( row, m_grid->GetNumberCols() - 1 ) );
        ((wxWindow *)m_grid->m_gridWin)->Refresh( false, &r );
    }

    // Send Event
    wxGridRangeSelectEvent gridEvt( m_grid->GetId(),
                                    wxEVT_GRID_RANGE_SELECT,
                                    m_grid,
                                    wxGridCellCoords( row, 0 ),
                                    wxGridCellCoords( row, m_grid->GetNumberCols() - 1 ),
                                    true,
                                    ControlDown,  ShiftDown,
                                    AltDown, MetaDown );

    m_grid->GetEventHandler()->ProcessEvent( gridEvt );
}

void wxGridSelection::SelectCol( int col,
                                 bool ControlDown,  bool ShiftDown,
                                 bool AltDown, bool MetaDown )
{
    if ( m_selectionMode == wxGrid::wxGridSelectRows )
        return;
    size_t count, n;

    // Remove single cells contained in newly selected block.
    if ( m_selectionMode == wxGrid::wxGridSelectCells )
    {
        count = m_cellSelection.GetCount();
        for ( n = 0; n < count; n++ )
        {
            wxGridCellCoords& coords = m_cellSelection[n];
            if ( BlockContainsCell( 0, col, m_grid->GetNumberRows() - 1, col,
                                    coords.GetRow(), coords.GetCol() ) )
            {
                m_cellSelection.RemoveAt(n);
                n--;
                count--;
            }
        }
    }

    // Simplify list of selected blocks (if possible)
    count = m_blockSelectionTopLeft.GetCount();
    bool done = false;
    for ( n = 0; n < count; n++ )
    {
        wxGridCellCoords& coords1 = m_blockSelectionTopLeft[n];
        wxGridCellCoords& coords2 = m_blockSelectionBottomRight[n];

        // Remove block if it is a subset of the column
        if ( coords1.GetCol() == col && col == coords2.GetCol() )
        {
            m_blockSelectionTopLeft.RemoveAt(n);
            m_blockSelectionBottomRight.RemoveAt(n);
            n--;
            count--;
        }
        else if ( coords1.GetRow() == 0  &&
                  coords2.GetRow() == m_grid->GetNumberRows() - 1 )
        {
            // silently return, if row is contained in block
            if ( coords1.GetCol() <= col && col <= coords2.GetCol() )
                return;
            // expand block, if it touched col
            else if ( coords1.GetCol() == col + 1)
            {
                coords1.SetCol(col);
                done = true;
            }
            else if ( coords2.GetCol() == col - 1)
            {
                coords2.SetCol(col);
                done = true;
            }
        }
    }

    // Unless we successfully handled the column,
    // Check whether col is already selected.
    if ( !done )
    {
        count = m_colSelection.GetCount();
        for ( n = 0; n < count; n++ )
        {
            if ( col == m_colSelection[n] )
                return;
        }

        // Add col to selection
        m_colSelection.Add(col);
    }

    // Update View:
    if ( !m_grid->GetBatchCount() )
    {
        wxRect r = m_grid->BlockToDeviceRect( wxGridCellCoords( 0, col ),
                                              wxGridCellCoords( m_grid->GetNumberRows() - 1, col ) );
        ((wxWindow *)m_grid->m_gridWin)->Refresh( false, &r );
    }

    // Send Event
    wxGridRangeSelectEvent gridEvt( m_grid->GetId(),
                                    wxEVT_GRID_RANGE_SELECT,
                                    m_grid,
                                    wxGridCellCoords( 0, col ),
                                    wxGridCellCoords( m_grid->GetNumberRows() - 1, col ),
                                    true,
                                    ControlDown,  ShiftDown,
                                    AltDown, MetaDown );

    m_grid->GetEventHandler()->ProcessEvent( gridEvt );
}

void wxGridSelection::SelectBlock( int topRow, int leftCol,
                                   int bottomRow, int rightCol,
                                   bool ControlDown, bool ShiftDown,
                                   bool AltDown, bool MetaDown,
                                   bool sendEvent )
{
    // Fix the coordinates of the block if needed.
    if ( m_selectionMode == wxGrid::wxGridSelectRows )
    {
        leftCol = 0;
        rightCol = m_grid->GetNumberCols() - 1;
    }
    else if ( m_selectionMode == wxGrid::wxGridSelectColumns )
    {
        topRow = 0;
        bottomRow = m_grid->GetNumberRows() - 1;
    }

    if ( topRow > bottomRow )
    {
        int temp = topRow;
        topRow = bottomRow;
        bottomRow = temp;
    }

    if ( leftCol > rightCol )
    {
        int temp = leftCol;
        leftCol = rightCol;
        rightCol = temp;
    }

    // Handle single cell selection in SelectCell.
    // (MB: added check for selection mode here to prevent
    //  crashes if, for example, we are select rows and the
    //  grid only has 1 col)
    if ( m_selectionMode == wxGrid::wxGridSelectCells &&
         topRow == bottomRow && leftCol == rightCol )
    {
        SelectCell( topRow, leftCol, ControlDown,  ShiftDown,
                    AltDown, MetaDown, sendEvent );
    }

    size_t count, n;

    // Remove single cells contained in newly selected block.
    if ( m_selectionMode == wxGrid::wxGridSelectCells )
    {
        count = m_cellSelection.GetCount();
        for ( n = 0; n < count; n++ )
        {
            wxGridCellCoords& coords = m_cellSelection[n];
            if ( BlockContainsCell( topRow, leftCol, bottomRow, rightCol,
                                    coords.GetRow(), coords.GetCol() ) )
            {
                m_cellSelection.RemoveAt(n);
                n--;
                count--;
            }
        }
    }

    // If a block containing the selection is already selected, return,
    // if a block contained in the selection is found, remove it.

    count = m_blockSelectionTopLeft.GetCount();
    for ( n = 0; n < count; n++ )
    {
        wxGridCellCoords& coords1 = m_blockSelectionTopLeft[n];
        wxGridCellCoords& coords2 = m_blockSelectionBottomRight[n];

        switch ( BlockContain( coords1.GetRow(), coords1.GetCol(),
                               coords2.GetRow(), coords2.GetCol(),
                               topRow, leftCol, bottomRow, rightCol ) )
        {
            case 1:
                return;

            case -1:
                m_blockSelectionTopLeft.RemoveAt(n);
                m_blockSelectionBottomRight.RemoveAt(n);
                n--;
                count--;
                break;

            default:
                break;
        }
    }

    // If a row containing the selection is already selected, return,
    // if a row contained in newly selected block is found, remove it.
    if ( m_selectionMode != wxGrid::wxGridSelectColumns )
    {
        count = m_rowSelection.GetCount();
        for ( n = 0; n < count; n++ )
        {
            switch ( BlockContain( m_rowSelection[n], 0,
                                   m_rowSelection[n], m_grid->GetNumberCols() - 1,
                                   topRow, leftCol, bottomRow, rightCol ) )
            {
                case 1:
                    return;

                case -1:
                    m_rowSelection.RemoveAt(n);
                    n--;
                    count--;
                    break;

                default:
                    break;
            }
        }
    }

    if ( m_selectionMode != wxGrid::wxGridSelectRows )
    {
        count = m_colSelection.GetCount();
        for ( n = 0; n < count; n++ )
        {
            switch ( BlockContain( 0, m_colSelection[n],
                                   m_grid->GetNumberRows() - 1, m_colSelection[n],
                                   topRow, leftCol, bottomRow, rightCol ) )
            {
                case 1:
                    return;

                case -1:
                    m_colSelection.RemoveAt(n);
                    n--;
                    count--;
                    break;

                default:
                    break;
            }
        }
    }

    m_blockSelectionTopLeft.Add( wxGridCellCoords( topRow, leftCol ) );
    m_blockSelectionBottomRight.Add( wxGridCellCoords( bottomRow, rightCol ) );

    // Update View:
    if ( !m_grid->GetBatchCount() )
    {
        wxRect r = m_grid->BlockToDeviceRect( wxGridCellCoords( topRow, leftCol ),
                                              wxGridCellCoords( bottomRow, rightCol ) );
        ((wxWindow *)m_grid->m_gridWin)->Refresh( false, &r );
    }

    // Send Event, if not disabled.
    if ( sendEvent )
    {
        wxGridRangeSelectEvent gridEvt( m_grid->GetId(),
            wxEVT_GRID_RANGE_SELECT,
            m_grid,
            wxGridCellCoords( topRow, leftCol ),
            wxGridCellCoords( bottomRow, rightCol ),
            true,
            ControlDown, ShiftDown,
            AltDown, MetaDown );
        m_grid->GetEventHandler()->ProcessEvent( gridEvt );
    }
}

void wxGridSelection::SelectCell( int row, int col,
                                  bool ControlDown, bool ShiftDown,
                                  bool AltDown, bool MetaDown,
                                  bool sendEvent )
{
    if ( m_selectionMode == wxGrid::wxGridSelectRows )
    {
        SelectBlock(row, 0, row, m_grid->GetNumberCols() - 1,
                    ControlDown, ShiftDown, AltDown, MetaDown, sendEvent);

        return;
    }
    else if ( m_selectionMode == wxGrid::wxGridSelectColumns )
    {
        SelectBlock(0, col, m_grid->GetNumberRows() - 1, col,
                    ControlDown, ShiftDown, AltDown, MetaDown, sendEvent);

        return;
    }
    else if ( IsInSelection ( row, col ) )
        return;

    m_cellSelection.Add( wxGridCellCoords( row, col ) );

    // Update View:
    if ( !m_grid->GetBatchCount() )
    {
        wxRect r = m_grid->BlockToDeviceRect(
            wxGridCellCoords( row, col ),
            wxGridCellCoords( row, col ) );
        ((wxWindow *)m_grid->m_gridWin)->Refresh( false, &r );
    }

    // Send event
    if (sendEvent)
    {
        wxGridRangeSelectEvent gridEvt( m_grid->GetId(),
            wxEVT_GRID_RANGE_SELECT,
            m_grid,
            wxGridCellCoords( row, col ),
            wxGridCellCoords( row, col ),
            true,
            ControlDown, ShiftDown,
            AltDown, MetaDown );
        m_grid->GetEventHandler()->ProcessEvent( gridEvt );
    }
}

void wxGridSelection::ToggleCellSelection( int row, int col,
                                           bool ControlDown, bool ShiftDown,
                                           bool AltDown, bool MetaDown )
{
    // if the cell is not selected, select it
    if ( !IsInSelection ( row, col ) )
    {
        SelectCell( row, col, ControlDown, ShiftDown, AltDown, MetaDown );

        return;
    }

    // otherwise deselect it. This can be simple or more or
    // less difficult, depending on how the cell is selected.
    size_t count, n;

    // The simplest case: The cell is contained in m_cellSelection
    // Then it can't be contained in rows/cols/block (since those
    // would remove the cell from m_cellSelection on creation), so
    // we just have to remove it from m_cellSelection.

    if ( m_selectionMode == wxGrid::wxGridSelectCells )
    {
        count = m_cellSelection.GetCount();
        for ( n = 0; n < count; n++ )
        {
            const wxGridCellCoords& sel = m_cellSelection[n];
            if ( row == sel.GetRow() && col == sel.GetCol() )
            {
                wxGridCellCoords coords = m_cellSelection[n];
                m_cellSelection.RemoveAt(n);
                if ( !m_grid->GetBatchCount() )
                {
                    wxRect r = m_grid->BlockToDeviceRect( coords, coords );
                    ((wxWindow *)m_grid->m_gridWin)->Refresh( false, &r );
                }

                // Send event
                wxGridRangeSelectEvent gridEvt( m_grid->GetId(),
                                                wxEVT_GRID_RANGE_SELECT,
                                                m_grid,
                                                wxGridCellCoords( row, col ),
                                                wxGridCellCoords( row, col ),
                                                false,
                                                ControlDown, ShiftDown,
                                                AltDown, MetaDown );
                m_grid->GetEventHandler()->ProcessEvent( gridEvt );

                return;
            }
        }
    }

    // The most difficult case: The cell is member of one or even several
    // blocks. Split each such block in up to 4 new parts, that don't
    // contain the cell to be selected, like this:
    // |---------------------------|
    // |                           |
    // |           part 1          |
    // |                           |
    // |---------------------------|
    // |   part 3   |x|   part 4   |
    // |---------------------------|
    // |                           |
    // |           part 2          |
    // |                           |
    // |---------------------------|
    //   (The x marks the newly deselected cell).
    // Note: in row selection mode, we only need part1 and part2;
    //       in column selection mode, we only need part 3 and part4,
    //          which are expanded to whole columns automatically!

    count = m_blockSelectionTopLeft.GetCount();
    for ( n = 0; n < count; n++ )
    {
        wxGridCellCoords& coords1 = m_blockSelectionTopLeft[n];
        wxGridCellCoords& coords2 = m_blockSelectionBottomRight[n];
        int topRow = coords1.GetRow();
        int leftCol = coords1.GetCol();
        int bottomRow = coords2.GetRow();
        int rightCol = coords2.GetCol();

        if ( BlockContainsCell( topRow, leftCol, bottomRow, rightCol, row, col ) )
        {
            // remove the block
            m_blockSelectionTopLeft.RemoveAt(n);
            m_blockSelectionBottomRight.RemoveAt(n);
            n--;
            count--;

            // add up to 4 smaller blocks and set update region
            if ( m_selectionMode != wxGrid::wxGridSelectColumns )
            {
                if ( topRow < row )
                    SelectBlock( topRow, leftCol, row - 1, rightCol,
                                 false, false, false, false, false );
                if ( bottomRow > row )
                    SelectBlock( row + 1, leftCol, bottomRow, rightCol,
                                 false, false, false, false, false );
            }

            if ( m_selectionMode != wxGrid::wxGridSelectRows )
            {
                if ( leftCol < col )
                    SelectBlock( row, leftCol, row, col - 1,
                                 false, false, false, false, false );
                if ( rightCol > col )
                    SelectBlock( row, col + 1, row, rightCol,
                                 false, false, false, false, false );
            }
        }
    }

    // remove a cell from a row, adding up to two new blocks
    if ( m_selectionMode != wxGrid::wxGridSelectColumns )
    {
        count = m_rowSelection.GetCount();
        for ( n = 0; n < count; n++ )
        {
            if ( m_rowSelection[n] == row )
            {
                m_rowSelection.RemoveAt(n);
                n--;
                count--;

                if (m_selectionMode == wxGrid::wxGridSelectCells)
                {
                    if ( col > 0 )
                        SelectBlock( row, 0, row, col - 1,
                                     false, false, false, false, false );
                    if ( col < m_grid->GetNumberCols() - 1 )
                        SelectBlock( row, col + 1,
                                     row, m_grid->GetNumberCols() - 1,
                                     false, false, false, false, false );
                }
            }
        }
    }

    // remove a cell from a column, adding up to two new blocks
    if ( m_selectionMode != wxGrid::wxGridSelectRows )
    {
        count = m_colSelection.GetCount();
        for ( n = 0; n < count; n++ )
        {
            if ( m_colSelection[n] == col )
            {
                m_colSelection.RemoveAt(n);
                n--;
                count--;

                if (m_selectionMode == wxGrid::wxGridSelectCells)
                {
                    if ( row > 0 )
                        SelectBlock( 0, col, row - 1, col,
                                     false, false, false, false, false );
                    if ( row < m_grid->GetNumberRows() - 1 )
                        SelectBlock( row + 1, col,
                                     m_grid->GetNumberRows() - 1, col,
                                     false, false, false, false, false );
                }
            }
        }
    }

    // Refresh the screen and send the event; according to m_selectionMode,
    // we need to either update only the cell, or the whole row/column.
    wxRect r;
    switch (m_selectionMode)
    {
        case wxGrid::wxGridSelectCells:
        {
            if ( !m_grid->GetBatchCount() )
            {
                r = m_grid->BlockToDeviceRect(
                    wxGridCellCoords( row, col ),
                    wxGridCellCoords( row, col ) );
                ((wxWindow *)m_grid->m_gridWin)->Refresh( false, &r );
            }

            wxGridRangeSelectEvent gridEvt( m_grid->GetId(),
                wxEVT_GRID_RANGE_SELECT,
                m_grid,
                wxGridCellCoords( row, col ),
                wxGridCellCoords( row, col ),
                false,
                ControlDown, ShiftDown,
                AltDown, MetaDown );
            m_grid->GetEventHandler()->ProcessEvent( gridEvt );
        }
            break;

        case wxGrid::wxGridSelectRows:
        {
            if ( !m_grid->GetBatchCount() )
            {
                r = m_grid->BlockToDeviceRect(
                    wxGridCellCoords( row, 0 ),
                    wxGridCellCoords( row, m_grid->GetNumberCols() - 1 ) );
                ((wxWindow *)m_grid->m_gridWin)->Refresh( false, &r );
            }

            wxGridRangeSelectEvent gridEvt( m_grid->GetId(),
                wxEVT_GRID_RANGE_SELECT,
                m_grid,
                wxGridCellCoords( row, 0 ),
                wxGridCellCoords( row, m_grid->GetNumberCols() - 1 ),
                false,
                ControlDown, ShiftDown,
                AltDown, MetaDown );
            m_grid->GetEventHandler()->ProcessEvent( gridEvt );
        }
            break;

        case wxGrid::wxGridSelectColumns:
        {
            if ( !m_grid->GetBatchCount() )
            {
                r = m_grid->BlockToDeviceRect(
                    wxGridCellCoords( 0, col ),
                    wxGridCellCoords( m_grid->GetNumberRows() - 1, col ) );
              ((wxWindow *)m_grid->m_gridWin)->Refresh( false, &r );
            }

            wxGridRangeSelectEvent gridEvt( m_grid->GetId(),
                wxEVT_GRID_RANGE_SELECT,
                m_grid,
                wxGridCellCoords( 0, col ),
                wxGridCellCoords( m_grid->GetNumberRows() - 1, col ),
                false,
                ControlDown, ShiftDown,
                AltDown, MetaDown );
            m_grid->GetEventHandler()->ProcessEvent( gridEvt );
        }
            break;

        default:
            break;
    }
}

void wxGridSelection::ClearSelection()
{
    size_t n;
    wxRect r;
    wxGridCellCoords coords1, coords2;

    // deselect all individual cells and update the screen
    if ( m_selectionMode == wxGrid::wxGridSelectCells )
    {
        while ( ( n = m_cellSelection.GetCount() ) > 0)
        {
            n--;
            coords1 = m_cellSelection[n];
            m_cellSelection.RemoveAt(n);
            if ( !m_grid->GetBatchCount() )
            {
                r = m_grid->BlockToDeviceRect( coords1, coords1 );
                ((wxWindow *)m_grid->m_gridWin)->Refresh( false, &r );

#ifdef __WXMAC__
                ((wxWindow *)m_grid->m_gridWin)->Update();
#endif
            }
        }
    }

    // deselect all blocks and update the screen
    while ( ( n = m_blockSelectionTopLeft.GetCount() ) > 0)
    {
        n--;
        coords1 = m_blockSelectionTopLeft[n];
        coords2 = m_blockSelectionBottomRight[n];
        m_blockSelectionTopLeft.RemoveAt(n);
        m_blockSelectionBottomRight.RemoveAt(n);
        if ( !m_grid->GetBatchCount() )
        {
            r = m_grid->BlockToDeviceRect( coords1, coords2 );
            ((wxWindow *)m_grid->m_gridWin)->Refresh( false, &r );

#ifdef __WXMAC__
            ((wxWindow *)m_grid->m_gridWin)->Update();
#endif
        }
    }

    // deselect all rows and update the screen
    if ( m_selectionMode != wxGrid::wxGridSelectColumns )
    {
        while ( ( n = m_rowSelection.GetCount() ) > 0)
        {
            n--;
            int row = m_rowSelection[n];
            m_rowSelection.RemoveAt(n);
            if ( !m_grid->GetBatchCount() )
            {
                r = m_grid->BlockToDeviceRect( wxGridCellCoords( row, 0 ),
                                               wxGridCellCoords( row, m_grid->GetNumberCols() - 1 ) );
                ((wxWindow *)m_grid->m_gridWin)->Refresh( false, &r );

#ifdef __WXMAC__
                ((wxWindow *)m_grid->m_gridWin)->Update();
#endif
            }
        }
    }

    // deselect all columns and update the screen
    if ( m_selectionMode != wxGrid::wxGridSelectRows )
    {
        while ( ( n = m_colSelection.GetCount() ) > 0)
        {
            n--;
            int col = m_colSelection[n];
            m_colSelection.RemoveAt(n);
            if ( !m_grid->GetBatchCount() )
            {
                r = m_grid->BlockToDeviceRect( wxGridCellCoords( 0, col ),
                                               wxGridCellCoords( m_grid->GetNumberRows() - 1, col ) );
                ((wxWindow *)m_grid->m_gridWin)->Refresh( false, &r );

#ifdef __WXMAC__
                ((wxWindow *)m_grid->m_gridWin)->Update();
#endif
            }
        }
    }

    // One deselection event, indicating deselection of _all_ cells.
    // (No finer grained events for each of the smaller regions
    //  deselected above!)
    wxGridRangeSelectEvent gridEvt( m_grid->GetId(),
                                    wxEVT_GRID_RANGE_SELECT,
                                    m_grid,
                                    wxGridCellCoords( 0, 0 ),
                                    wxGridCellCoords(
                                        m_grid->GetNumberRows() - 1,
                                        m_grid->GetNumberCols() - 1 ),
                                    false );

    m_grid->GetEventHandler()->ProcessEvent(gridEvt);
}


void wxGridSelection::UpdateRows( size_t pos, int numRows )
{
    size_t count = m_cellSelection.GetCount();
    size_t n;
    for ( n = 0; n < count; n++ )
    {
        wxGridCellCoords& coords = m_cellSelection[n];
        wxCoord row = coords.GetRow();
        if ((size_t)row >= pos)
        {
            if (numRows > 0)
            {
                // If rows inserted, increase row counter where necessary
                coords.SetRow(row + numRows);
            }
            else if (numRows < 0)
            {
                // If rows deleted ...
                if ((size_t)row >= pos - numRows)
                {
                    // ...either decrement row counter (if row still exists)...
                    coords.SetRow(row + numRows);
                }
                else
                {
                    // ...or remove the attribute
                    m_cellSelection.RemoveAt(n);
                    n--;
                    count--;
                }
            }
        }
    }

    count = m_blockSelectionTopLeft.GetCount();
    for ( n = 0; n < count; n++ )
    {
        wxGridCellCoords& coords1 = m_blockSelectionTopLeft[n];
        wxGridCellCoords& coords2 = m_blockSelectionBottomRight[n];
        wxCoord row1 = coords1.GetRow();
        wxCoord row2 = coords2.GetRow();

        if ((size_t)row2 >= pos)
        {
            if (numRows > 0)
            {
                // If rows inserted, increase row counter where necessary
                coords2.SetRow( row2 + numRows );
                if ((size_t)row1 >= pos)
                    coords1.SetRow( row1 + numRows );
            }
            else if (numRows < 0)
            {
                // If rows deleted ...
                if ((size_t)row2 >= pos - numRows)
                {
                    // ...either decrement row counter (if row still exists)...
                    coords2.SetRow( row2 + numRows );
                    if ((size_t)row1 >= pos)
                        coords1.SetRow( wxMax(row1 + numRows, (int)pos) );

                }
                else
                {
                    if ((size_t)row1 >= pos)
                    {
                        // ...or remove the attribute
                        m_blockSelectionTopLeft.RemoveAt(n);
                        m_blockSelectionBottomRight.RemoveAt(n);
                        n--;
                        count--;
                    }
                    else
                        coords2.SetRow( pos );
                }
            }
        }
    }

    count = m_rowSelection.GetCount();
    for ( n = 0; n < count; n++ )
    {
    int  rowOrCol_ = m_rowSelection[n];

      if ((size_t) rowOrCol_ >= pos)
      {
          if ( numRows > 0 )
          {
              m_rowSelection[n] += numRows;
          }
          else if ( numRows < 0 )
          {
              if ((size_t)rowOrCol_ >= (pos - numRows))
                  m_rowSelection[n] += numRows;
              else
              {
                  m_rowSelection.RemoveAt( n );
                  n--;
                  count--;
              }
          }
      }
    }
    // No need to touch selected columns, unless we removed _all_
    // rows, in this case, we remove all columns from the selection.

    if ( !m_grid->GetNumberRows() )
        m_colSelection.Clear();
}


void wxGridSelection::UpdateCols( size_t pos, int numCols )
{
    size_t count = m_cellSelection.GetCount();
    size_t n;

    for ( n = 0; n < count; n++ )
    {
        wxGridCellCoords& coords = m_cellSelection[n];
        wxCoord col = coords.GetCol();
        if ((size_t)col >= pos)
        {
            if (numCols > 0)
            {
                // If rows inserted, increase row counter where necessary
                coords.SetCol(col + numCols);
            }
            else if (numCols < 0)
            {
                // If rows deleted ...
                if ((size_t)col >= pos - numCols)
                {
                    // ...either decrement row counter (if row still exists)...
                    coords.SetCol(col + numCols);
                }
                else
                {
                    // ...or remove the attribute
                    m_cellSelection.RemoveAt(n);
                    n--;
                    count--;
                }
            }
        }
    }

    count = m_blockSelectionTopLeft.GetCount();
    for ( n = 0; n < count; n++ )
    {
        wxGridCellCoords& coords1 = m_blockSelectionTopLeft[n];
        wxGridCellCoords& coords2 = m_blockSelectionBottomRight[n];
        wxCoord col1 = coords1.GetCol();
        wxCoord col2 = coords2.GetCol();

        if ((size_t)col2 >= pos)
        {
            if (numCols > 0)
            {
                // If rows inserted, increase row counter where necessary
                coords2.SetCol(col2 + numCols);
                if ((size_t)col1 >= pos)
                    coords1.SetCol(col1 + numCols);
            }
            else if (numCols < 0)
            {
                // If cols deleted ...
                if ((size_t)col2 >= pos - numCols)
                {
                    // ...either decrement col counter (if col still exists)...
                    coords2.SetCol(col2 + numCols);
                    if ( (size_t) col1 >= pos)
                        coords1.SetCol( wxMax(col1 + numCols, (int)pos) );

                }
                else
                {
                    if ((size_t)col1 >= pos)
                    {
                        // ...or remove the attribute
                        m_blockSelectionTopLeft.RemoveAt(n);
                        m_blockSelectionBottomRight.RemoveAt(n);
                        n--;
                        count--;
                    }
                    else
                        coords2.SetCol(pos);
                }
            }
        }
    }

    count = m_colSelection.GetCount();
    for ( n = 0; n < count; n++ )
    {
        int   rowOrCol = m_colSelection[n];

        if ((size_t)rowOrCol >= pos)
        {
            if ( numCols > 0 )
                m_colSelection[n] += numCols;
            else if ( numCols < 0 )
            {
                if ((size_t)rowOrCol >= (pos - numCols))
                    m_colSelection[n] += numCols;
                else
                {
                    m_colSelection.RemoveAt( n );
                    n--;
                    count--;
                }
            }
        }
    }

    // No need to touch selected rows, unless we removed _all_
    // columns, in this case, we remove all rows from the selection.
    if ( !m_grid->GetNumberCols() )
        m_rowSelection.Clear();
}

int wxGridSelection::BlockContain( int topRow1, int leftCol1,
                                   int bottomRow1, int rightCol1,
                                   int topRow2, int leftCol2,
                                   int bottomRow2, int rightCol2 )
// returns 1, if Block1 contains Block2,
//        -1, if Block2 contains Block1,
//         0, otherwise
{
    if ( topRow1 <= topRow2 && bottomRow2 <= bottomRow1 &&
         leftCol1 <= leftCol2 && rightCol2 <= rightCol1 )
        return 1;
    else if ( topRow2 <= topRow1 && bottomRow1 <= bottomRow2 &&
              leftCol2 <= leftCol1 && rightCol1 <= rightCol2 )
        return -1;

    return 0;
}

#endif
