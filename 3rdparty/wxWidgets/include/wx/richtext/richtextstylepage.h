/////////////////////////////////////////////////////////////////////////////
// Name:        wx/richtext/richtextstylepage.h
// Purpose:
// Author:      Julian Smart
// Modified by:
// Created:     10/5/2006 11:34:55 AM
// RCS-ID:      $Id: richtextstylepage.h 42678 2006-10-29 22:01:06Z JS $
// Copyright:   (c) Julian Smart
// Licence:     wxWindows licence
/////////////////////////////////////////////////////////////////////////////

#ifndef _RICHTEXTSTYLEPAGE_H_
#define _RICHTEXTSTYLEPAGE_H_

/*!
 * Control identifiers
 */

////@begin control identifiers
#define SYMBOL_WXRICHTEXTSTYLEPAGE_STYLE wxRESIZE_BORDER|wxTAB_TRAVERSAL
#define SYMBOL_WXRICHTEXTSTYLEPAGE_TITLE _("wxRichTextStylePage")
#define SYMBOL_WXRICHTEXTSTYLEPAGE_IDNAME ID_RICHTEXTSTYLEPAGE
#define SYMBOL_WXRICHTEXTSTYLEPAGE_SIZE wxSize(400, 300)
#define SYMBOL_WXRICHTEXTSTYLEPAGE_POSITION wxDefaultPosition
////@end control identifiers

/*!
 * wxRichTextStylePage class declaration
 */

class wxRichTextStylePage: public wxPanel
{
    DECLARE_DYNAMIC_CLASS( wxRichTextStylePage )
    DECLARE_EVENT_TABLE()

public:
    /// Constructors
    wxRichTextStylePage( );
    wxRichTextStylePage( wxWindow* parent, wxWindowID id = wxID_ANY, const wxPoint& pos = SYMBOL_WXRICHTEXTSTYLEPAGE_POSITION, const wxSize& size = SYMBOL_WXRICHTEXTSTYLEPAGE_SIZE, long style = SYMBOL_WXRICHTEXTSTYLEPAGE_STYLE );

    /// Creation
    bool Create( wxWindow* parent, wxWindowID id = wxID_ANY, const wxPoint& pos = SYMBOL_WXRICHTEXTSTYLEPAGE_POSITION, const wxSize& size = SYMBOL_WXRICHTEXTSTYLEPAGE_SIZE, long style = SYMBOL_WXRICHTEXTSTYLEPAGE_STYLE );

    /// Initialise members
    void Init();

    /// Creates the controls and sizers
    void CreateControls();

    /// Transfer data from/to window
    virtual bool TransferDataFromWindow();
    virtual bool TransferDataToWindow();

    /// Gets the attributes associated with the main formatting dialog
    wxTextAttrEx* GetAttributes();

////@begin wxRichTextStylePage event handler declarations

    /// wxEVT_UPDATE_UI event handler for ID_RICHTEXTSTYLEPAGE_NEXT_STYLE
    void OnNextStyleUpdate( wxUpdateUIEvent& event );

////@end wxRichTextStylePage event handler declarations

////@begin wxRichTextStylePage member function declarations

    /// Retrieves bitmap resources
    wxBitmap GetBitmapResource( const wxString& name );

    /// Retrieves icon resources
    wxIcon GetIconResource( const wxString& name );
////@end wxRichTextStylePage member function declarations

    /// Should we show tooltips?
    static bool ShowToolTips();

////@begin wxRichTextStylePage member variables
    wxTextCtrl* m_styleName;
    wxComboBox* m_basedOn;
    wxComboBox* m_nextStyle;
    /// Control identifiers
    enum {
        ID_RICHTEXTSTYLEPAGE = 10403,
        ID_RICHTEXTSTYLEPAGE_STYLE_NAME = 10404,
        ID_RICHTEXTSTYLEPAGE_BASED_ON = 10405,
        ID_RICHTEXTSTYLEPAGE_NEXT_STYLE = 10406
    };
////@end wxRichTextStylePage member variables
};

#endif
    // _RICHTEXTSTYLEPAGE_H_
