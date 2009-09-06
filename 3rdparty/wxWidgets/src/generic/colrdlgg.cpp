/////////////////////////////////////////////////////////////////////////////
// Name:        src/generic/colrdlgg.cpp
// Purpose:     Choice dialogs
// Author:      Julian Smart
// Modified by:
// Created:     04/01/98
// RCS-ID:      $Id: colrdlgg.cpp 41838 2006-10-09 21:08:45Z VZ $
// Copyright:   (c) Julian Smart
// Licence:     wxWindows licence
/////////////////////////////////////////////////////////////////////////////

// For compilers that support precompilation, includes "wx.h".
#include "wx/wxprec.h"

#ifdef __BORLANDC__
    #pragma hdrstop
#endif

#if wxUSE_COLOURDLG && (!defined(__WXGTK20__) || defined(__WXUNIVERSAL__))

#ifndef WX_PRECOMP
    #include "wx/utils.h"
    #include "wx/intl.h"
    #include "wx/dialog.h"
    #include "wx/listbox.h"
    #include "wx/button.h"
    #include "wx/stattext.h"
    #include "wx/layout.h"
    #include "wx/dcclient.h"
    #include "wx/sizer.h"
    #include "wx/slider.h"
#endif

#if wxUSE_STATLINE
    #include "wx/statline.h"
#endif

#include "wx/generic/colrdlgg.h"

IMPLEMENT_DYNAMIC_CLASS(wxGenericColourDialog, wxDialog)

BEGIN_EVENT_TABLE(wxGenericColourDialog, wxDialog)
    EVT_BUTTON(wxID_ADD_CUSTOM, wxGenericColourDialog::OnAddCustom)
#if wxUSE_SLIDER
    EVT_SLIDER(wxID_RED_SLIDER, wxGenericColourDialog::OnRedSlider)
    EVT_SLIDER(wxID_GREEN_SLIDER, wxGenericColourDialog::OnGreenSlider)
    EVT_SLIDER(wxID_BLUE_SLIDER, wxGenericColourDialog::OnBlueSlider)
#endif
    EVT_PAINT(wxGenericColourDialog::OnPaint)
    EVT_MOUSE_EVENTS(wxGenericColourDialog::OnMouseEvent)
    EVT_CLOSE(wxGenericColourDialog::OnCloseWindow)
END_EVENT_TABLE()


/*
 * Generic wxColourDialog
 */

// don't change the number of elements (48) in this array, the code below is
// hardcoded to use it
static const wxChar *wxColourDialogNames[] =
{
    wxT("ORANGE"),
    wxT("GOLDENROD"),
    wxT("WHEAT"),
    wxT("SPRING GREEN"),
    wxT("SKY BLUE"),
    wxT("SLATE BLUE"),
    wxT("MEDIUM VIOLET RED"),
    wxT("PURPLE"),

    wxT("RED"),
    wxT("YELLOW"),
    wxT("MEDIUM SPRING GREEN"),
    wxT("PALE GREEN"),
    wxT("CYAN"),
    wxT("LIGHT STEEL BLUE"),
    wxT("ORCHID"),
    wxT("LIGHT MAGENTA"),

    wxT("BROWN"),
    wxT("YELLOW"),
    wxT("GREEN"),
    wxT("CADET BLUE"),
    wxT("MEDIUM BLUE"),
    wxT("MAGENTA"),
    wxT("MAROON"),
    wxT("ORANGE RED"),

    wxT("FIREBRICK"),
    wxT("CORAL"),
    wxT("FOREST GREEN"),
    wxT("AQUAMARINE"),
    wxT("BLUE"),
    wxT("NAVY"),
    wxT("THISTLE"),
    wxT("MEDIUM VIOLET RED"),

    wxT("INDIAN RED"),
    wxT("GOLD"),
    wxT("MEDIUM SEA GREEN"),
    wxT("MEDIUM BLUE"),
    wxT("MIDNIGHT BLUE"),
    wxT("GREY"),
    wxT("PURPLE"),
    wxT("KHAKI"),

    wxT("BLACK"),
    wxT("MEDIUM FOREST GREEN"),
    wxT("KHAKI"),
    wxT("DARK GREY"),
    wxT("SEA GREEN"),
    wxT("LIGHT GREY"),
    wxT("MEDIUM SLATE BLUE"),
    wxT("WHITE")
};

wxGenericColourDialog::wxGenericColourDialog()
{
    dialogParent = NULL;
    whichKind = 1;
    colourSelection = -1;
}

wxGenericColourDialog::wxGenericColourDialog(wxWindow *parent,
                                             wxColourData *data)
{
    whichKind = 1;
    colourSelection = -1;
    Create(parent, data);
}

wxGenericColourDialog::~wxGenericColourDialog()
{
}

void wxGenericColourDialog::OnCloseWindow(wxCloseEvent& WXUNUSED(event))
{
    EndModal(wxID_CANCEL);
}

bool wxGenericColourDialog::Create(wxWindow *parent, wxColourData *data)
{
    if ( !wxDialog::Create(parent, wxID_ANY, _("Choose colour"),
                           wxPoint(0,0), wxSize(900, 900)) )
        return false;

    dialogParent = parent;

    if (data)
        colourData = *data;

    InitializeColours();
    CalculateMeasurements();
    CreateWidgets();

    return true;
}

int wxGenericColourDialog::ShowModal()
{
    return wxDialog::ShowModal();
}


// Internal functions
void wxGenericColourDialog::OnMouseEvent(wxMouseEvent& event)
{
  if (event.ButtonDown(1))
  {
    int x = (int)event.GetX();
    int y = (int)event.GetY();

#ifdef __WXPM__
    // Handle OS/2's reverse coordinate system and account for the dialog title
    int                             nClientHeight;

    GetClientSize(NULL, &nClientHeight);
    y = (nClientHeight - y) + 20;
#endif
    if ((x >= standardColoursRect.x && x <= (standardColoursRect.x + standardColoursRect.width)) &&
        (y >= standardColoursRect.y && y <= (standardColoursRect.y + standardColoursRect.height)))
    {
      int selX = (int)(x - standardColoursRect.x)/(smallRectangleSize.x + gridSpacing);
      int selY = (int)(y - standardColoursRect.y)/(smallRectangleSize.y + gridSpacing);
      int ptr = (int)(selX + selY*8);
      OnBasicColourClick(ptr);
    }
    else if ((x >= customColoursRect.x && x <= (customColoursRect.x + customColoursRect.width)) &&
        (y >= customColoursRect.y && y <= (customColoursRect.y + customColoursRect.height)))
    {
      int selX = (int)(x - customColoursRect.x)/(smallRectangleSize.x + gridSpacing);
      int selY = (int)(y - customColoursRect.y)/(smallRectangleSize.y + gridSpacing);
      int ptr = (int)(selX + selY*8);
      OnCustomColourClick(ptr);
    }
    else
        event.Skip();
  }
  else
      event.Skip();
}

void wxGenericColourDialog::OnPaint(wxPaintEvent& event)
{
#if !defined(__WXMOTIF__) && !defined(__WXPM__) && !defined(__WXCOCOA__)
    wxDialog::OnPaint(event);
#else
    wxUnusedVar(event);
#endif

    wxPaintDC dc(this);

    PaintBasicColours(dc);
    PaintCustomColours(dc);
    PaintCustomColour(dc);
    PaintHighlight(dc, true);
}

void wxGenericColourDialog::CalculateMeasurements()
{
    smallRectangleSize.x = 18;
    smallRectangleSize.y = 14;
    customRectangleSize.x = 40;
    customRectangleSize.y = 40;

    gridSpacing = 6;
    sectionSpacing = 15;

    standardColoursRect.x = 10;
#ifdef __WXPM__
    standardColoursRect.y = 15 + 20; /* OS/2 needs to account for dialog titlebar */
#else
    standardColoursRect.y = 15;
#endif
    standardColoursRect.width = (8*smallRectangleSize.x) + (7*gridSpacing);
    standardColoursRect.height = (6*smallRectangleSize.y) + (5*gridSpacing);

    customColoursRect.x = standardColoursRect.x;
    customColoursRect.y = standardColoursRect.y + standardColoursRect.height  + 20;
    customColoursRect.width = (8*smallRectangleSize.x) + (7*gridSpacing);
    customColoursRect.height = (2*smallRectangleSize.y) + (1*gridSpacing);

    singleCustomColourRect.x = customColoursRect.width + customColoursRect.x + sectionSpacing;
    singleCustomColourRect.y = 80;
    singleCustomColourRect.width = customRectangleSize.x;
    singleCustomColourRect.height = customRectangleSize.y;

    okButtonX = 10;
    customButtonX = singleCustomColourRect.x ;
    buttonY = customColoursRect.y + customColoursRect.height + 10;
}

void wxGenericColourDialog::CreateWidgets()
{
    wxBeginBusyCursor();

    wxBoxSizer *topSizer = new wxBoxSizer( wxVERTICAL );

    const int sliderHeight = 160;

    // first sliders
#if wxUSE_SLIDER
    const int sliderX = singleCustomColourRect.x + singleCustomColourRect.width + sectionSpacing;

    redSlider = new wxSlider(this, wxID_RED_SLIDER, colourData.m_dataColour.Red(), 0, 255,
        wxDefaultPosition, wxSize(wxDefaultCoord, sliderHeight), wxSL_VERTICAL|wxSL_LABELS|wxSL_INVERSE);
    greenSlider = new wxSlider(this, wxID_GREEN_SLIDER, colourData.m_dataColour.Green(), 0, 255,
        wxDefaultPosition, wxSize(wxDefaultCoord, sliderHeight), wxSL_VERTICAL|wxSL_LABELS|wxSL_INVERSE);
    blueSlider = new wxSlider(this, wxID_BLUE_SLIDER, colourData.m_dataColour.Blue(), 0, 255,
        wxDefaultPosition, wxSize(wxDefaultCoord, sliderHeight), wxSL_VERTICAL|wxSL_LABELS|wxSL_INVERSE);

    wxBoxSizer *sliderSizer = new wxBoxSizer( wxHORIZONTAL );

    sliderSizer->Add(sliderX, sliderHeight );

    wxSizerFlags flagsRight;
    flagsRight.Align(wxALIGN_RIGHT | wxALIGN_CENTER_VERTICAL).DoubleBorder();

    sliderSizer->Add(redSlider, flagsRight);
    sliderSizer->Add(greenSlider,flagsRight);
    sliderSizer->Add(blueSlider,flagsRight);

    topSizer->Add(sliderSizer, wxSizerFlags().Centre().DoubleBorder());
#else
    topSizer->Add(1, sliderHeight, wxSizerFlags(1).Centre().TripleBorder());
#endif // wxUSE_SLIDER

    // then the custom button
    topSizer->Add(new wxButton(this, wxID_ADD_CUSTOM,
                                  _("Add to custom colours") ),
                     wxSizerFlags().DoubleHorzBorder());

    // then the standard buttons
    wxSizer *buttonsizer = CreateSeparatedButtonSizer(wxOK | wxCANCEL);
    if ( buttonsizer )
    {
        topSizer->Add(buttonsizer, wxSizerFlags().Expand().DoubleBorder());
    }

    SetAutoLayout( true );
    SetSizer( topSizer );

    topSizer->SetSizeHints( this );
    topSizer->Fit( this );

    Centre( wxBOTH );

    wxEndBusyCursor();
}

void wxGenericColourDialog::InitializeColours(void)
{
    size_t i;

    for (i = 0; i < WXSIZEOF(wxColourDialogNames); i++)
    {
        wxColour col = wxTheColourDatabase->Find(wxColourDialogNames[i]);
        if (col.Ok())
            standardColours[i].Set(col.Red(), col.Green(), col.Blue());
        else
            standardColours[i].Set(0, 0, 0);
    }

    for (i = 0; i < WXSIZEOF(customColours); i++)
    {
        wxColour c = colourData.GetCustomColour(i);
        if (c.Ok())
            customColours[i] = colourData.GetCustomColour(i);
        else
            customColours[i] = wxColour(255, 255, 255);
    }

    wxColour curr = colourData.GetColour();
    if ( curr.Ok() )
    {
        bool initColourFound = false;

        for (i = 0; i < WXSIZEOF(wxColourDialogNames); i++)
        {
            if ( standardColours[i] == curr && !initColourFound )
            {
                whichKind = 1;
                colourSelection = i;
                initColourFound = true;
                break;
            }
        }
        if ( !initColourFound )
        {
            for ( i = 0; i < WXSIZEOF(customColours); i++ )
            {
                if ( customColours[i] == curr )
                {
                    whichKind = 2;
                    colourSelection = i;
                    break;
                }
            }
        }
        colourData.m_dataColour.Set( curr.Red(), curr.Green(), curr.Blue() );
    }
    else
    {
        whichKind = 1;
        colourSelection = 0;
        colourData.m_dataColour.Set( 0, 0, 0 );
    }
}

void wxGenericColourDialog::PaintBasicColours(wxDC& dc)
{
    int i;
    for (i = 0; i < 6; i++)
    {
        int j;
        for (j = 0; j < 8; j++)
        {
            int ptr = i*8 + j;

            int x = (j*(smallRectangleSize.x+gridSpacing) + standardColoursRect.x);
            int y = (i*(smallRectangleSize.y+gridSpacing) + standardColoursRect.y);

            dc.SetPen(*wxBLACK_PEN);
            wxBrush brush(standardColours[ptr], wxSOLID);
            dc.SetBrush(brush);

            dc.DrawRectangle( x, y, smallRectangleSize.x, smallRectangleSize.y);
        }
    }
}

void wxGenericColourDialog::PaintCustomColours(wxDC& dc)
{
  int i;
  for (i = 0; i < 2; i++)
  {
    int j;
    for (j = 0; j < 8; j++)
    {
      int ptr = i*8 + j;

      int x = (j*(smallRectangleSize.x+gridSpacing)) + customColoursRect.x;
      int y = (i*(smallRectangleSize.y+gridSpacing)) + customColoursRect.y;

      dc.SetPen(*wxBLACK_PEN);

      wxBrush brush(customColours[ptr], wxSOLID);
      dc.SetBrush(brush);

      dc.DrawRectangle( x, y, smallRectangleSize.x, smallRectangleSize.y);
    }
  }
}

void wxGenericColourDialog::PaintHighlight(wxDC& dc, bool draw)
{
  if ( colourSelection < 0 )
      return;

  // Number of pixels bigger than the standard rectangle size
  // for drawing a highlight
  int deltaX = 2;
  int deltaY = 2;

  if (whichKind == 1)
  {
    // Standard colours
    int y = (int)(colourSelection / 8);
    int x = (int)(colourSelection - (y*8));

    x = (x*(smallRectangleSize.x + gridSpacing) + standardColoursRect.x) - deltaX;
    y = (y*(smallRectangleSize.y + gridSpacing) + standardColoursRect.y) - deltaY;

    if (draw)
      dc.SetPen(*wxBLACK_PEN);
    else
      dc.SetPen(*wxLIGHT_GREY_PEN);

    dc.SetBrush(*wxTRANSPARENT_BRUSH);
    dc.DrawRectangle( x, y, (smallRectangleSize.x + (2*deltaX)), (smallRectangleSize.y + (2*deltaY)));
  }
  else
  {
    // User-defined colours
    int y = (int)(colourSelection / 8);
    int x = (int)(colourSelection - (y*8));

    x = (x*(smallRectangleSize.x + gridSpacing) + customColoursRect.x) - deltaX;
    y = (y*(smallRectangleSize.y + gridSpacing) + customColoursRect.y) - deltaY;

    if (draw)
      dc.SetPen(*wxBLACK_PEN);
    else
      dc.SetPen(*wxLIGHT_GREY_PEN);

    dc.SetBrush(*wxTRANSPARENT_BRUSH);
    dc.DrawRectangle( x, y, (smallRectangleSize.x + (2*deltaX)), (smallRectangleSize.y + (2*deltaY)));
  }
}

void wxGenericColourDialog::PaintCustomColour(wxDC& dc)
{
    dc.SetPen(*wxBLACK_PEN);

    wxBrush *brush = new wxBrush(colourData.m_dataColour, wxSOLID);
    dc.SetBrush(*brush);

    dc.DrawRectangle( singleCustomColourRect.x, singleCustomColourRect.y,
                      customRectangleSize.x, customRectangleSize.y);

    dc.SetBrush(wxNullBrush);
    delete brush;
}

void wxGenericColourDialog::OnBasicColourClick(int which)
{
    wxClientDC dc(this);

    PaintHighlight(dc, false);
    whichKind = 1;
    colourSelection = which;

#if wxUSE_SLIDER
    redSlider->SetValue( standardColours[colourSelection].Red() );
    greenSlider->SetValue( standardColours[colourSelection].Green() );
    blueSlider->SetValue( standardColours[colourSelection].Blue() );
#endif // wxUSE_SLIDER

    colourData.m_dataColour.Set(standardColours[colourSelection].Red(),
                                standardColours[colourSelection].Green(),
                                standardColours[colourSelection].Blue());

    PaintCustomColour(dc);
    PaintHighlight(dc, true);
}

void wxGenericColourDialog::OnCustomColourClick(int which)
{
    wxClientDC dc(this);
    PaintHighlight(dc, false);
    whichKind = 2;
    colourSelection = which;

#if wxUSE_SLIDER
    redSlider->SetValue( customColours[colourSelection].Red() );
    greenSlider->SetValue( customColours[colourSelection].Green() );
    blueSlider->SetValue( customColours[colourSelection].Blue() );
#endif // wxUSE_SLIDER

    colourData.m_dataColour.Set(customColours[colourSelection].Red(),
                                customColours[colourSelection].Green(),
                                customColours[colourSelection].Blue());

    PaintCustomColour(dc);
    PaintHighlight(dc, true);
}

/*
void wxGenericColourDialog::OnOk(void)
{
  Show(false);
}

void wxGenericColourDialog::OnCancel(void)
{
  colourDialogCancelled = true;
  Show(false);
}
*/

void wxGenericColourDialog::OnAddCustom(wxCommandEvent& WXUNUSED(event))
{
  wxClientDC dc(this);
  if (whichKind != 2)
  {
    PaintHighlight(dc, false);
    whichKind = 2;
    colourSelection = 0;
    PaintHighlight(dc, true);
  }

  customColours[colourSelection].Set(colourData.m_dataColour.Red(),
                                     colourData.m_dataColour.Green(),
                                     colourData.m_dataColour.Blue());

  colourData.SetCustomColour(colourSelection, customColours[colourSelection]);

  PaintCustomColours(dc);
}

#if wxUSE_SLIDER

void wxGenericColourDialog::OnRedSlider(wxCommandEvent& WXUNUSED(event))
{
  if (!redSlider)
    return;

  wxClientDC dc(this);
  colourData.m_dataColour.Set((unsigned char)redSlider->GetValue(), colourData.m_dataColour.Green(), colourData.m_dataColour.Blue());
  PaintCustomColour(dc);
}

void wxGenericColourDialog::OnGreenSlider(wxCommandEvent& WXUNUSED(event))
{
  if (!greenSlider)
    return;

  wxClientDC dc(this);
  colourData.m_dataColour.Set(colourData.m_dataColour.Red(), (unsigned char)greenSlider->GetValue(), colourData.m_dataColour.Blue());
  PaintCustomColour(dc);
}

void wxGenericColourDialog::OnBlueSlider(wxCommandEvent& WXUNUSED(event))
{
  if (!blueSlider)
    return;

  wxClientDC dc(this);
  colourData.m_dataColour.Set(colourData.m_dataColour.Red(), colourData.m_dataColour.Green(), (unsigned char)blueSlider->GetValue());
  PaintCustomColour(dc);
}

#endif // wxUSE_SLIDER

#endif // wxUSE_COLOURDLG && !defined(__WXGTK20__)
