/////////////////////////////////////////////////////////////////////////////
// Name:        wx/generic/gridsel.h
// Purpose:     wxGridSelection
// Author:      Stefan Neis
// Modified by:
// Created:     20/02/2000
// RCS-ID:      $$
// Copyright:   (c) Stefan Neis
// Licence:     wxWindows licence
/////////////////////////////////////////////////////////////////////////////

#ifndef _WX_GENERIC_GRIDSEL_H_
#define _WX_GENERIC_GRIDSEL_H_

#include "wx/defs.h"

#if wxUSE_GRID

#include "wx/grid.h"

class WXDLLIMPEXP_ADV wxGridSelection
{
public:
    wxGridSelection( wxGrid * grid, wxGrid::wxGridSelectionModes sel =
                     wxGrid::wxGridSelectCells );
    bool IsSelection();
    bool IsInSelection ( int row, int col );
    void SetSelectionMode(wxGrid::wxGridSelectionModes selmode);
    wxGrid::wxGridSelectionModes GetSelectionMode() { return m_selectionMode; }
    void SelectRow( int row,
                    bool ControlDown = false,  bool ShiftDown = false,
                    bool AltDown = false, bool MetaDown = false );
    void SelectCol( int col,
                    bool ControlDown = false,  bool ShiftDown = false,
                    bool AltDown = false, bool MetaDown = false );
    void SelectBlock( int topRow, int leftCol,
                      int bottomRow, int rightCol,
                      bool ControlDown = false,  bool ShiftDown = false,
                      bool AltDown = false, bool MetaDown = false,
                      bool sendEvent = true );
    void SelectCell( int row, int col,
                     bool ControlDown = false,  bool ShiftDown = false,
                     bool AltDown = false, bool MetaDown = false,
                     bool sendEvent = true );
    void ToggleCellSelection( int row, int col,
                              bool ControlDown = false,
                              bool ShiftDown = false,
                              bool AltDown = false, bool MetaDown = false );
    void ClearSelection();

    void UpdateRows( size_t pos, int numRows );
    void UpdateCols( size_t pos, int numCols );

private:
    int BlockContain( int topRow1, int leftCol1,
                       int bottomRow1, int rightCol1,
                       int topRow2, int leftCol2,
                       int bottomRow2, int rightCol2 );
      // returns 1, if Block1 contains Block2,
      //        -1, if Block2 contains Block1,
      //         0, otherwise

    int BlockContainsCell( int topRow, int leftCol,
                           int bottomRow, int rightCol,
                           int row, int col )
      // returns 1, if Block contains Cell,
      //         0, otherwise
    {
        return ( topRow <= row && row <= bottomRow &&
                 leftCol <= col && col <= rightCol );
    }

    wxGridCellCoordsArray               m_cellSelection;
    wxGridCellCoordsArray               m_blockSelectionTopLeft;
    wxGridCellCoordsArray               m_blockSelectionBottomRight;
    wxArrayInt                          m_rowSelection;
    wxArrayInt                          m_colSelection;

    wxGrid                              *m_grid;
    wxGrid::wxGridSelectionModes        m_selectionMode;

    friend class WXDLLIMPEXP_FWD_ADV wxGrid;

    DECLARE_NO_COPY_CLASS(wxGridSelection)
};

#endif  // wxUSE_GRID
#endif  // _WX_GENERIC_GRIDSEL_H_
