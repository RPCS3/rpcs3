/////////////////////////////////////////////////////////////////////////////
// Name:        wx/generic/colrdlgg.h
// Purpose:     wxGenericColourDialog
// Author:      Julian Smart
// Modified by:
// Created:     01/02/97
// RCS-ID:      $Id: colrdlgg.h 37164 2006-01-26 17:20:50Z ABX $
// Copyright:   (c) Julian Smart
// Licence:     wxWindows licence
/////////////////////////////////////////////////////////////////////////////

#ifndef __COLORDLGH_G__
#define __COLORDLGH_G__

#include "wx/defs.h"
#include "wx/gdicmn.h"
#include "wx/dialog.h"
#include "wx/cmndata.h"

#define wxID_ADD_CUSTOM     3000

#if wxUSE_SLIDER

    #define wxID_RED_SLIDER     3001
    #define wxID_GREEN_SLIDER   3002
    #define wxID_BLUE_SLIDER    3003

    class WXDLLEXPORT wxSlider;

#endif // wxUSE_SLIDER

class WXDLLEXPORT wxGenericColourDialog : public wxDialog
{
public:
    wxGenericColourDialog();
    wxGenericColourDialog(wxWindow *parent,
                          wxColourData *data = (wxColourData *) NULL);
    virtual ~wxGenericColourDialog();

    bool Create(wxWindow *parent, wxColourData *data = (wxColourData *) NULL);

    wxColourData &GetColourData() { return colourData; }

    virtual int ShowModal();

    // Internal functions
    void OnMouseEvent(wxMouseEvent& event);
    void OnPaint(wxPaintEvent& event);

    virtual void CalculateMeasurements();
    virtual void CreateWidgets();
    virtual void InitializeColours();

    virtual void PaintBasicColours(wxDC& dc);
    virtual void PaintCustomColours(wxDC& dc);
    virtual void PaintCustomColour(wxDC& dc);
    virtual void PaintHighlight(wxDC& dc, bool draw);

    virtual void OnBasicColourClick(int which);
    virtual void OnCustomColourClick(int which);

    void OnAddCustom(wxCommandEvent& event);

#if wxUSE_SLIDER
    void OnRedSlider(wxCommandEvent& event);
    void OnGreenSlider(wxCommandEvent& event);
    void OnBlueSlider(wxCommandEvent& event);
#endif // wxUSE_SLIDER

    void OnCloseWindow(wxCloseEvent& event);

protected:
    wxColourData colourData;
    wxWindow *dialogParent;

    // Area reserved for grids of colours
    wxRect standardColoursRect;
    wxRect customColoursRect;
    wxRect singleCustomColourRect;

    // Size of each colour rectangle
    wxPoint smallRectangleSize;

    // For single customizable colour
    wxPoint customRectangleSize;

    // Grid spacing (between rectangles)
    int gridSpacing;

    // Section spacing (between left and right halves of dialog box)
    int sectionSpacing;

    // 48 'standard' colours
    wxColour standardColours[48];

    // 16 'custom' colours
    wxColour customColours[16];

    // Which colour is selected? An index into one of the two areas.
    int colourSelection;
    int whichKind; // 1 for standard colours, 2 for custom colours,

#if wxUSE_SLIDER
    wxSlider *redSlider;
    wxSlider *greenSlider;
    wxSlider *blueSlider;
#endif // wxUSE_SLIDER

    int buttonY;

    int okButtonX;
    int customButtonX;

    //  static bool colourDialogCancelled;

    DECLARE_EVENT_TABLE()
    DECLARE_DYNAMIC_CLASS(wxGenericColourDialog)
};

#endif
