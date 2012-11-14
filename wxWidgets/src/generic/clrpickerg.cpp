///////////////////////////////////////////////////////////////////////////////
// Name:        src/generic/clrpickerg.cpp
// Purpose:     wxGenericColourButton class implementation
// Author:      Francesco Montorsi (readapted code written by Vadim Zeitlin)
// Modified by:
// Created:     15/04/2006
// RCS-ID:      $Id: clrpickerg.cpp 58967 2009-02-17 13:31:28Z SC $
// Copyright:   (c) Vadim Zeitlin, Francesco Montorsi
// Licence:     wxWindows licence
///////////////////////////////////////////////////////////////////////////////

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

#if wxUSE_COLOURPICKERCTRL

#include "wx/clrpicker.h"

#include "wx/colordlg.h"


// ============================================================================
// implementation
// ============================================================================

wxColourData wxGenericColourButton::ms_data;
IMPLEMENT_DYNAMIC_CLASS(wxGenericColourButton, wxCLRBTN_BASE_CLASS)

// ----------------------------------------------------------------------------
// wxGenericColourButton
// ----------------------------------------------------------------------------

bool wxGenericColourButton::Create( wxWindow *parent, wxWindowID id,
                        const wxColour &col, const wxPoint &pos,
                        const wxSize &size, long style,
                        const wxValidator& validator, const wxString &name)
{
    // create this button
#if wxCLRBTN_USES_BMP_BUTTON
    wxBitmap empty(1,1);
    if (!wxBitmapButton::Create( parent, id, empty, pos,
                           size, style, validator, name ))
#else
    if (!wxButton::Create( parent, id, wxEmptyString, pos,
                           size, style, validator, name ))
#endif
    {
        wxFAIL_MSG( wxT("wxGenericColourButton creation failed") );
        return false;
    }

    // and handle user clicks on it
    Connect(GetId(), wxEVT_COMMAND_BUTTON_CLICKED,
            wxCommandEventHandler(wxGenericColourButton::OnButtonClick),
            NULL, this);

    m_colour = col;
    UpdateColour();
    InitColourData();

    return true;
}

void wxGenericColourButton::InitColourData()
{
    ms_data.SetChooseFull(true);
    unsigned char grey = 0;
    for (int i = 0; i < 16; i++, grey += 16)
    {
        // fill with grey tones the custom colors palette
        wxColour colour(grey, grey, grey);
        ms_data.SetCustomColour(i, colour);
    }
}

void wxGenericColourButton::OnButtonClick(wxCommandEvent& WXUNUSED(ev))
{
    // update the wxColouData to be shown in the the dialog
    ms_data.SetColour(m_colour);

    // create the colour dialog and display it
    wxColourDialog dlg(this, &ms_data);
    if (dlg.ShowModal() == wxID_OK)
    {
        ms_data = dlg.GetColourData();
        SetColour(ms_data.GetColour());

        // fire an event
        wxColourPickerEvent event(this, GetId(), m_colour);
        GetEventHandler()->ProcessEvent(event);
    }
}

void wxGenericColourButton::UpdateColour()
{
    if ( !m_colour.Ok() )
    {
#if wxCLRBTN_USES_BMP_BUTTON
        wxBitmap empty(1,1);
        SetBitmapLabel(empty);
#else
        if ( HasFlag(wxCLRP_SHOW_LABEL) )
            SetLabel(wxEmptyString);
#endif
        return;
    }

    // some combinations of the fg/bg colours may be unreadable, so we invert
    // the colour to make sure fg colour is different enough from m_colour
    wxColour colFg(~m_colour.Red(), ~m_colour.Green(), ~m_colour.Blue());

#if wxCLRBTN_USES_BMP_BUTTON
    wxSize sz = GetSize();
    sz.x -= 2*GetMarginX();
    sz.y -= 2*GetMarginY();

    wxPoint topleft;
    
    if ( sz.x < 1 )
        sz.x = 1;
    else
    if ( sz.y < 1 )
        sz.y = 1;
    
    wxBitmap bmp(sz.x, sz.y);
    {
        wxMemoryDC memdc(bmp);
        memdc.SetPen(colFg);
        memdc.SetBrush(m_colour);
        memdc.DrawRectangle(topleft,sz);
        if ( HasFlag(wxCLRP_SHOW_LABEL) )
        {
            int x, y, leading, desc;
            wxString label = m_colour.GetAsString(wxC2S_HTML_SYNTAX);
            memdc.GetTextExtent(label,&x,&y,&desc,&leading);
            if ( x <= sz.x && y <= sz.y )
            {
                topleft.x += (sz.x-x)/2;
                topleft.y += (sz.y-y)/2;
                memdc.SetTextForeground(colFg);
                memdc.DrawText(label,topleft);
            }
        }
    }
    SetBitmapLabel(bmp);
#else
    SetForegroundColour(colFg);
    SetBackgroundColour(m_colour);

    if ( HasFlag(wxCLRP_SHOW_LABEL) )
        SetLabel(m_colour.GetAsString(wxC2S_HTML_SYNTAX));
#endif
}

wxSize wxGenericColourButton::DoGetBestSize() const
{
    wxSize sz(wxButton::DoGetBestSize());
    if ( HasFlag(wxCLRP_SHOW_LABEL) )
    {
#if wxCLRBTN_USES_BMP_BUTTON
        int x, y, leading, desc;
        wxString label = m_colour.GetAsString(wxC2S_HTML_SYNTAX);
        wxClientDC dc(const_cast<wxGenericColourButton*>(this));
        dc.GetTextExtent(label,&x,&y,&desc,&leading);
        sz.x = sz.y+x;
#endif
        return sz;
    }

    // if we have no label, then make this button a square
    // (like e.g. native GTK version of this control)
    sz.SetWidth(sz.GetHeight());
    return sz;
}

#endif      // wxUSE_COLOURPICKERCTRL
