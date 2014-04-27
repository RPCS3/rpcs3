///////////////////////////////////////////////////////////////////////////
// Name:        generic/gridctrl.cpp
// Purpose:     wxGrid controls
// Author:      Paul Gammans, Roger Gammans
// Modified by:
// Created:     11/04/2001
// RCS-ID:      $Id: gridctrl.cpp 59121 2009-02-25 00:09:23Z VZ $
// Copyright:   (c) The Computer Surgery (paul@compsurg.co.uk)
// Licence:     wxWindows licence
/////////////////////////////////////////////////////////////////////////////

#include "wx/wxprec.h"

#ifdef __BORLANDC__
    #pragma hdrstop
#endif

#if wxUSE_GRID

#include "wx/generic/gridctrl.h"

#ifndef WX_PRECOMP
    #include "wx/textctrl.h"
    #include "wx/dc.h"
    #include "wx/combobox.h"
#endif // WX_PRECOMP

#include "wx/tokenzr.h"

// ----------------------------------------------------------------------------
// wxGridCellDateTimeRenderer
// ----------------------------------------------------------------------------

#if wxUSE_DATETIME

// Enables a grid cell to display a formatted date and or time

wxGridCellDateTimeRenderer::wxGridCellDateTimeRenderer(const wxString& outformat, const wxString& informat)
{
    m_iformat = informat;
    m_oformat = outformat;
    m_tz = wxDateTime::Local;
    m_dateDef = wxDefaultDateTime;
}

wxGridCellRenderer *wxGridCellDateTimeRenderer::Clone() const
{
    wxGridCellDateTimeRenderer *renderer = new wxGridCellDateTimeRenderer;
    renderer->m_iformat = m_iformat;
    renderer->m_oformat = m_oformat;
    renderer->m_dateDef = m_dateDef;
    renderer->m_tz = m_tz;

    return renderer;
}

wxString wxGridCellDateTimeRenderer::GetString(const wxGrid& grid, int row, int col)
{
    wxGridTableBase *table = grid.GetTable();

    bool hasDatetime = false;
    wxDateTime val;
    wxString text;
    if ( table->CanGetValueAs(row, col, wxGRID_VALUE_DATETIME) )
    {
        void * tempval = table->GetValueAsCustom(row, col,wxGRID_VALUE_DATETIME);

        if (tempval){
            val = *((wxDateTime *)tempval);
            hasDatetime = true;
            delete (wxDateTime *)tempval;
        }

    }

    if (!hasDatetime )
    {
        text = table->GetValue(row, col);
        hasDatetime = (val.ParseFormat( text, m_iformat, m_dateDef ) != (wxChar *)NULL) ;
    }

    if ( hasDatetime )
        text = val.Format(m_oformat, m_tz );

    //If we faild to parse string just show what we where given?
    return text;
}

void wxGridCellDateTimeRenderer::Draw(wxGrid& grid,
                                   wxGridCellAttr& attr,
                                   wxDC& dc,
                                   const wxRect& rectCell,
                                   int row, int col,
                                   bool isSelected)
{
    wxGridCellRenderer::Draw(grid, attr, dc, rectCell, row, col, isSelected);

    SetTextColoursAndFont(grid, attr, dc, isSelected);

    // draw the text right aligned by default
    int hAlign, vAlign;
    attr.GetAlignment(&hAlign, &vAlign);
    hAlign = wxRIGHT;

    wxRect rect = rectCell;
    rect.Inflate(-1);

    grid.DrawTextRectangle(dc, GetString(grid, row, col), rect, hAlign, vAlign);
}

wxSize wxGridCellDateTimeRenderer::GetBestSize(wxGrid& grid,
                                            wxGridCellAttr& attr,
                                            wxDC& dc,
                                            int row, int col)
{
    return DoGetBestSize(attr, dc, GetString(grid, row, col));
}

void wxGridCellDateTimeRenderer::SetParameters(const wxString& params)
{
    if (!params.empty())
        m_oformat=params;
}

#endif // wxUSE_DATETIME

// ----------------------------------------------------------------------------
// wxGridCellChoiceNumberRenderer
// ----------------------------------------------------------------------------
// Renders a number as a textual equivalent.
// eg data in cell is 0,1,2 ... n the cell could be rendered as "John","Fred"..."Bob"


wxGridCellEnumRenderer::wxGridCellEnumRenderer(const wxString& choices)
{
    if (!choices.empty())
        SetParameters(choices);
}

wxGridCellRenderer *wxGridCellEnumRenderer::Clone() const
{
    wxGridCellEnumRenderer *renderer = new wxGridCellEnumRenderer;
    renderer->m_choices = m_choices;
    return renderer;
}

wxString wxGridCellEnumRenderer::GetString(const wxGrid& grid, int row, int col)
{
    wxGridTableBase *table = grid.GetTable();
    wxString text;
    if ( table->CanGetValueAs(row, col, wxGRID_VALUE_NUMBER) )
    {
        int choiceno = table->GetValueAsLong(row, col);
        text.Printf(_T("%s"), m_choices[ choiceno ].c_str() );
    }
    else
    {
        text = table->GetValue(row, col);
    }


    //If we faild to parse string just show what we where given?
    return text;
}

void wxGridCellEnumRenderer::Draw(wxGrid& grid,
                                   wxGridCellAttr& attr,
                                   wxDC& dc,
                                   const wxRect& rectCell,
                                   int row, int col,
                                   bool isSelected)
{
    wxGridCellRenderer::Draw(grid, attr, dc, rectCell, row, col, isSelected);

    SetTextColoursAndFont(grid, attr, dc, isSelected);

    // draw the text right aligned by default
    int hAlign, vAlign;
    attr.GetAlignment(&hAlign, &vAlign);
    hAlign = wxRIGHT;

    wxRect rect = rectCell;
    rect.Inflate(-1);

    grid.DrawTextRectangle(dc, GetString(grid, row, col), rect, hAlign, vAlign);
}

wxSize wxGridCellEnumRenderer::GetBestSize(wxGrid& grid,
                                            wxGridCellAttr& attr,
                                            wxDC& dc,
                                            int row, int col)
{
    return DoGetBestSize(attr, dc, GetString(grid, row, col));
}

void wxGridCellEnumRenderer::SetParameters(const wxString& params)
{
    if ( !params )
    {
        // what can we do?
        return;
    }

    m_choices.Empty();

    wxStringTokenizer tk(params, _T(','));
    while ( tk.HasMoreTokens() )
    {
        m_choices.Add(tk.GetNextToken());
    }
}

#if wxUSE_COMBOBOX

// ----------------------------------------------------------------------------
// wxGridCellEnumEditor
// ----------------------------------------------------------------------------

// A cell editor which displays an enum number as a textual equivalent. eg
// data in cell is 0,1,2 ... n the cell could be displayed as
// "John","Fred"..."Bob" in the combo choice box

wxGridCellEnumEditor::wxGridCellEnumEditor(const wxString& choices)
                     :wxGridCellChoiceEditor()
{
    m_startint = -1;

    if (!choices.empty())
        SetParameters(choices);
}

wxGridCellEditor *wxGridCellEnumEditor::Clone() const
{
    wxGridCellEnumEditor *editor = new wxGridCellEnumEditor();
    editor->m_startint = m_startint;
    return editor;
}

void wxGridCellEnumEditor::BeginEdit(int row, int col, wxGrid* grid)
{
    wxASSERT_MSG(m_control,
                 wxT("The wxGridCellEnumEditor must be Created first!"));

    wxGridTableBase *table = grid->GetTable();

    if ( table->CanGetValueAs(row, col, wxGRID_VALUE_NUMBER) )
    {
        m_startint = table->GetValueAsLong(row, col);
    }
    else
    {
        wxString startValue = table->GetValue(row, col);
        if (startValue.IsNumber() && !startValue.empty())
        {
            startValue.ToLong(&m_startint);
        }
        else
        {
            m_startint=-1;
        }
    }

    Combo()->SetSelection(m_startint);
    Combo()->SetInsertionPointEnd();
    Combo()->SetFocus();

}

bool wxGridCellEnumEditor::EndEdit(int row, int col, wxGrid* grid)
{
    int pos = Combo()->GetSelection();
    bool changed = (pos != m_startint);
    if (changed)
    {
        if (grid->GetTable()->CanSetValueAs(row, col, wxGRID_VALUE_NUMBER))
            grid->GetTable()->SetValueAsLong(row, col, pos);
        else
            grid->GetTable()->SetValue(row, col,wxString::Format(wxT("%i"),pos));
    }

    return changed;
}

#endif // wxUSE_COMBOBOX

// ----------------------------------------------------------------------------
// wxGridCellAutoWrapStringEditor
// ----------------------------------------------------------------------------

void
wxGridCellAutoWrapStringEditor::Create(wxWindow* parent,
                                       wxWindowID id,
                                       wxEvtHandler* evtHandler)
{
  m_control = new wxTextCtrl(parent, id, wxEmptyString,
                             wxDefaultPosition, wxDefaultSize,
                             wxTE_MULTILINE | wxTE_RICH);


  wxGridCellEditor::Create(parent, id, evtHandler);
}

void
wxGridCellAutoWrapStringRenderer::Draw(wxGrid& grid,
                      wxGridCellAttr& attr,
                      wxDC& dc,
                      const wxRect& rectCell,
                      int row, int col,
                      bool isSelected) {


    wxGridCellRenderer::Draw(grid, attr, dc, rectCell, row, col, isSelected);

    // now we only have to draw the text
    SetTextColoursAndFont(grid, attr, dc, isSelected);

    int horizAlign, vertAlign;
    attr.GetAlignment(&horizAlign, &vertAlign);

    wxRect rect = rectCell;
    rect.Inflate(-1);

    grid.DrawTextRectangle(dc, GetTextLines(grid,dc,attr,rect,row,col),
                           rect, horizAlign, vertAlign);
}


wxArrayString
wxGridCellAutoWrapStringRenderer::GetTextLines(wxGrid& grid,
                                               wxDC& dc,
                                               const wxGridCellAttr& attr,
                                               const wxRect& rect,
                                               int row, int col)
{
    wxString  data = grid.GetCellValue(row, col);

    wxArrayString lines;
    dc.SetFont(attr.GetFont());

    //Taken from wxGrid again!
    wxCoord x = 0, y = 0, curr_x = 0;
    wxCoord max_x = rect.GetWidth();

    dc.SetFont(attr.GetFont());
    wxStringTokenizer tk(data , _T(" \n\t\r"));
    wxString thisline = wxEmptyString;

    while ( tk.HasMoreTokens() )
    {
        wxString tok = tk.GetNextToken();
        //FIXME: this causes us to print an extra unnecesary
        //       space at the end of the line. But it
        //       is invisible , simplifies the size calculation
        //       and ensures tokens are separated in the display
        tok += _T(" ");

        dc.GetTextExtent(tok, &x, &y);
        if ( curr_x + x > max_x)
        {
            if ( curr_x == 0 )
            {
                // this means that a single token is wider than the maximal
                // width -- still use it as is as we need to show at least the
                // part of it which fits
                lines.Add(tok);
            }
            else
            {
                lines.Add(thisline);
                thisline = tok;
                curr_x = x;
            }
        }
        else
        {
            thisline+= tok;
            curr_x += x;
        }
    }
    //Add last line
    lines.Add( wxString(thisline) );

    return lines;
}


wxSize
wxGridCellAutoWrapStringRenderer::GetBestSize(wxGrid& grid,
                                              wxGridCellAttr& attr,
                                              wxDC& dc,
                                              int row, int col)
{
    wxCoord x,y, height , width = grid.GetColSize(col) -20;
    // for width, subtract 20 because ColSize includes a magin of 10 pixels
    // that we do not want here and because we always start with an increment
    // by 10 in the loop below.
    int count = 250; //Limit iterations..

    wxRect rect(0,0,width,10);

    // M is a nice large character 'y' gives descender!.
    dc.GetTextExtent(wxT("My"), &x, &y);

    do
    {
        width+=10;
        rect.SetWidth(width);
        height = y * (wx_truncate_cast(wxCoord, GetTextLines(grid,dc,attr,rect,row,col).GetCount()));
        count--;
    // Search for a shape no taller than the golden ratio.
    } while (count && (width  < (height*1.68)) );


    return wxSize(width,height);
}

#endif // wxUSE_GRID

