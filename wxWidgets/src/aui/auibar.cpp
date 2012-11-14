///////////////////////////////////////////////////////////////////////////////

// Name:        src/aui/dockart.cpp
// Purpose:     wxaui: wx advanced user interface - docking window manager
// Author:      Benjamin I. Williams
// Modified by:
// Created:     2005-05-17
// RCS-ID:      $Id: auibar.cpp 66926 2011-02-16 23:25:21Z JS $
// Copyright:   (C) Copyright 2005-2006, Kirix Corporation, All Rights Reserved
// Licence:     wxWindows Library Licence, Version 3.1
///////////////////////////////////////////////////////////////////////////////

// ============================================================================
// declarations
// ============================================================================

// ----------------------------------------------------------------------------
// headers
// ----------------------------------------------------------------------------

#include "wx/wxprec.h"

#ifdef __BORLANDC__
    #pragma hdrstop
#endif

#if wxUSE_AUI

#include "wx/statline.h"
#include "wx/dcbuffer.h"
#include "wx/sizer.h"
#include "wx/image.h"
#include "wx/settings.h"
#include "wx/menu.h"

#include "wx/aui/auibar.h"
#include "wx/aui/framemanager.h"

#ifdef __WXMAC__
#include "wx/mac/carbon/private.h"
// for themeing support
#include <Carbon/Carbon.h>
#endif

#include "wx/arrimpl.cpp"
WX_DEFINE_OBJARRAY(wxAuiToolBarItemArray)


DEFINE_EVENT_TYPE(wxEVT_COMMAND_AUITOOLBAR_TOOL_DROPDOWN)
DEFINE_EVENT_TYPE(wxEVT_COMMAND_AUITOOLBAR_OVERFLOW_CLICK)
DEFINE_EVENT_TYPE(wxEVT_COMMAND_AUITOOLBAR_RIGHT_CLICK)
DEFINE_EVENT_TYPE(wxEVT_COMMAND_AUITOOLBAR_MIDDLE_CLICK)
DEFINE_EVENT_TYPE(wxEVT_COMMAND_AUITOOLBAR_BEGIN_DRAG)


IMPLEMENT_CLASS(wxAuiToolBar, wxControl)
IMPLEMENT_DYNAMIC_CLASS(wxAuiToolBarEvent, wxEvent)


// missing wxITEM_* items
enum
{
    wxITEM_CONTROL = wxITEM_MAX,
    wxITEM_LABEL,
    wxITEM_SPACER
};

const int BUTTON_DROPDOWN_WIDTH = 10;


wxBitmap wxAuiBitmapFromBits(const unsigned char bits[], int w, int h,
                             const wxColour& color);

unsigned char wxAuiBlendColour(unsigned char fg, unsigned char bg, double alpha);
wxColor wxAuiStepColour(const wxColor& c, int percent);

static wxBitmap MakeDisabledBitmap(wxBitmap& bmp)
{
    wxImage image = bmp.ConvertToImage();

    int mr, mg, mb;
    mr = image.GetMaskRed();
    mg = image.GetMaskGreen();
    mb = image.GetMaskBlue();

    unsigned char* data = image.GetData();
    int width = image.GetWidth();
    int height = image.GetHeight();
    bool has_mask = image.HasMask();

    for (int y = height-1; y >= 0; --y)
    {
        for (int x = width-1; x >= 0; --x)
        {
            data = image.GetData() + (y*(width*3))+(x*3);
            unsigned char* r = data;
            unsigned char* g = data+1;
            unsigned char* b = data+2;

            if (has_mask && *r == mr && *g == mg && *b == mb)
                continue;

            *r = wxAuiBlendColour(*r, 255, 0.4);
            *g = wxAuiBlendColour(*g, 255, 0.4);
            *b = wxAuiBlendColour(*b, 255, 0.4);
        }
    }

    return wxBitmap(image);
}

static wxColor GetBaseColor()
{

#if defined( __WXMAC__ ) && wxOSX_USE_COCOA_OR_CARBON
    wxColor base_colour = wxColour( wxMacCreateCGColorFromHITheme(kThemeBrushToolbarBackground));
#else
    wxColor base_colour = wxSystemSettings::GetColour(wxSYS_COLOUR_3DFACE);
#endif

    // the base_colour is too pale to use as our base colour,
    // so darken it a bit --
    if ((255-base_colour.Red()) +
        (255-base_colour.Green()) +
        (255-base_colour.Blue()) < 60)
    {
        base_colour = wxAuiStepColour(base_colour, 92);
    }

    return base_colour;
}



class ToolbarCommandCapture : public wxEvtHandler
{
public:

    ToolbarCommandCapture() { m_last_id = 0; }
    int GetCommandId() const { return m_last_id; }

    bool ProcessEvent(wxEvent& evt)
    {
        if (evt.GetEventType() == wxEVT_COMMAND_MENU_SELECTED)
        {
            m_last_id = evt.GetId();
            return true;
        }

        if (GetNextHandler())
            return GetNextHandler()->ProcessEvent(evt);

        return false;
    }

private:
    int m_last_id;
};



static const unsigned char
    DISABLED_TEXT_GREY_HUE = wxAuiBlendColour(0, 255, 0.4);
const wxColour DISABLED_TEXT_COLOR(DISABLED_TEXT_GREY_HUE,
                                   DISABLED_TEXT_GREY_HUE,
                                   DISABLED_TEXT_GREY_HUE);

wxAuiDefaultToolBarArt::wxAuiDefaultToolBarArt()
{
    m_base_colour = GetBaseColor();

    m_flags = 0;
    m_text_orientation = wxAUI_TBTOOL_TEXT_BOTTOM;
    m_highlight_colour = wxSystemSettings::GetColour(wxSYS_COLOUR_HIGHLIGHT);

    m_separator_size = 7;
    m_gripper_size = 7;
    m_overflow_size = 16;

    wxColor darker1_colour = wxAuiStepColour(m_base_colour, 85);
    wxColor darker2_colour = wxAuiStepColour(m_base_colour, 75);
    wxColor darker3_colour = wxAuiStepColour(m_base_colour, 60);
    wxColor darker4_colour = wxAuiStepColour(m_base_colour, 50);
    wxColor darker5_colour = wxAuiStepColour(m_base_colour, 40);

    m_gripper_pen1 = wxPen(darker5_colour);
    m_gripper_pen2 = wxPen(darker3_colour);
    m_gripper_pen3 = *wxWHITE_PEN;

    static unsigned char button_dropdown_bits[] = { 0xe0, 0xf1, 0xfb };
    static unsigned char overflow_bits[] = { 0x80, 0xff, 0x80, 0xc1, 0xe3, 0xf7 };

    m_button_dropdown_bmp = wxAuiBitmapFromBits(button_dropdown_bits, 5, 3,
                                                *wxBLACK);
    m_disabled_button_dropdown_bmp = wxAuiBitmapFromBits(
                                                button_dropdown_bits, 5, 3,
                                                wxColor(128,128,128));
    m_overflow_bmp = wxAuiBitmapFromBits(overflow_bits, 7, 6, *wxBLACK);
    m_disabled_overflow_bmp = wxAuiBitmapFromBits(overflow_bits, 7, 6, wxColor(128,128,128));

    m_font = *wxNORMAL_FONT;
}

wxAuiDefaultToolBarArt::~wxAuiDefaultToolBarArt()
{
    m_font = *wxNORMAL_FONT;
}


wxAuiToolBarArt* wxAuiDefaultToolBarArt::Clone()
{
    return static_cast<wxAuiToolBarArt*>(new wxAuiDefaultToolBarArt);
}

void wxAuiDefaultToolBarArt::SetFlags(unsigned int flags)
{
    m_flags = flags;
}

void wxAuiDefaultToolBarArt::SetFont(const wxFont& font)
{
    m_font = font;
}

void wxAuiDefaultToolBarArt::SetTextOrientation(int orientation)
{
    m_text_orientation = orientation;
}

void wxAuiDefaultToolBarArt::DrawBackground(
                                    wxDC& dc,
                                    wxWindow* WXUNUSED(wnd),
                                    const wxRect& _rect)
{
    wxRect rect = _rect;
    rect.height++;
    wxColour start_colour = wxAuiStepColour(m_base_colour, 150);
    wxColour end_colour = wxAuiStepColour(m_base_colour, 90);
    dc.GradientFillLinear(rect, start_colour, end_colour, wxSOUTH);
}

void wxAuiDefaultToolBarArt::DrawLabel(
                                    wxDC& dc,
                                    wxWindow* WXUNUSED(wnd),
                                    const wxAuiToolBarItem& item,
                                    const wxRect& rect)
{
    dc.SetFont(m_font);
    dc.SetTextForeground(*wxBLACK);

    // we only care about the text height here since the text
    // will get cropped based on the width of the item
    int text_width = 0, text_height = 0;
    dc.GetTextExtent(wxT("ABCDHgj"), &text_width, &text_height);

    // set the clipping region
    wxRect clip_rect = rect;
    clip_rect.width -= 1;
    dc.SetClippingRegion(clip_rect);

    int text_x, text_y;
    text_x = rect.x + 1;
    text_y = rect.y + (rect.height-text_height)/2;
    dc.DrawText(item.GetLabel(), text_x, text_y);
    dc.DestroyClippingRegion();
}


void wxAuiDefaultToolBarArt::DrawButton(
                                    wxDC& dc,
                                    wxWindow* WXUNUSED(wnd),
                                    const wxAuiToolBarItem& item,
                                    const wxRect& rect)
{
    int text_width = 0, text_height = 0;

    if (m_flags & wxAUI_TB_TEXT)
    {
        dc.SetFont(m_font);

        int tx, ty;

        dc.GetTextExtent(wxT("ABCDHgj"), &tx, &text_height);
        text_width = 0;
        dc.GetTextExtent(item.GetLabel(), &text_width, &ty);
    }

    int bmp_x = 0, bmp_y = 0;
    int text_x = 0, text_y = 0;

    if (m_text_orientation == wxAUI_TBTOOL_TEXT_BOTTOM)
    {
        bmp_x = rect.x +
                (rect.width/2) -
                (item.GetBitmap().GetWidth()/2);

        bmp_y = rect.y +
                ((rect.height-text_height)/2) -
                (item.GetBitmap().GetHeight()/2);

        text_x = rect.x + (rect.width/2) - (text_width/2) + 1;
        text_y = rect.y + rect.height - text_height - 1;
    }
    else if (m_text_orientation == wxAUI_TBTOOL_TEXT_RIGHT)
    {
        bmp_x = rect.x + 3;

        bmp_y = rect.y +
                (rect.height/2) -
                (item.GetBitmap().GetHeight()/2);

        text_x = bmp_x + 3 + item.GetBitmap().GetWidth();
        text_y = rect.y +
                 (rect.height/2) -
                 (text_height/2);
    }


    if (!(item.GetState() & wxAUI_BUTTON_STATE_DISABLED))
    {
        if (item.GetState() & wxAUI_BUTTON_STATE_PRESSED)
        {
            dc.SetPen(wxPen(m_highlight_colour));
            dc.SetBrush(wxBrush(wxAuiStepColour(m_highlight_colour, 150)));
            dc.DrawRectangle(rect);
        }
        else if ((item.GetState() & wxAUI_BUTTON_STATE_HOVER) || item.IsSticky())
        {
            dc.SetPen(wxPen(m_highlight_colour));
            dc.SetBrush(wxBrush(wxAuiStepColour(m_highlight_colour, 170)));

            // draw an even lighter background for checked item hovers (since
            // the hover background is the same color as the check background)
            if (item.GetState() & wxAUI_BUTTON_STATE_CHECKED)
                dc.SetBrush(wxBrush(wxAuiStepColour(m_highlight_colour, 180)));

            dc.DrawRectangle(rect);
        }
        else if (item.GetState() & wxAUI_BUTTON_STATE_CHECKED)
        {
            // it's important to put this code in an else statment after the
            // hover, otherwise hovers won't draw properly for checked items
            dc.SetPen(wxPen(m_highlight_colour));
            dc.SetBrush(wxBrush(wxAuiStepColour(m_highlight_colour, 170)));
            dc.DrawRectangle(rect);
        }
    }

    wxBitmap bmp;
    if (item.GetState() & wxAUI_BUTTON_STATE_DISABLED)
        bmp = item.GetDisabledBitmap();
    else
        bmp = item.GetBitmap();

    if (!bmp.IsOk())
        return;

    dc.DrawBitmap(bmp, bmp_x, bmp_y, true);

    // set the item's text color based on if it is disabled
    dc.SetTextForeground(*wxBLACK);
    if (item.GetState() & wxAUI_BUTTON_STATE_DISABLED)
        dc.SetTextForeground(DISABLED_TEXT_COLOR);

    if ( (m_flags & wxAUI_TB_TEXT) && !item.GetLabel().empty() )
    {
        dc.DrawText(item.GetLabel(), text_x, text_y);
    }
}


void wxAuiDefaultToolBarArt::DrawDropDownButton(
                                    wxDC& dc,
                                    wxWindow* WXUNUSED(wnd),
                                    const wxAuiToolBarItem& item,
                                    const wxRect& rect)
{
    int text_width = 0, text_height = 0, text_x = 0, text_y = 0;
    int bmp_x = 0, bmp_y = 0, dropbmp_x = 0, dropbmp_y = 0;

    wxRect button_rect = wxRect(rect.x,
                                rect.y,
                                rect.width-BUTTON_DROPDOWN_WIDTH,
                                rect.height);
    wxRect dropdown_rect = wxRect(rect.x+rect.width-BUTTON_DROPDOWN_WIDTH-1,
                                  rect.y,
                                  BUTTON_DROPDOWN_WIDTH+1,
                                  rect.height);

    if (m_flags & wxAUI_TB_TEXT)
    {
        dc.SetFont(m_font);

        int tx, ty;
        if (m_flags & wxAUI_TB_TEXT)
        {
            dc.GetTextExtent(wxT("ABCDHgj"), &tx, &text_height);
            text_width = 0;
        }

        dc.GetTextExtent(item.GetLabel(), &text_width, &ty);
    }



    dropbmp_x = dropdown_rect.x +
                (dropdown_rect.width/2) -
                (m_button_dropdown_bmp.GetWidth()/2);
    dropbmp_y = dropdown_rect.y +
                (dropdown_rect.height/2) -
                (m_button_dropdown_bmp.GetHeight()/2);


    if (m_text_orientation == wxAUI_TBTOOL_TEXT_BOTTOM)
    {
        bmp_x = button_rect.x +
                (button_rect.width/2) -
                (item.GetBitmap().GetWidth()/2);
        bmp_y = button_rect.y +
                ((button_rect.height-text_height)/2) -
                (item.GetBitmap().GetHeight()/2);

        text_x = rect.x + (rect.width/2) - (text_width/2) + 1;
        text_y = rect.y + rect.height - text_height - 1;
    }
    else if (m_text_orientation == wxAUI_TBTOOL_TEXT_RIGHT)
    {
        bmp_x = rect.x + 3;

        bmp_y = rect.y +
                (rect.height/2) -
                (item.GetBitmap().GetHeight()/2);

        text_x = bmp_x + 3 + item.GetBitmap().GetWidth();
        text_y = rect.y +
                 (rect.height/2) -
                 (text_height/2);
    }


    if (item.GetState() & wxAUI_BUTTON_STATE_PRESSED)
    {
        dc.SetPen(wxPen(m_highlight_colour));
        dc.SetBrush(wxBrush(wxAuiStepColour(m_highlight_colour, 140)));
        dc.DrawRectangle(button_rect);
        dc.DrawRectangle(dropdown_rect);
    }
    else if (item.GetState() & wxAUI_BUTTON_STATE_HOVER ||
             item.IsSticky())
    {
        dc.SetPen(wxPen(m_highlight_colour));
        dc.SetBrush(wxBrush(wxAuiStepColour(m_highlight_colour, 170)));
        dc.DrawRectangle(button_rect);
        dc.DrawRectangle(dropdown_rect);
    }

    wxBitmap bmp;
    wxBitmap dropbmp;
    if (item.GetState() & wxAUI_BUTTON_STATE_DISABLED)
    {
        bmp = item.GetDisabledBitmap();
        dropbmp = m_disabled_button_dropdown_bmp;
    }
    else
    {
        bmp = item.GetBitmap();
        dropbmp = m_button_dropdown_bmp;
    }

    if (!bmp.IsOk())
        return;

    dc.DrawBitmap(bmp, bmp_x, bmp_y, true);
    dc.DrawBitmap(dropbmp, dropbmp_x, dropbmp_y, true);

    // set the item's text color based on if it is disabled
    dc.SetTextForeground(*wxBLACK);
    if (item.GetState() & wxAUI_BUTTON_STATE_DISABLED)
        dc.SetTextForeground(DISABLED_TEXT_COLOR);

    if ( (m_flags & wxAUI_TB_TEXT) && !item.GetLabel().empty() )
    {
        dc.DrawText(item.GetLabel(), text_x, text_y);
    }
}

void wxAuiDefaultToolBarArt::DrawControlLabel(
                                    wxDC& dc,
                                    wxWindow* WXUNUSED(wnd),
                                    const wxAuiToolBarItem& item,
                                    const wxRect& rect)
{
    if (!(m_flags & wxAUI_TB_TEXT))
        return;

    if (m_text_orientation != wxAUI_TBTOOL_TEXT_BOTTOM)
        return;

    int text_x = 0, text_y = 0;
    int text_width = 0, text_height = 0;

    dc.SetFont(m_font);

    int tx, ty;
    if (m_flags & wxAUI_TB_TEXT)
    {
        dc.GetTextExtent(wxT("ABCDHgj"), &tx, &text_height);
        text_width = 0;
    }

    dc.GetTextExtent(item.GetLabel(), &text_width, &ty);

    // don't draw the label if it is wider than the item width
    if (text_width > rect.width)
        return;

    // set the label's text color
    dc.SetTextForeground(*wxBLACK);

    text_x = rect.x + (rect.width/2) - (text_width/2) + 1;
    text_y = rect.y + rect.height - text_height - 1;

    if ( (m_flags & wxAUI_TB_TEXT) && !item.GetLabel().empty() )
    {
        dc.DrawText(item.GetLabel(), text_x, text_y);
    }
}

wxSize wxAuiDefaultToolBarArt::GetLabelSize(
                                        wxDC& dc,
                                        wxWindow* WXUNUSED(wnd),
                                        const wxAuiToolBarItem& item)
{
    dc.SetFont(m_font);

    // get label's height
    int width = 0, height = 0;
    dc.GetTextExtent(wxT("ABCDHgj"), &width, &height);

    // get item's width
    width = item.GetMinSize().GetWidth();

    if (width == -1)
    {
        // no width specified, measure the text ourselves
        width = dc.GetTextExtent(item.GetLabel()).GetX();
    }

    return wxSize(width, height);
}

wxSize wxAuiDefaultToolBarArt::GetToolSize(
                                        wxDC& dc,
                                        wxWindow* WXUNUSED(wnd),
                                        const wxAuiToolBarItem& item)
{
    if (!item.GetBitmap().IsOk() && !(m_flags & wxAUI_TB_TEXT))
        return wxSize(16,16);

    int width = item.GetBitmap().GetWidth();
    int height = item.GetBitmap().GetHeight();

    if (m_flags & wxAUI_TB_TEXT)
    {
        dc.SetFont(m_font);
        int tx, ty;

        if (m_text_orientation == wxAUI_TBTOOL_TEXT_BOTTOM)
        {
            dc.GetTextExtent(wxT("ABCDHgj"), &tx, &ty);
            height += ty;

            if ( !item.GetLabel().empty() )
            {
                dc.GetTextExtent(item.GetLabel(), &tx, &ty);
                width = wxMax(width, tx+6);
            }
        }
        else if ( m_text_orientation == wxAUI_TBTOOL_TEXT_RIGHT &&
                  !item.GetLabel().empty() )
        {
            width += 3; // space between left border and bitmap
            width += 3; // space between bitmap and text

            if ( !item.GetLabel().empty() )
            {
                dc.GetTextExtent(item.GetLabel(), &tx, &ty);
                width += tx;
                height = wxMax(height, ty);
            }
        }
    }

    // if the tool has a dropdown button, add it to the width
    if (item.HasDropDown())
        width += (BUTTON_DROPDOWN_WIDTH+4);

    return wxSize(width, height);
}

void wxAuiDefaultToolBarArt::DrawSeparator(
                                    wxDC& dc,
                                    wxWindow* WXUNUSED(wnd),
                                    const wxRect& _rect)
{
    bool horizontal = true;
    if (m_flags & wxAUI_TB_VERTICAL)
        horizontal = false;

    wxRect rect = _rect;

    if (horizontal)
    {
        rect.x += (rect.width/2);
        rect.width = 1;
        int new_height = (rect.height*3)/4;
        rect.y += (rect.height/2) - (new_height/2);
        rect.height = new_height;
    }
    else
    {
        rect.y += (rect.height/2);
        rect.height = 1;
        int new_width = (rect.width*3)/4;
        rect.x += (rect.width/2) - (new_width/2);
        rect.width = new_width;
    }

    wxColour start_colour = wxAuiStepColour(m_base_colour, 80);
    wxColour end_colour = wxAuiStepColour(m_base_colour, 80);
    dc.GradientFillLinear(rect, start_colour, end_colour, horizontal ? wxSOUTH : wxEAST);
}

void wxAuiDefaultToolBarArt::DrawGripper(wxDC& dc,
                                    wxWindow* WXUNUSED(wnd),
                                    const wxRect& rect)
{
    int i = 0;
    while (1)
    {
        int x, y;

        if (m_flags & wxAUI_TB_VERTICAL)
        {
            x = rect.x + (i*4) + 5;
            y = rect.y + 3;
            if (x > rect.GetWidth()-5)
                break;
        }
        else
        {
            x = rect.x + 3;
            y = rect.y + (i*4) + 5;
            if (y > rect.GetHeight()-5)
                break;
        }

        dc.SetPen(m_gripper_pen1);
        dc.DrawPoint(x, y);
        dc.SetPen(m_gripper_pen2);
        dc.DrawPoint(x, y+1);
        dc.DrawPoint(x+1, y);
        dc.SetPen(m_gripper_pen3);
        dc.DrawPoint(x+2, y+1);
        dc.DrawPoint(x+2, y+2);
        dc.DrawPoint(x+1, y+2);

        i++;
    }

}

void wxAuiDefaultToolBarArt::DrawOverflowButton(wxDC& dc,
                                          wxWindow* wnd,
                                          const wxRect& rect,
                                          int state)
{
    if (state & wxAUI_BUTTON_STATE_HOVER ||
        state & wxAUI_BUTTON_STATE_PRESSED)
    {
        wxRect cli_rect = wnd->GetClientRect();
        wxColor light_gray_bg = wxAuiStepColour(m_highlight_colour, 170);

        if (m_flags & wxAUI_TB_VERTICAL)
        {
            dc.SetPen(wxPen(m_highlight_colour));
            dc.DrawLine(rect.x, rect.y, rect.x+rect.width, rect.y);
            dc.SetPen(wxPen(light_gray_bg));
            dc.SetBrush(wxBrush(light_gray_bg));
            dc.DrawRectangle(rect.x, rect.y+1, rect.width, rect.height);
        }
        else
        {
            dc.SetPen(wxPen(m_highlight_colour));
            dc.DrawLine(rect.x, rect.y, rect.x, rect.y+rect.height);
            dc.SetPen(wxPen(light_gray_bg));
            dc.SetBrush(wxBrush(light_gray_bg));
            dc.DrawRectangle(rect.x+1, rect.y, rect.width, rect.height);
        }
    }

    int x = rect.x+1+(rect.width-m_overflow_bmp.GetWidth())/2;
    int y = rect.y+1+(rect.height-m_overflow_bmp.GetHeight())/2;
    dc.DrawBitmap(m_overflow_bmp, x, y, true);
}

int wxAuiDefaultToolBarArt::GetElementSize(int element_id)
{
    switch (element_id)
    {
        case wxAUI_TBART_SEPARATOR_SIZE: return m_separator_size;
        case wxAUI_TBART_GRIPPER_SIZE:   return m_gripper_size;
        case wxAUI_TBART_OVERFLOW_SIZE:  return m_overflow_size;
        default: return 0;
    }
}

void wxAuiDefaultToolBarArt::SetElementSize(int element_id, int size)
{
    switch (element_id)
    {
        case wxAUI_TBART_SEPARATOR_SIZE: m_separator_size = size; break;
        case wxAUI_TBART_GRIPPER_SIZE:   m_gripper_size = size; break;
        case wxAUI_TBART_OVERFLOW_SIZE:  m_overflow_size = size; break;
    }
}

int wxAuiDefaultToolBarArt::ShowDropDown(wxWindow* wnd,
                                         const wxAuiToolBarItemArray& items)
{
    wxMenu menuPopup;

    size_t items_added = 0;

    size_t i, count = items.GetCount();
    for (i = 0; i < count; ++i)
    {
        wxAuiToolBarItem& item = items.Item(i);

        if (item.GetKind() == wxITEM_NORMAL)
        {
            wxString text = item.GetShortHelp();
            if (text.empty())
                text = item.GetLabel();

            if (text.empty())
                text = wxT(" ");

            wxMenuItem* m =  new wxMenuItem(&menuPopup, item.GetId(), text, item.GetShortHelp());

            m->SetBitmap(item.GetBitmap());
            menuPopup.Append(m);
            items_added++;
        }
        else if (item.GetKind() == wxITEM_SEPARATOR)
        {
            if (items_added > 0)
                menuPopup.AppendSeparator();
        }
    }

    // find out where to put the popup menu of window items
    wxPoint pt = ::wxGetMousePosition();
    pt = wnd->ScreenToClient(pt);

    // find out the screen coordinate at the bottom of the tab ctrl
    wxRect cli_rect = wnd->GetClientRect();
    pt.y = cli_rect.y + cli_rect.height;

    ToolbarCommandCapture* cc = new ToolbarCommandCapture;
    wnd->PushEventHandler(cc);
    wnd->PopupMenu(&menuPopup, pt);
    int command = cc->GetCommandId();
    wnd->PopEventHandler(true);

    return command;
}




BEGIN_EVENT_TABLE(wxAuiToolBar, wxControl)
    EVT_SIZE(wxAuiToolBar::OnSize)
    EVT_IDLE(wxAuiToolBar::OnIdle)
    EVT_ERASE_BACKGROUND(wxAuiToolBar::OnEraseBackground)
    EVT_PAINT(wxAuiToolBar::OnPaint)
    EVT_LEFT_DOWN(wxAuiToolBar::OnLeftDown)
    EVT_LEFT_DCLICK(wxAuiToolBar::OnLeftDown)
    EVT_LEFT_UP(wxAuiToolBar::OnLeftUp)
    EVT_RIGHT_DOWN(wxAuiToolBar::OnRightDown)
    EVT_RIGHT_DCLICK(wxAuiToolBar::OnRightDown)
    EVT_RIGHT_UP(wxAuiToolBar::OnRightUp)
    EVT_MIDDLE_DOWN(wxAuiToolBar::OnMiddleDown)
    EVT_MIDDLE_DCLICK(wxAuiToolBar::OnMiddleDown)
    EVT_MIDDLE_UP(wxAuiToolBar::OnMiddleUp)
    EVT_MOTION(wxAuiToolBar::OnMotion)
    EVT_LEAVE_WINDOW(wxAuiToolBar::OnLeaveWindow)
    EVT_SET_CURSOR(wxAuiToolBar::OnSetCursor)
END_EVENT_TABLE()


wxAuiToolBar::wxAuiToolBar(wxWindow* parent,
                           wxWindowID id,
                           const wxPoint& position,
                           const wxSize& size,
                           long style)
                            : wxControl(parent,
                                        id,
                                        position,
                                        size,
                                        style | wxBORDER_NONE)
{
    m_sizer = new wxBoxSizer(wxHORIZONTAL);
    m_button_width = -1;
    m_button_height = -1;
    m_sizer_element_count = 0;
    m_action_pos = wxPoint(-1,-1);
    m_action_item = NULL;
    m_tip_item = NULL;
    m_art = new wxAuiDefaultToolBarArt;
    m_tool_packing = 2;
    m_tool_border_padding = 3;
    m_tool_text_orientation = wxAUI_TBTOOL_TEXT_BOTTOM;
    m_gripper_sizer_item = NULL;
    m_overflow_sizer_item = NULL;
    m_dragging = false;
    m_style = style | wxBORDER_NONE;
    m_gripper_visible = (m_style & wxAUI_TB_GRIPPER) ? true : false;
    m_overflow_visible = (m_style & wxAUI_TB_OVERFLOW) ? true : false;
    m_overflow_state = 0;
    SetMargins(5, 5, 2, 2);
    SetFont(*wxNORMAL_FONT);
    m_art->SetFlags((unsigned int)m_style);
    SetExtraStyle(wxWS_EX_PROCESS_IDLE);
    if (style & wxAUI_TB_HORZ_LAYOUT)
        SetToolTextOrientation(wxAUI_TBTOOL_TEXT_RIGHT);
}


wxAuiToolBar::~wxAuiToolBar()
{
    delete m_art;
    delete m_sizer;
}

void wxAuiToolBar::SetWindowStyleFlag(long style)
{
    wxControl::SetWindowStyleFlag(style);

    m_style = style;

    if (m_art)
    {
        m_art->SetFlags((unsigned int)m_style);
    }

    if (m_style & wxAUI_TB_GRIPPER)
        m_gripper_visible = true;
    else
        m_gripper_visible = false;


    if (m_style & wxAUI_TB_OVERFLOW)
        m_overflow_visible = true;
    else
        m_overflow_visible = false;

    if (style & wxAUI_TB_HORZ_LAYOUT)
        SetToolTextOrientation(wxAUI_TBTOOL_TEXT_RIGHT);
    else
        SetToolTextOrientation(wxAUI_TBTOOL_TEXT_BOTTOM);
}

long wxAuiToolBar::GetWindowStyleFlag() const
{
    return m_style;
}

void wxAuiToolBar::SetArtProvider(wxAuiToolBarArt* art)
{
    delete m_art;

    m_art = art;

    if (m_art)
    {
        m_art->SetFlags((unsigned int)m_style);
        m_art->SetTextOrientation(m_tool_text_orientation);
    }
}

wxAuiToolBarArt* wxAuiToolBar::GetArtProvider() const
{
    return m_art;
}




void wxAuiToolBar::AddTool(int tool_id,
                           const wxString& label,
                           const wxBitmap& bitmap,
                           const wxString& short_help_string,
                           wxItemKind kind)
{
    AddTool(tool_id,
            label,
            bitmap,
            wxNullBitmap,
            kind,
            short_help_string,
            wxEmptyString,
            NULL);
}


void wxAuiToolBar::AddTool(int tool_id,
                           const wxString& label,
                           const wxBitmap& bitmap,
                           const wxBitmap& disabled_bitmap,
                           wxItemKind kind,
                           const wxString& short_help_string,
                           const wxString& long_help_string,
                           wxObject* WXUNUSED(client_data))
{
    wxAuiToolBarItem item;
    item.window = NULL;
    item.label = label;
    item.bitmap = bitmap;
    item.disabled_bitmap = disabled_bitmap;
    item.short_help = short_help_string;
    item.long_help = long_help_string;
    item.active = true;
    item.dropdown = false;
    item.spacer_pixels = 0;
    item.id = tool_id;
    item.state = 0;
    item.proportion = 0;
    item.kind = kind;
    item.sizer_item = NULL;
    item.min_size = wxDefaultSize;
    item.user_data = 0;
    item.sticky = false;

    if (item.id == wxID_ANY)
        item.id = wxNewId();

    if (!item.disabled_bitmap.IsOk())
    {
        // no disabled bitmap specified, we need to make one
        if (item.bitmap.IsOk())
        {
            //wxImage img = item.bitmap.ConvertToImage();
            //wxImage grey_version = img.ConvertToGreyscale();
            //item.disabled_bitmap = wxBitmap(grey_version);
            item.disabled_bitmap = MakeDisabledBitmap(item.bitmap);
        }
    }

    m_items.Add(item);
}

void wxAuiToolBar::AddControl(wxControl* control,
                              const wxString& label)
{
    wxAuiToolBarItem item;
    item.window = (wxWindow*)control;
    item.label = label;
    item.bitmap = wxNullBitmap;
    item.disabled_bitmap = wxNullBitmap;
    item.active = true;
    item.dropdown = false;
    item.spacer_pixels = 0;
    item.id = control->GetId();
    item.state = 0;
    item.proportion = 0;
    item.kind = wxITEM_CONTROL;
    item.sizer_item = NULL;
    item.min_size = control->GetEffectiveMinSize();
    item.user_data = 0;
    item.sticky = false;

    m_items.Add(item);
}

void wxAuiToolBar::AddLabel(int tool_id,
                            const wxString& label,
                            const int width)
{
    wxSize min_size = wxDefaultSize;
    if (width != -1)
        min_size.x = width;

    wxAuiToolBarItem item;
    item.window = NULL;
    item.label = label;
    item.bitmap = wxNullBitmap;
    item.disabled_bitmap = wxNullBitmap;
    item.active = true;
    item.dropdown = false;
    item.spacer_pixels = 0;
    item.id = tool_id;
    item.state = 0;
    item.proportion = 0;
    item.kind = wxITEM_LABEL;
    item.sizer_item = NULL;
    item.min_size = min_size;
    item.user_data = 0;
    item.sticky = false;

    if (item.id == wxID_ANY)
        item.id = wxNewId();

    m_items.Add(item);
}

void wxAuiToolBar::AddSeparator()
{
    wxAuiToolBarItem item;
    item.window = NULL;
    item.label = wxEmptyString;
    item.bitmap = wxNullBitmap;
    item.disabled_bitmap = wxNullBitmap;
    item.active = true;
    item.dropdown = false;
    item.id = -1;
    item.state = 0;
    item.proportion = 0;
    item.kind = wxITEM_SEPARATOR;
    item.sizer_item = NULL;
    item.min_size = wxDefaultSize;
    item.user_data = 0;
    item.sticky = false;

    m_items.Add(item);
}

void wxAuiToolBar::AddSpacer(int pixels)
{
    wxAuiToolBarItem item;
    item.window = NULL;
    item.label = wxEmptyString;
    item.bitmap = wxNullBitmap;
    item.disabled_bitmap = wxNullBitmap;
    item.active = true;
    item.dropdown = false;
    item.spacer_pixels = pixels;
    item.id = -1;
    item.state = 0;
    item.proportion = 0;
    item.kind = wxITEM_SPACER;
    item.sizer_item = NULL;
    item.min_size = wxDefaultSize;
    item.user_data = 0;
    item.sticky = false;

    m_items.Add(item);
}

void wxAuiToolBar::AddStretchSpacer(int proportion)
{
    wxAuiToolBarItem item;
    item.window = NULL;
    item.label = wxEmptyString;
    item.bitmap = wxNullBitmap;
    item.disabled_bitmap = wxNullBitmap;
    item.active = true;
    item.dropdown = false;
    item.spacer_pixels = 0;
    item.id = -1;
    item.state = 0;
    item.proportion = proportion;
    item.kind = wxITEM_SPACER;
    item.sizer_item = NULL;
    item.min_size = wxDefaultSize;
    item.user_data = 0;
    item.sticky = false;

    m_items.Add(item);
}

void wxAuiToolBar::Clear()
{
    m_items.Clear();
    m_sizer_element_count = 0;
}

bool wxAuiToolBar::DeleteTool(int tool_id)
{
    int idx = GetToolIndex(tool_id);
    if (idx >= 0 && idx < (int)m_items.GetCount())
    {
        m_items.RemoveAt(idx);
        Realize();
        return true;
    }

    return false;
}

bool wxAuiToolBar::DeleteByIndex(int idx)
{
    if (idx >= 0 && idx < (int)m_items.GetCount())
    {
        m_items.RemoveAt(idx);
        Realize();
        return true;
    }

    return false;
}


wxControl* wxAuiToolBar::FindControl(int id)
{
    wxWindow* wnd = FindWindow(id);
    return (wxControl*)wnd;
}

wxAuiToolBarItem* wxAuiToolBar::FindTool(int tool_id) const
{
    size_t i, count;
    for (i = 0, count = m_items.GetCount(); i < count; ++i)
    {
        wxAuiToolBarItem& item = m_items.Item(i);
        if (item.id == tool_id)
            return &item;
    }

    return NULL;
}

wxAuiToolBarItem* wxAuiToolBar::FindToolByPosition(wxCoord x, wxCoord y) const
{
    size_t i, count;
    for (i = 0, count = m_items.GetCount(); i < count; ++i)
    {
        wxAuiToolBarItem& item = m_items.Item(i);

        if (!item.sizer_item)
            continue;

        wxRect rect = item.sizer_item->GetRect();
        if (rect.Contains(x,y))
        {
            // if the item doesn't fit on the toolbar, return NULL
            if (!GetToolFitsByIndex(i))
                return NULL;

            return &item;
        }
    }

    return NULL;
}

wxAuiToolBarItem* wxAuiToolBar::FindToolByPositionWithPacking(wxCoord x, wxCoord y) const
{
    size_t i, count;
    for (i = 0, count = m_items.GetCount(); i < count; ++i)
    {
        wxAuiToolBarItem& item = m_items.Item(i);

        if (!item.sizer_item)
            continue;

        wxRect rect = item.sizer_item->GetRect();

        // apply tool packing
        if (i+1 < count)
            rect.width += m_tool_packing;

        if (rect.Contains(x,y))
        {
            // if the item doesn't fit on the toolbar, return NULL
            if (!GetToolFitsByIndex(i))
                return NULL;

            return &item;
        }
    }

    return NULL;
}

wxAuiToolBarItem* wxAuiToolBar::FindToolByIndex(int idx) const
{
    if (idx < 0)
        return NULL;

    if (idx >= (int)m_items.size())
        return NULL;

    return &(m_items[idx]);
}

void wxAuiToolBar::SetToolBitmapSize(const wxSize& WXUNUSED(size))
{
    // TODO: wxToolBar compatibility
}

wxSize wxAuiToolBar::GetToolBitmapSize() const
{
    // TODO: wxToolBar compatibility
    return wxSize(16,15);
}

void wxAuiToolBar::SetToolProportion(int tool_id, int proportion)
{
    wxAuiToolBarItem* item = FindTool(tool_id);
    if (!item)
        return;

    item->proportion = proportion;
}

int wxAuiToolBar::GetToolProportion(int tool_id) const
{
    wxAuiToolBarItem* item = FindTool(tool_id);
    if (!item)
        return 0;

    return item->proportion;
}

void wxAuiToolBar::SetToolSeparation(int separation)
{
    if (m_art)
        m_art->SetElementSize(wxAUI_TBART_SEPARATOR_SIZE, separation);
}

int wxAuiToolBar::GetToolSeparation() const
{
    if (m_art)
        return m_art->GetElementSize(wxAUI_TBART_SEPARATOR_SIZE);
    else
        return 5;
}


void wxAuiToolBar::SetToolDropDown(int tool_id, bool dropdown)
{
    wxAuiToolBarItem* item = FindTool(tool_id);
    if (!item)
        return;

    item->dropdown = dropdown;
}

bool wxAuiToolBar::GetToolDropDown(int tool_id) const
{
    wxAuiToolBarItem* item = FindTool(tool_id);
    if (!item)
        return 0;

    return item->dropdown;
}

void wxAuiToolBar::SetToolSticky(int tool_id, bool sticky)
{
    // ignore separators
    if (tool_id == -1)
        return;

    wxAuiToolBarItem* item = FindTool(tool_id);
    if (!item)
        return;

    if (item->sticky == sticky)
        return;

    item->sticky = sticky;

    Refresh(false);
    Update();
}

bool wxAuiToolBar::GetToolSticky(int tool_id) const
{
    wxAuiToolBarItem* item = FindTool(tool_id);
    if (!item)
        return 0;

    return item->sticky;
}




void wxAuiToolBar::SetToolBorderPadding(int padding)
{
    m_tool_border_padding = padding;
}

int wxAuiToolBar::GetToolBorderPadding() const
{
    return m_tool_border_padding;
}

void wxAuiToolBar::SetToolTextOrientation(int orientation)
{
    m_tool_text_orientation = orientation;

    if (m_art)
    {
        m_art->SetTextOrientation(orientation);
    }
}

int wxAuiToolBar::GetToolTextOrientation() const
{
    return m_tool_text_orientation;
}

void wxAuiToolBar::SetToolPacking(int packing)
{
    m_tool_packing = packing;
}

int wxAuiToolBar::GetToolPacking() const
{
    return m_tool_packing;
}


void wxAuiToolBar::SetOrientation(int WXUNUSED(orientation))
{
}

void wxAuiToolBar::SetMargins(int left, int right, int top, int bottom)
{
    if (left != -1)
        m_left_padding = left;
    if (right != -1)
        m_right_padding = right;
    if (top != -1)
        m_top_padding = top;
    if (bottom != -1)
        m_bottom_padding = bottom;
}

bool wxAuiToolBar::GetGripperVisible() const
{
    return m_gripper_visible;
}

void wxAuiToolBar::SetGripperVisible(bool visible)
{
    m_gripper_visible = visible;
    if (visible)
        m_style |= wxAUI_TB_GRIPPER;
    else
        m_style &= ~wxAUI_TB_GRIPPER;
    Realize();
    Refresh(false);
}


bool wxAuiToolBar::GetOverflowVisible() const
{
    return m_overflow_visible;
}

void wxAuiToolBar::SetOverflowVisible(bool visible)
{
    m_overflow_visible = visible;
    if (visible)
        m_style |= wxAUI_TB_OVERFLOW;
    else
        m_style &= ~wxAUI_TB_OVERFLOW;
    Refresh(false);
}

bool wxAuiToolBar::SetFont(const wxFont& font)
{
    bool res = wxWindow::SetFont(font);

    if (m_art)
    {
        m_art->SetFont(font);
    }

    return res;
}


void wxAuiToolBar::SetHoverItem(wxAuiToolBarItem* pitem)
{
    wxAuiToolBarItem* former_hover = NULL;

    size_t i, count;
    for (i = 0, count = m_items.GetCount(); i < count; ++i)
    {
        wxAuiToolBarItem& item = m_items.Item(i);
        if (item.state & wxAUI_BUTTON_STATE_HOVER)
            former_hover = &item;
        item.state &= ~wxAUI_BUTTON_STATE_HOVER;
    }

    if (pitem)
    {
        pitem->state |= wxAUI_BUTTON_STATE_HOVER;
    }

    if (former_hover != pitem)
    {
        Refresh(false);
        Update();
    }
}

void wxAuiToolBar::SetPressedItem(wxAuiToolBarItem* pitem)
{
    wxAuiToolBarItem* former_item = NULL;

    size_t i, count;
    for (i = 0, count = m_items.GetCount(); i < count; ++i)
    {
        wxAuiToolBarItem& item = m_items.Item(i);
        if (item.state & wxAUI_BUTTON_STATE_PRESSED)
            former_item = &item;
        item.state &= ~wxAUI_BUTTON_STATE_PRESSED;
    }

    if (pitem)
    {
        pitem->state &= ~wxAUI_BUTTON_STATE_HOVER;
        pitem->state |= wxAUI_BUTTON_STATE_PRESSED;
    }

    if (former_item != pitem)
    {
        Refresh(false);
        Update();
    }
}

void wxAuiToolBar::RefreshOverflowState()
{
    if (!m_overflow_sizer_item)
    {
        m_overflow_state = 0;
        return;
    }

    int overflow_state = 0;

    wxRect overflow_rect = GetOverflowRect();


    // find out the mouse's current position
    wxPoint pt = ::wxGetMousePosition();
    pt = this->ScreenToClient(pt);

    // find out if the mouse cursor is inside the dropdown rectangle
    if (overflow_rect.Contains(pt.x, pt.y))
    {
        if (::wxGetMouseState().LeftDown())
            overflow_state = wxAUI_BUTTON_STATE_PRESSED;
        else
            overflow_state = wxAUI_BUTTON_STATE_HOVER;
    }

    if (overflow_state != m_overflow_state)
    {
        m_overflow_state = overflow_state;
        Refresh(false);
        Update();
    }

    m_overflow_state = overflow_state;
}

void wxAuiToolBar::ToggleTool(int tool_id, bool state)
{
    wxAuiToolBarItem* tool = FindTool(tool_id);

    if (tool && (tool->kind == wxITEM_CHECK || tool->kind == wxITEM_RADIO))
    {
        if (tool->kind == wxITEM_RADIO)
        {
            int i, idx, count;
            idx = GetToolIndex(tool_id);
            count = (int)m_items.GetCount();

            if (idx >= 0 && idx < count)
            {
                for (i = idx; i < count; ++i)
                {
                    if (m_items[i].kind != wxITEM_RADIO)
                        break;
                    m_items[i].state &= ~wxAUI_BUTTON_STATE_CHECKED;
                }
                for (i = idx; i > 0; i--)
                {
                    if (m_items[i].kind != wxITEM_RADIO)
                        break;
                    m_items[i].state &= ~wxAUI_BUTTON_STATE_CHECKED;
                }
            }

            tool->state |= wxAUI_BUTTON_STATE_CHECKED;
        }
         else if (tool->kind == wxITEM_CHECK)
        {
            if (state == true)
                tool->state |= wxAUI_BUTTON_STATE_CHECKED;
            else
                tool->state &= ~wxAUI_BUTTON_STATE_CHECKED;
        }
    }
}

bool wxAuiToolBar::GetToolToggled(int tool_id) const
{
    wxAuiToolBarItem* tool = FindTool(tool_id);

    if (tool)
    {
        if ( (tool->kind != wxITEM_CHECK) && (tool->kind != wxITEM_RADIO) )
            return false;

        return (tool->state & wxAUI_BUTTON_STATE_CHECKED) ? true : false;
    }

    return false;
}

void wxAuiToolBar::EnableTool(int tool_id, bool state)
{
    wxAuiToolBarItem* tool = FindTool(tool_id);

    if (tool)
    {
        if (state == true)
            tool->state &= ~wxAUI_BUTTON_STATE_DISABLED;
        else
            tool->state |= wxAUI_BUTTON_STATE_DISABLED;
    }
}

bool wxAuiToolBar::GetToolEnabled(int tool_id) const
{
    wxAuiToolBarItem* tool = FindTool(tool_id);

    if (tool)
        return (tool->state & wxAUI_BUTTON_STATE_DISABLED) ? false : true;

    return false;
}

wxString wxAuiToolBar::GetToolLabel(int tool_id) const
{
    wxAuiToolBarItem* tool = FindTool(tool_id);
    wxASSERT_MSG(tool, wxT("can't find tool in toolbar item array"));
    if (!tool)
        return wxEmptyString;

    return tool->label;
}

void wxAuiToolBar::SetToolLabel(int tool_id, const wxString& label)
{
    wxAuiToolBarItem* tool = FindTool(tool_id);
    if (tool)
    {
        tool->label = label;
    }
}

wxBitmap wxAuiToolBar::GetToolBitmap(int tool_id) const
{
    wxAuiToolBarItem* tool = FindTool(tool_id);
    wxASSERT_MSG(tool, wxT("can't find tool in toolbar item array"));
    if (!tool)
        return wxNullBitmap;

    return tool->bitmap;
}

void wxAuiToolBar::SetToolBitmap(int tool_id, const wxBitmap& bitmap)
{
    wxAuiToolBarItem* tool = FindTool(tool_id);
    if (tool)
    {
        tool->bitmap = bitmap;
    }
}

wxString wxAuiToolBar::GetToolShortHelp(int tool_id) const
{
    wxAuiToolBarItem* tool = FindTool(tool_id);
    wxASSERT_MSG(tool, wxT("can't find tool in toolbar item array"));
    if (!tool)
        return wxEmptyString;

    return tool->short_help;
}

void wxAuiToolBar::SetToolShortHelp(int tool_id, const wxString& help_string)
{
    wxAuiToolBarItem* tool = FindTool(tool_id);
    if (tool)
    {
        tool->short_help = help_string;
    }
}

wxString wxAuiToolBar::GetToolLongHelp(int tool_id) const
{
    wxAuiToolBarItem* tool = FindTool(tool_id);
    wxASSERT_MSG(tool, wxT("can't find tool in toolbar item array"));
    if (!tool)
        return wxEmptyString;

    return tool->long_help;
}

void wxAuiToolBar::SetToolLongHelp(int tool_id, const wxString& help_string)
{
    wxAuiToolBarItem* tool = FindTool(tool_id);
    if (tool)
    {
        tool->long_help = help_string;
    }
}

void wxAuiToolBar::SetCustomOverflowItems(const wxAuiToolBarItemArray& prepend,
                                          const wxAuiToolBarItemArray& append)
{
    m_custom_overflow_prepend = prepend;
    m_custom_overflow_append = append;
}


size_t wxAuiToolBar::GetToolCount() const
{
    return m_items.size();
}

int wxAuiToolBar::GetToolIndex(int tool_id) const
{
    // this will prevent us from returning the index of the
    // first separator in the toolbar since its id is equal to -1
    if (tool_id == -1)
        return wxNOT_FOUND;

    size_t i, count = m_items.GetCount();
    for (i = 0; i < count; ++i)
    {
        wxAuiToolBarItem& item = m_items.Item(i);
        if (item.id == tool_id)
            return i;
    }

    return wxNOT_FOUND;
}

bool wxAuiToolBar::GetToolFitsByIndex(int tool_idx) const
{
    if (tool_idx < 0 || tool_idx >= (int)m_items.GetCount())
        return false;

    if (!m_items[tool_idx].sizer_item)
        return false;

    int cli_w, cli_h;
    GetClientSize(&cli_w, &cli_h);

    wxRect rect = m_items[tool_idx].sizer_item->GetRect();

    if (m_style & wxAUI_TB_VERTICAL)
    {
        // take the dropdown size into account
        if (m_overflow_visible)
            cli_h -= m_overflow_sizer_item->GetSize().y;

        if (rect.y+rect.height < cli_h)
            return true;
    }
    else
    {
        // take the dropdown size into account
        if (m_overflow_visible)
            cli_w -= m_overflow_sizer_item->GetSize().x;

        if (rect.x+rect.width < cli_w)
            return true;
    }

    return false;
}


bool wxAuiToolBar::GetToolFits(int tool_id) const
{
    return GetToolFitsByIndex(GetToolIndex(tool_id));
}

wxRect wxAuiToolBar::GetToolRect(int tool_id) const
{
    wxAuiToolBarItem* tool = FindTool(tool_id);
    if (tool && tool->sizer_item)
    {
        return tool->sizer_item->GetRect();
    }

    return wxRect();
}

bool wxAuiToolBar::GetToolBarFits() const
{
    if (m_items.GetCount() == 0)
    {
        // empty toolbar always 'fits'
        return true;
    }

    // entire toolbar content fits if the last tool fits
    return GetToolFitsByIndex(m_items.GetCount() - 1);
}

bool wxAuiToolBar::Realize()
{
    wxClientDC dc(this);
    if (!dc.IsOk())
        return false;

    bool horizontal = true;
    if (m_style & wxAUI_TB_VERTICAL)
        horizontal = false;


    // create the new sizer to add toolbar elements to
    wxBoxSizer* sizer = new wxBoxSizer(horizontal ? wxHORIZONTAL : wxVERTICAL);

    // add gripper area
    int separator_size = m_art->GetElementSize(wxAUI_TBART_SEPARATOR_SIZE);
    int gripper_size = m_art->GetElementSize(wxAUI_TBART_GRIPPER_SIZE);
    if (gripper_size > 0 && m_gripper_visible)
    {
        if (horizontal)
            m_gripper_sizer_item = sizer->Add(gripper_size, 1, 0, wxEXPAND);
        else
            m_gripper_sizer_item = sizer->Add(1, gripper_size, 0, wxEXPAND);
    }
    else
    {
        m_gripper_sizer_item = NULL;
    }

    // add "left" padding
    if (m_left_padding > 0)
    {
        if (horizontal)
            sizer->Add(m_left_padding, 1);
        else
            sizer->Add(1, m_left_padding);
    }

    size_t i, count;
    for (i = 0, count = m_items.GetCount(); i < count; ++i)
    {
        wxAuiToolBarItem& item = m_items.Item(i);
        wxSizerItem* sizer_item = NULL;

        switch (item.kind)
        {
            case wxITEM_LABEL:
            {
                wxSize size = m_art->GetLabelSize(dc, this, item);
                sizer_item = sizer->Add(size.x + (m_tool_border_padding*2),
                                        size.y + (m_tool_border_padding*2),
                                        item.proportion,
                                        wxALIGN_CENTER);
                if (i+1 < count)
                {
                    sizer->AddSpacer(m_tool_packing);
                }

                break;
            }

            case wxITEM_CHECK:
            case wxITEM_NORMAL:
            case wxITEM_RADIO:
            {
                wxSize size = m_art->GetToolSize(dc, this, item);
                sizer_item = sizer->Add(size.x + (m_tool_border_padding*2),
                                        size.y + (m_tool_border_padding*2),
                                        0,
                                        wxALIGN_CENTER);
                // add tool packing
                if (i+1 < count)
                {
                    sizer->AddSpacer(m_tool_packing);
                }

                break;
            }

            case wxITEM_SEPARATOR:
            {
                if (horizontal)
                    sizer_item = sizer->Add(separator_size, 1, 0, wxEXPAND);
                else
                    sizer_item = sizer->Add(1, separator_size, 0, wxEXPAND);

                // add tool packing
                if (i+1 < count)
                {
                    sizer->AddSpacer(m_tool_packing);
                }

                break;
            }

            case wxITEM_SPACER:
                if (item.proportion > 0)
                    sizer_item = sizer->AddStretchSpacer(item.proportion);
                else
                    sizer_item = sizer->Add(item.spacer_pixels, 1);
                break;

            case wxITEM_CONTROL:
            {
                //sizer_item = sizer->Add(item.window, item.proportion, wxEXPAND);
                wxSizerItem* ctrl_sizer_item;

                wxBoxSizer* vert_sizer = new wxBoxSizer(wxVERTICAL);
                vert_sizer->AddStretchSpacer(1);
                ctrl_sizer_item = vert_sizer->Add(item.window, 0, wxEXPAND);
                vert_sizer->AddStretchSpacer(1);
                if ( (m_style & wxAUI_TB_TEXT) &&
                     m_tool_text_orientation == wxAUI_TBTOOL_TEXT_BOTTOM &&
                     !item.GetLabel().empty() )
                {
                    wxSize s = GetLabelSize(item.GetLabel());
                    vert_sizer->Add(1, s.y);
                }


                sizer_item = sizer->Add(vert_sizer, item.proportion, wxEXPAND);

                wxSize min_size = item.min_size;


                // proportional items will disappear from the toolbar if
                // their min width is not set to something really small
                if (item.proportion != 0)
                {
                    min_size.x = 1;
                }

                if (min_size.IsFullySpecified())
                {
                    sizer_item->SetMinSize(min_size);
                    ctrl_sizer_item->SetMinSize(min_size);
                }

                // add tool packing
                if (i+1 < count)
                {
                    sizer->AddSpacer(m_tool_packing);
                }
            }
        }

        item.sizer_item = sizer_item;
    }

    // add "right" padding
    if (m_right_padding > 0)
    {
        if (horizontal)
            sizer->Add(m_right_padding, 1);
        else
            sizer->Add(1, m_right_padding);
    }

    // add drop down area
    m_overflow_sizer_item = NULL;

    if (m_style & wxAUI_TB_OVERFLOW)
    {
        int overflow_size = m_art->GetElementSize(wxAUI_TBART_OVERFLOW_SIZE);
        if (overflow_size > 0 && m_overflow_visible)
        {
            if (horizontal)
                m_overflow_sizer_item = sizer->Add(overflow_size, 1, 0, wxEXPAND);
            else
                m_overflow_sizer_item = sizer->Add(1, overflow_size, 0, wxEXPAND);
        }
        else
        {
            m_overflow_sizer_item = NULL;
        }
    }


    // the outside sizer helps us apply the "top" and "bottom" padding
    wxBoxSizer* outside_sizer = new wxBoxSizer(horizontal ? wxVERTICAL : wxHORIZONTAL);

    // add "top" padding
    if (m_top_padding > 0)
    {
        if (horizontal)
            outside_sizer->Add(1, m_top_padding);
        else
            outside_sizer->Add(m_top_padding, 1);
    }

    // add the sizer that contains all of the toolbar elements
    outside_sizer->Add(sizer, 1, wxEXPAND);

    // add "bottom" padding
    if (m_bottom_padding > 0)
    {
        if (horizontal)
            outside_sizer->Add(1, m_bottom_padding);
        else
            outside_sizer->Add(m_bottom_padding, 1);
    }

    delete m_sizer; // remove old sizer
    m_sizer = outside_sizer;

    // calculate the rock-bottom minimum size
    for (i = 0, count = m_items.GetCount(); i < count; ++i)
    {
        wxAuiToolBarItem& item = m_items.Item(i);
        if (item.sizer_item && item.proportion > 0 && item.min_size.IsFullySpecified())
            item.sizer_item->SetMinSize(0,0);
    }

    m_absolute_min_size = m_sizer->GetMinSize();

    // reset the min sizes to what they were
    for (i = 0, count = m_items.GetCount(); i < count; ++i)
    {
        wxAuiToolBarItem& item = m_items.Item(i);
        if (item.sizer_item && item.proportion > 0 && item.min_size.IsFullySpecified())
            item.sizer_item->SetMinSize(item.min_size);
    }

    // set control size
    wxSize size = m_sizer->GetMinSize();
    m_minWidth = size.x;
    m_minHeight = size.y;

    if ((m_style & wxAUI_TB_NO_AUTORESIZE) == 0)
    {
        wxSize cur_size = GetClientSize();
        wxSize new_size = GetMinSize();
        if (new_size != cur_size)
        {
            SetClientSize(new_size);
        }
        else
        {
            m_sizer->SetDimension(0, 0, cur_size.x, cur_size.y);
        }
    }
    else
    {
        wxSize cur_size = GetClientSize();
        m_sizer->SetDimension(0, 0, cur_size.x, cur_size.y);
    }

    Refresh(false);
    return true;
}

int wxAuiToolBar::GetOverflowState() const
{
    return m_overflow_state;
}

wxRect wxAuiToolBar::GetOverflowRect() const
{
    wxRect cli_rect(wxPoint(0,0), GetClientSize());
    wxRect overflow_rect = m_overflow_sizer_item->GetRect();
    int overflow_size = m_art->GetElementSize(wxAUI_TBART_OVERFLOW_SIZE);

    if (m_style & wxAUI_TB_VERTICAL)
    {
        overflow_rect.y = cli_rect.height - overflow_size;
        overflow_rect.x = 0;
        overflow_rect.width = cli_rect.width;
        overflow_rect.height = overflow_size;
    }
    else
    {
        overflow_rect.x = cli_rect.width - overflow_size;
        overflow_rect.y = 0;
        overflow_rect.width = overflow_size;
        overflow_rect.height = cli_rect.height;
    }

    return overflow_rect;
}

wxSize wxAuiToolBar::GetLabelSize(const wxString& label)
{
    wxClientDC dc(this);

    int tx, ty;
    int text_width = 0, text_height = 0;

    dc.SetFont(m_font);

    // get the text height
    dc.GetTextExtent(wxT("ABCDHgj"), &tx, &text_height);

    // get the text width
    dc.GetTextExtent(label, &text_width, &ty);

    return wxSize(text_width, text_height);
}


void wxAuiToolBar::DoIdleUpdate()
{
    wxEvtHandler* handler = GetEventHandler();

    bool need_refresh = false;

    size_t i, count;
    for (i = 0, count = m_items.GetCount(); i < count; ++i)
    {
        wxAuiToolBarItem& item = m_items.Item(i);

        if (item.id == -1)
            continue;

        wxUpdateUIEvent evt(item.id);
        evt.SetEventObject(this);

        if (handler->ProcessEvent(evt))
        {
            if (evt.GetSetEnabled())
            {
                bool is_enabled;
                if (item.window)
                    is_enabled = item.window->IsEnabled();
                else
                    is_enabled = (item.state & wxAUI_BUTTON_STATE_DISABLED) ? false : true;

                bool new_enabled = evt.GetEnabled();
                if (new_enabled != is_enabled)
                {
                    if (item.window)
                    {
                        item.window->Enable(new_enabled);
                    }
                    else
                    {
                        if (new_enabled)
                            item.state &= ~wxAUI_BUTTON_STATE_DISABLED;
                        else
                            item.state |= wxAUI_BUTTON_STATE_DISABLED;
                    }
                    need_refresh = true;
                }
            }

            if (evt.GetSetChecked())
            {
                // make sure we aren't checking an item that can't be
                if (item.kind != wxITEM_CHECK && item.kind != wxITEM_RADIO)
                    continue;

                bool is_checked = (item.state & wxAUI_BUTTON_STATE_CHECKED) ? true : false;
                bool new_checked = evt.GetChecked();

                if (new_checked != is_checked)
                {
                    if (new_checked)
                        item.state |= wxAUI_BUTTON_STATE_CHECKED;
                    else
                        item.state &= ~wxAUI_BUTTON_STATE_CHECKED;

                    need_refresh = true;
                }
            }

        }
    }


    if (need_refresh)
    {
        Refresh(false);
    }
}


void wxAuiToolBar::OnSize(wxSizeEvent& WXUNUSED(evt))
{
    int x, y;
    GetClientSize(&x, &y);

    if (x > y)
        SetOrientation(wxHORIZONTAL);
    else
        SetOrientation(wxVERTICAL);

    if (((x >= y) && m_absolute_min_size.x > x) ||
        ((y > x) && m_absolute_min_size.y > y))
    {
        // hide all flexible items
        size_t i, count;
        for (i = 0, count = m_items.GetCount(); i < count; ++i)
        {
            wxAuiToolBarItem& item = m_items.Item(i);
            if (item.sizer_item && item.proportion > 0 && item.sizer_item->IsShown())
            {
                item.sizer_item->Show(false);
                item.sizer_item->SetProportion(0);
            }
        }
    }
    else
    {
        // show all flexible items
        size_t i, count;
        for (i = 0, count = m_items.GetCount(); i < count; ++i)
        {
            wxAuiToolBarItem& item = m_items.Item(i);
            if (item.sizer_item && item.proportion > 0 && !item.sizer_item->IsShown())
            {
                item.sizer_item->Show(true);
                item.sizer_item->SetProportion(item.proportion);
            }
        }
    }

    m_sizer->SetDimension(0, 0, x, y);

    Refresh(false);
    Update();
}



void wxAuiToolBar::DoSetSize(int x,
                             int y,
                             int width,
                             int height,
                             int sizeFlags)
{
    wxSize parent_size = GetParent()->GetClientSize();
    if (x + width > parent_size.x)
        width = wxMax(0, parent_size.x - x);
    if (y + height > parent_size.y)
        height = wxMax(0, parent_size.y - y);

    wxWindow::DoSetSize(x, y, width, height, sizeFlags);
}


void wxAuiToolBar::OnIdle(wxIdleEvent& evt)
{
    DoIdleUpdate();
    evt.Skip();
}

void wxAuiToolBar::OnPaint(wxPaintEvent& WXUNUSED(evt))
{
    wxBufferedPaintDC dc(this);
    wxRect cli_rect(wxPoint(0,0), GetClientSize());


    bool horizontal = true;
    if (m_style & wxAUI_TB_VERTICAL)
        horizontal = false;


    m_art->DrawBackground(dc, this, cli_rect);

    int gripper_size = m_art->GetElementSize(wxAUI_TBART_GRIPPER_SIZE);
    int dropdown_size = m_art->GetElementSize(wxAUI_TBART_OVERFLOW_SIZE);

    // paint the gripper
    if (gripper_size > 0 && m_gripper_sizer_item)
    {
        wxRect gripper_rect = m_gripper_sizer_item->GetRect();
        if (horizontal)
            gripper_rect.width = gripper_size;
        else
            gripper_rect.height = gripper_size;
        m_art->DrawGripper(dc, this, gripper_rect);
    }

    // calculated how far we can draw items
    int last_extent;
    if (horizontal)
        last_extent = cli_rect.width;
    else
        last_extent = cli_rect.height;
    if (m_overflow_visible)
        last_extent -= dropdown_size;

    // paint each individual tool
    size_t i, count = m_items.GetCount();
    for (i = 0; i < count; ++i)
    {
        wxAuiToolBarItem& item = m_items.Item(i);

        if (!item.sizer_item)
            continue;

        wxRect item_rect = item.sizer_item->GetRect();


        if ((horizontal  && item_rect.x + item_rect.width >= last_extent) ||
            (!horizontal && item_rect.y + item_rect.height >= last_extent))
        {
            break;
        }

        if (item.kind == wxITEM_SEPARATOR)
        {
            // draw a separator
            m_art->DrawSeparator(dc, this, item_rect);
        }
        else if (item.kind == wxITEM_LABEL)
        {
            // draw a text label only
            m_art->DrawLabel(dc, this, item, item_rect);
        }
        else if (item.kind == wxITEM_NORMAL)
        {
            // draw a regular button or dropdown button
            if (!item.dropdown)
                m_art->DrawButton(dc, this, item, item_rect);
            else
                m_art->DrawDropDownButton(dc, this, item, item_rect);
        }
        else if (item.kind == wxITEM_CHECK)
        {
            // draw a toggle button
            m_art->DrawButton(dc, this, item, item_rect);
        }
        else if (item.kind == wxITEM_RADIO)
        {
            // draw a toggle button
            m_art->DrawButton(dc, this, item, item_rect);
        }
        else if (item.kind == wxITEM_CONTROL)
        {
            // draw the control's label
            m_art->DrawControlLabel(dc, this, item, item_rect);
        }

        // fire a signal to see if the item wants to be custom-rendered
        OnCustomRender(dc, item, item_rect);
    }

    // paint the overflow button
    if (dropdown_size > 0 && m_overflow_sizer_item)
    {
        wxRect dropdown_rect = GetOverflowRect();
        m_art->DrawOverflowButton(dc, this, dropdown_rect, m_overflow_state);
    }
}

void wxAuiToolBar::OnEraseBackground(wxEraseEvent& WXUNUSED(evt))
{
    // empty
}

void wxAuiToolBar::OnLeftDown(wxMouseEvent& evt)
{
    wxRect cli_rect(wxPoint(0,0), GetClientSize());

    if (m_gripper_sizer_item)
    {
        wxRect gripper_rect = m_gripper_sizer_item->GetRect();
        if (gripper_rect.Contains(evt.GetX(), evt.GetY()))
        {
            // find aui manager
            wxAuiManager* manager = wxAuiManager::GetManager(this);
            if (!manager)
                return;

            int x_drag_offset = evt.GetX() - gripper_rect.GetX();
            int y_drag_offset = evt.GetY() - gripper_rect.GetY();

            // gripper was clicked
            manager->StartPaneDrag(this, wxPoint(x_drag_offset, y_drag_offset));
            return;
        }
    }

    if (m_overflow_sizer_item)
    {
        wxRect overflow_rect = GetOverflowRect();

        if (m_art &&
            m_overflow_visible &&
            overflow_rect.Contains(evt.m_x, evt.m_y))
        {
            wxAuiToolBarEvent e(wxEVT_COMMAND_AUITOOLBAR_OVERFLOW_CLICK, -1);
            e.SetEventObject(this);
            e.SetToolId(-1);
            e.SetClickPoint(wxPoint(evt.GetX(), evt.GetY()));
            bool processed = ProcessEvent(e);

            if (processed)
            {
                DoIdleUpdate();
            }
            else
            {
                size_t i, count;
                wxAuiToolBarItemArray overflow_items;


                // add custom overflow prepend items, if any
                count = m_custom_overflow_prepend.GetCount();
                for (i = 0; i < count; ++i)
                    overflow_items.Add(m_custom_overflow_prepend[i]);

                // only show items that don't fit in the dropdown
                count = m_items.GetCount();
                for (i = 0; i < count; ++i)
                {
                    if (!GetToolFitsByIndex(i))
                        overflow_items.Add(m_items[i]);
                }

                // add custom overflow append items, if any
                count = m_custom_overflow_append.GetCount();
                for (i = 0; i < count; ++i)
                    overflow_items.Add(m_custom_overflow_append[i]);

                int res = m_art->ShowDropDown(this, overflow_items);
                m_overflow_state = 0;
                Refresh(false);
                if (res != -1)
                {
                    wxCommandEvent e(wxEVT_COMMAND_MENU_SELECTED, res);
                    e.SetEventObject(this);
                    GetParent()->ProcessEvent(e);
                }
            }

            return;
        }
    }

    m_dragging = false;
    m_action_pos = wxPoint(evt.GetX(), evt.GetY());
    m_action_item = FindToolByPosition(evt.GetX(), evt.GetY());

    if (m_action_item)
    {
        if (m_action_item->state & wxAUI_BUTTON_STATE_DISABLED)
        {
            m_action_pos = wxPoint(-1,-1);
            m_action_item = NULL;
            return;
        }

        SetPressedItem(m_action_item);

        // fire the tool dropdown event
        wxAuiToolBarEvent e(wxEVT_COMMAND_AUITOOLBAR_TOOL_DROPDOWN, m_action_item->id);
        e.SetEventObject(this);
        e.SetToolId(m_action_item->id);
        e.SetDropDownClicked(false);

        int mouse_x = evt.GetX();
        wxRect rect = m_action_item->sizer_item->GetRect();

        if (m_action_item->dropdown &&
            mouse_x >= (rect.x+rect.width-BUTTON_DROPDOWN_WIDTH-1) &&
            mouse_x < (rect.x+rect.width))
        {
            e.SetDropDownClicked(true);
        }

        e.SetClickPoint(evt.GetPosition());
        e.SetItemRect(rect);
        ProcessEvent(e);
        DoIdleUpdate();
    }
}

void wxAuiToolBar::OnLeftUp(wxMouseEvent& evt)
{
    SetPressedItem(NULL);

    wxAuiToolBarItem* hit_item = FindToolByPosition(evt.GetX(), evt.GetY());
    if (hit_item && !(hit_item->state & wxAUI_BUTTON_STATE_DISABLED))
    {
        SetHoverItem(hit_item);
    }


    if (m_dragging)
    {
        // reset drag and drop member variables
        m_dragging = false;
        m_action_pos = wxPoint(-1,-1);
        m_action_item = NULL;
        return;
    }
    else
    {
        wxAuiToolBarItem* hit_item;
        hit_item = FindToolByPosition(evt.GetX(), evt.GetY());

        if (m_action_item && hit_item == m_action_item)
        {
            UnsetToolTip();

            if (hit_item->kind == wxITEM_CHECK || hit_item->kind == wxITEM_RADIO)
            {
                bool toggle = false;

                if (m_action_item->state & wxAUI_BUTTON_STATE_CHECKED)
                    toggle = false;
                else
                    toggle = true;

                ToggleTool(m_action_item->id, toggle);

                // repaint immediately
                Refresh(false);
                Update();

                wxCommandEvent e(wxEVT_COMMAND_MENU_SELECTED, m_action_item->id);
                e.SetEventObject(this);
                e.SetInt(toggle);
                ProcessEvent(e);
                DoIdleUpdate();
            }
            else
            {
                wxCommandEvent e(wxEVT_COMMAND_MENU_SELECTED, m_action_item->id);
                e.SetEventObject(this);
                ProcessEvent(e);
                DoIdleUpdate();
            }
        }
    }

    // reset drag and drop member variables
    m_dragging = false;
    m_action_pos = wxPoint(-1,-1);
    m_action_item = NULL;
}

void wxAuiToolBar::OnRightDown(wxMouseEvent& evt)
{
    wxRect cli_rect(wxPoint(0,0), GetClientSize());

    if (m_gripper_sizer_item)
    {
        wxRect gripper_rect = m_gripper_sizer_item->GetRect();
        if (gripper_rect.Contains(evt.GetX(), evt.GetY()))
            return;
    }

    if (m_overflow_sizer_item)
    {
        int dropdown_size = m_art->GetElementSize(wxAUI_TBART_OVERFLOW_SIZE);
        if (dropdown_size > 0 &&
            evt.m_x > cli_rect.width - dropdown_size &&
            evt.m_y >= 0 &&
            evt.m_y < cli_rect.height &&
            m_art)
        {
            return;
        }
    }

    m_action_pos = wxPoint(evt.GetX(), evt.GetY());
    m_action_item = FindToolByPosition(evt.GetX(), evt.GetY());

    if (m_action_item)
    {
        if (m_action_item->state & wxAUI_BUTTON_STATE_DISABLED)
        {
            m_action_pos = wxPoint(-1,-1);
            m_action_item = NULL;
            return;
        }
    }
}

void wxAuiToolBar::OnRightUp(wxMouseEvent& evt)
{
    wxAuiToolBarItem* hit_item;
    hit_item = FindToolByPosition(evt.GetX(), evt.GetY());

    if (m_action_item && hit_item == m_action_item)
    {
        wxAuiToolBarEvent e(wxEVT_COMMAND_AUITOOLBAR_RIGHT_CLICK, m_action_item->id);
        e.SetEventObject(this);
        e.SetToolId(m_action_item->id);
        e.SetClickPoint(m_action_pos);
        ProcessEvent(e);
        DoIdleUpdate();
    }
    else
    {
        // right-clicked on the invalid area of the toolbar
        wxAuiToolBarEvent e(wxEVT_COMMAND_AUITOOLBAR_RIGHT_CLICK, -1);
        e.SetEventObject(this);
        e.SetToolId(-1);
        e.SetClickPoint(m_action_pos);
        ProcessEvent(e);
        DoIdleUpdate();
    }

    // reset member variables
    m_action_pos = wxPoint(-1,-1);
    m_action_item = NULL;
}

void wxAuiToolBar::OnMiddleDown(wxMouseEvent& evt)
{
    wxRect cli_rect(wxPoint(0,0), GetClientSize());

    if (m_gripper_sizer_item)
    {
        wxRect gripper_rect = m_gripper_sizer_item->GetRect();
        if (gripper_rect.Contains(evt.GetX(), evt.GetY()))
            return;
    }

    if (m_overflow_sizer_item)
    {
        int dropdown_size = m_art->GetElementSize(wxAUI_TBART_OVERFLOW_SIZE);
        if (dropdown_size > 0 &&
            evt.m_x > cli_rect.width - dropdown_size &&
            evt.m_y >= 0 &&
            evt.m_y < cli_rect.height &&
            m_art)
        {
            return;
        }
    }

    m_action_pos = wxPoint(evt.GetX(), evt.GetY());
    m_action_item = FindToolByPosition(evt.GetX(), evt.GetY());

    if (m_action_item)
    {
        if (m_action_item->state & wxAUI_BUTTON_STATE_DISABLED)
        {
            m_action_pos = wxPoint(-1,-1);
            m_action_item = NULL;
            return;
        }
    }
}

void wxAuiToolBar::OnMiddleUp(wxMouseEvent& evt)
{
    wxAuiToolBarItem* hit_item;
    hit_item = FindToolByPosition(evt.GetX(), evt.GetY());

    if (m_action_item && hit_item == m_action_item)
    {
        if (hit_item->kind == wxITEM_NORMAL)
        {
            wxAuiToolBarEvent e(wxEVT_COMMAND_AUITOOLBAR_MIDDLE_CLICK, m_action_item->id);
            e.SetEventObject(this);
            e.SetToolId(m_action_item->id);
            e.SetClickPoint(m_action_pos);
            ProcessEvent(e);
            DoIdleUpdate();
        }
    }

    // reset member variables
    m_action_pos = wxPoint(-1,-1);
    m_action_item = NULL;
}

void wxAuiToolBar::OnMotion(wxMouseEvent& evt)
{
    // start a drag event
    if (!m_dragging &&
        m_action_item != NULL &&
        m_action_pos != wxPoint(-1,-1) &&
        abs(evt.m_x - m_action_pos.x) + abs(evt.m_y - m_action_pos.y) > 5)
    {
        UnsetToolTip();

        m_dragging = true;

        wxAuiToolBarEvent e(wxEVT_COMMAND_AUITOOLBAR_BEGIN_DRAG, GetId());
        e.SetEventObject(this);
        e.SetToolId(m_action_item->id);
        ProcessEvent(e);
        DoIdleUpdate();
        return;
    }

    wxAuiToolBarItem* hit_item = FindToolByPosition(evt.GetX(), evt.GetY());
    if (hit_item)
    {
        if (!(hit_item->state & wxAUI_BUTTON_STATE_DISABLED))
            SetHoverItem(hit_item);
        else
            SetHoverItem(NULL);
    }
    else
    {
        // no hit item, remove any hit item
        SetHoverItem(hit_item);
    }

    // figure out tooltips
    wxAuiToolBarItem* packing_hit_item;
    packing_hit_item = FindToolByPositionWithPacking(evt.GetX(), evt.GetY());
    if (packing_hit_item)
    {
        if (packing_hit_item != m_tip_item)
        {
            m_tip_item = packing_hit_item;

            if ( !packing_hit_item->short_help.empty() )
                SetToolTip(packing_hit_item->short_help);
            else
                UnsetToolTip();
        }
    }
    else
    {
        UnsetToolTip();
        m_tip_item = NULL;
    }

    // if we've pressed down an item and we're hovering
    // over it, make sure it's state is set to pressed
    if (m_action_item)
    {
        if (m_action_item == hit_item)
            SetPressedItem(m_action_item);
        else
            SetPressedItem(NULL);
    }

    // figure out the dropdown button state (are we hovering or pressing it?)
    RefreshOverflowState();
}

void wxAuiToolBar::OnLeaveWindow(wxMouseEvent& WXUNUSED(evt))
{
    RefreshOverflowState();
    SetHoverItem(NULL);
    SetPressedItem(NULL);

    m_tip_item = NULL;
}


void wxAuiToolBar::OnSetCursor(wxSetCursorEvent& evt)
{
    wxCursor cursor = wxNullCursor;

    if (m_gripper_sizer_item)
    {
        wxRect gripper_rect = m_gripper_sizer_item->GetRect();
        if (gripper_rect.Contains(evt.GetX(), evt.GetY()))
        {
            cursor = wxCursor(wxCURSOR_SIZING);
        }
    }

    evt.SetCursor(cursor);
}


#endif // wxUSE_AUI

