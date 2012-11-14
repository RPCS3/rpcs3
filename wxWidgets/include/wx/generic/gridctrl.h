///////////////////////////////////////////////////////////////////////////
// Name:        generic/gridctrl.h
// Purpose:     wxGrid controls
// Author:      Paul Gammans, Roger Gammans
// Modified by:
// Created:     11/04/2001
// RCS-ID:      $Id: gridctrl.h 61872 2009-09-09 22:37:05Z VZ $
// Copyright:   (c) The Computer Surgery (paul@compsurg.co.uk)
// Licence:     wxWindows licence
/////////////////////////////////////////////////////////////////////////////

#ifndef _WX_GENERIC_GRIDCTRL_H_
#define _WX_GENERIC_GRIDCTRL_H_

#include "wx/grid.h"

#if wxUSE_GRID

#define wxGRID_VALUE_CHOICEINT    wxT("choiceint")
#define wxGRID_VALUE_DATETIME     wxT("datetime")

#if wxUSE_DATETIME

#include "wx/datetime.h"

// the default renderer for the cells containing Time and dates..
class WXDLLIMPEXP_ADV wxGridCellDateTimeRenderer : public wxGridCellStringRenderer
{
public:
    wxGridCellDateTimeRenderer(const wxString& outformat = wxDefaultDateTimeFormat,
                               const wxString& informat = wxDefaultDateTimeFormat);

    // draw the string right aligned
    virtual void Draw(wxGrid& grid,
                      wxGridCellAttr& attr,
                      wxDC& dc,
                      const wxRect& rect,
                      int row, int col,
                      bool isSelected);

    virtual wxSize GetBestSize(wxGrid& grid,
                               wxGridCellAttr& attr,
                               wxDC& dc,
                               int row, int col);

    virtual wxGridCellRenderer *Clone() const;

    // parameters string format is "width[,precision]"
    virtual void SetParameters(const wxString& params);

protected:
    wxString GetString(const wxGrid& grid, int row, int col);

    wxString m_iformat;
    wxString m_oformat;
    wxDateTime m_dateDef;
    wxDateTime::TimeZone m_tz;
};

#endif // wxUSE_DATETIME

// the default renderer for the cells containing Time and dates..
class WXDLLIMPEXP_ADV wxGridCellEnumRenderer : public wxGridCellStringRenderer
{
public:
    wxGridCellEnumRenderer( const wxString& choices = wxEmptyString );

    // draw the string right aligned
    virtual void Draw(wxGrid& grid,
                      wxGridCellAttr& attr,
                      wxDC& dc,
                      const wxRect& rect,
                      int row, int col,
                      bool isSelected);

    virtual wxSize GetBestSize(wxGrid& grid,
                               wxGridCellAttr& attr,
                               wxDC& dc,
                               int row, int col);

    virtual wxGridCellRenderer *Clone() const;

    // parameters string format is "item1[,item2[...,itemN]]"
    virtual void SetParameters(const wxString& params);

protected:
    wxString GetString(const wxGrid& grid, int row, int col);

    wxArrayString m_choices;
};


#if wxUSE_COMBOBOX

class WXDLLIMPEXP_ADV wxGridCellEnumEditor : public wxGridCellChoiceEditor
{
public:
    wxGridCellEnumEditor( const wxString& choices = wxEmptyString );
    virtual ~wxGridCellEnumEditor() {}

    virtual wxGridCellEditor*  Clone() const;

    virtual bool EndEdit(int row, int col, wxGrid* grid);
    virtual void BeginEdit(int row, int col, wxGrid* grid);

private:
    long int   m_startint;

    DECLARE_NO_COPY_CLASS(wxGridCellEnumEditor)
};

#endif // wxUSE_COMBOBOX

class WXDLLIMPEXP_ADV wxGridCellAutoWrapStringEditor : public wxGridCellTextEditor
{
public:
    wxGridCellAutoWrapStringEditor() : wxGridCellTextEditor() { }
    virtual void Create(wxWindow* parent,
                        wxWindowID id,
                        wxEvtHandler* evtHandler);

    virtual wxGridCellEditor *Clone() const
        { return new wxGridCellAutoWrapStringEditor; }

    DECLARE_NO_COPY_CLASS(wxGridCellAutoWrapStringEditor)
};

class WXDLLIMPEXP_ADV wxGridCellAutoWrapStringRenderer : public wxGridCellStringRenderer
{
public:
    wxGridCellAutoWrapStringRenderer() : wxGridCellStringRenderer() { }

    virtual void Draw(wxGrid& grid,
                      wxGridCellAttr& attr,
                      wxDC& dc,
                      const wxRect& rect,
                      int row, int col,
                      bool isSelected);

    virtual wxSize GetBestSize(wxGrid& grid,
                               wxGridCellAttr& attr,
                               wxDC& dc,
                               int row, int col);

    virtual wxGridCellRenderer *Clone() const
        { return new wxGridCellAutoWrapStringRenderer; }

private:
    wxArrayString GetTextLines( wxGrid& grid,
                                wxDC& dc,
                                const wxGridCellAttr& attr,
                                const wxRect& rect,
                                int row, int col);

};

#endif  // wxUSE_GRID
#endif // _WX_GENERIC_GRIDCTRL_H_
