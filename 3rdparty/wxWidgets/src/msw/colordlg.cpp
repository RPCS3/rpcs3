/////////////////////////////////////////////////////////////////////////////
// Name:        src/msw/colordlg.cpp
// Purpose:     wxColourDialog class
// Author:      Julian Smart
// Modified by:
// Created:     01/02/97
// RCS-ID:      $Id: colordlg.cpp 49973 2007-11-15 16:44:08Z PC $
// Copyright:   (c) Julian Smart
// Licence:     wxWindows licence
/////////////////////////////////////////////////////////////////////////////

// ============================================================================
// declarations
// ============================================================================

// ----------------------------------------------------------------------------
// headers
// ----------------------------------------------------------------------------

// For compilers that support precompilation, includes "wx.h".
#include "wx/wxprec.h"

#ifdef __BORLANDC__
    #pragma hdrstop
#endif

#if wxUSE_COLOURDLG && !(defined(__SMARTPHONE__) && defined(__WXWINCE__))

#include "wx/colordlg.h"

#ifndef WX_PRECOMP
    #include "wx/msw/wrapcdlg.h"
    #include <stdio.h>
    #include "wx/colour.h"
    #include "wx/gdicmn.h"
    #include "wx/utils.h"
    #include "wx/dialog.h"
    #include "wx/cmndata.h"
    #include "wx/math.h"
#endif

#include "wx/msw/private.h"

#include <stdlib.h>
#include <string.h>

// ----------------------------------------------------------------------------
// wxWin macros
// ----------------------------------------------------------------------------

IMPLEMENT_DYNAMIC_CLASS(wxColourDialog, wxDialog)

// ============================================================================
// implementation
// ============================================================================

// ----------------------------------------------------------------------------
// colour dialog hook proc
// ----------------------------------------------------------------------------

UINT_PTR CALLBACK
wxColourDialogHookProc(HWND hwnd,
                       UINT uiMsg,
                       WPARAM WXUNUSED(wParam),
                       LPARAM lParam)
{
    if ( uiMsg == WM_INITDIALOG )
    {
        CHOOSECOLOR *pCC = (CHOOSECOLOR *)lParam;
        wxColourDialog *dialog = (wxColourDialog *)pCC->lCustData;

        const wxString title = dialog->GetTitle();
        if ( !title.empty() )
            ::SetWindowText(hwnd, title);

        wxPoint pos = dialog->GetPosition();
        if ( pos != wxDefaultPosition )
        {
            ::SetWindowPos(hwnd, NULL /* Z-order: ignored */,
                           pos.x, pos.y, -1, -1,
                           SWP_NOSIZE | SWP_NOZORDER);
        }
    }

    return 0;
}

// ----------------------------------------------------------------------------
// wxColourDialog
// ----------------------------------------------------------------------------

wxColourDialog::wxColourDialog()
{
    m_pos = wxDefaultPosition;
}

wxColourDialog::wxColourDialog(wxWindow *parent, wxColourData *data)
{
    m_pos = wxDefaultPosition;

    Create(parent, data);
}

bool wxColourDialog::Create(wxWindow *parent, wxColourData *data)
{
    m_parent = parent;
    if (data)
        m_colourData = *data;

    return true;
}

int wxColourDialog::ShowModal()
{
    CHOOSECOLOR chooseColorStruct;
    COLORREF custColours[16];
    memset(&chooseColorStruct, 0, sizeof(CHOOSECOLOR));

    int i;
    for (i = 0; i < 16; i++)
    {
        if (m_colourData.m_custColours[i].Ok())
            custColours[i] = wxColourToRGB(m_colourData.m_custColours[i]);
        else
            custColours[i] = RGB(255,255,255);
    }

    chooseColorStruct.lStructSize = sizeof(CHOOSECOLOR);
    if ( m_parent )
        chooseColorStruct.hwndOwner = GetHwndOf(m_parent);
    chooseColorStruct.rgbResult = wxColourToRGB(m_colourData.m_dataColour);
    chooseColorStruct.lpCustColors = custColours;

    chooseColorStruct.Flags = CC_RGBINIT | CC_ENABLEHOOK;
    chooseColorStruct.lCustData = (LPARAM)this;
    chooseColorStruct.lpfnHook = wxColourDialogHookProc;

    if (m_colourData.GetChooseFull())
        chooseColorStruct.Flags |= CC_FULLOPEN;

    // Do the modal dialog
    bool success = ::ChooseColor(&(chooseColorStruct)) != 0;

    // Try to highlight the correct window (the parent)
    if (GetParent())
    {
      HWND hWndParent = (HWND) GetParent()->GetHWND();
      if (hWndParent)
        ::BringWindowToTop(hWndParent);
    }


    // Restore values
    for (i = 0; i < 16; i++)
    {
      wxRGBToColour(m_colourData.m_custColours[i], custColours[i]);
    }

    wxRGBToColour(m_colourData.m_dataColour, chooseColorStruct.rgbResult);

    return success ? wxID_OK : wxID_CANCEL;
}

// ----------------------------------------------------------------------------
// title
// ----------------------------------------------------------------------------

void wxColourDialog::SetTitle(const wxString& title)
{
    m_title = title;
}

wxString wxColourDialog::GetTitle() const
{
    return m_title;
}

// ----------------------------------------------------------------------------
// position/size
// ----------------------------------------------------------------------------

void wxColourDialog::DoGetPosition(int *x, int *y) const
{
    if ( x )
        *x = m_pos.x;
    if ( y )
        *y = m_pos.y;
}

void wxColourDialog::DoSetSize(int x, int y,
                               int WXUNUSED(width), int WXUNUSED(height),
                               int WXUNUSED(sizeFlags))
{
    if ( x != wxDefaultCoord )
        m_pos.x = x;

    if ( y != wxDefaultCoord )
        m_pos.y = y;

    // ignore the size params - we can't change the size of a standard dialog
    return;
}

// NB: of course, both of these functions are completely bogus, but it's better
//     than nothing
void wxColourDialog::DoGetSize(int *width, int *height) const
{
    // the standard dialog size
    if ( width )
        *width = 225;
    if ( height )
        *height = 324;
}

void wxColourDialog::DoGetClientSize(int *width, int *height) const
{
    // the standard dialog size
    if ( width )
        *width = 219;
    if ( height )
        *height = 299;
}

#endif // wxUSE_COLOURDLG && !(__SMARTPHONE__ && __WXWINCE__)
