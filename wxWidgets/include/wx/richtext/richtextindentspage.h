/////////////////////////////////////////////////////////////////////////////
// Name:        wx/richtext/richtextindentspage.h
// Purpose:
// Author:      Julian Smart
// Modified by:
// Created:     10/3/2006 2:28:21 PM
// RCS-ID:      $Id: richtextindentspage.h 43277 2006-11-10 15:48:46Z JS $
// Copyright:   (c) Julian Smart
// Licence:     wxWindows licence
/////////////////////////////////////////////////////////////////////////////

#ifndef _RICHTEXTINDENTSPAGE_H_
#define _RICHTEXTINDENTSPAGE_H_

/*!
 * Includes
 */

////@begin includes
#include "wx/statline.h"
////@end includes

/*!
 * Forward declarations
 */

////@begin forward declarations
class wxRichTextCtrl;
////@end forward declarations

/*!
 * Control identifiers
 */

////@begin control identifiers
#define SYMBOL_WXRICHTEXTINDENTSSPACINGPAGE_STYLE wxRESIZE_BORDER|wxTAB_TRAVERSAL
#define SYMBOL_WXRICHTEXTINDENTSSPACINGPAGE_TITLE _("wxRichTextFontPage")
#define SYMBOL_WXRICHTEXTINDENTSSPACINGPAGE_IDNAME ID_RICHTEXTINDENTSSPACINGPAGE
#define SYMBOL_WXRICHTEXTINDENTSSPACINGPAGE_SIZE wxSize(400, 300)
#define SYMBOL_WXRICHTEXTINDENTSSPACINGPAGE_POSITION wxDefaultPosition
////@end control identifiers

/*!
 * wxRichTextIndentsSpacingPage class declaration
 */

class WXDLLIMPEXP_RICHTEXT wxRichTextIndentsSpacingPage: public wxPanel
{
    DECLARE_DYNAMIC_CLASS( wxRichTextIndentsSpacingPage )
    DECLARE_EVENT_TABLE()

public:
    /// Constructors
    wxRichTextIndentsSpacingPage( );
    wxRichTextIndentsSpacingPage( wxWindow* parent, wxWindowID id = wxID_ANY, const wxPoint& pos = SYMBOL_WXRICHTEXTINDENTSSPACINGPAGE_POSITION, const wxSize& size = SYMBOL_WXRICHTEXTINDENTSSPACINGPAGE_SIZE, long style = SYMBOL_WXRICHTEXTINDENTSSPACINGPAGE_STYLE );

    /// Creation
    bool Create( wxWindow* parent, wxWindowID id = wxID_ANY, const wxPoint& pos = SYMBOL_WXRICHTEXTINDENTSSPACINGPAGE_POSITION, const wxSize& size = SYMBOL_WXRICHTEXTINDENTSSPACINGPAGE_SIZE, long style = SYMBOL_WXRICHTEXTINDENTSSPACINGPAGE_STYLE );

    /// Initialise members
    void Init();

    /// Creates the controls and sizers
    void CreateControls();

    /// Transfer data from/to window
    virtual bool TransferDataFromWindow();
    virtual bool TransferDataToWindow();

    /// Updates the paragraph preview
    void UpdatePreview();

    /// Gets the attributes associated with the main formatting dialog
    wxTextAttrEx* GetAttributes();

////@begin wxRichTextIndentsSpacingPage event handler declarations

    /// wxEVT_COMMAND_RADIOBUTTON_SELECTED event handler for ID_RICHTEXTINDENTSSPACINGPAGE_ALIGNMENT_LEFT
    void OnAlignmentLeftSelected( wxCommandEvent& event );

    /// wxEVT_COMMAND_RADIOBUTTON_SELECTED event handler for ID_RICHTEXTINDENTSSPACINGPAGE_ALIGNMENT_RIGHT
    void OnAlignmentRightSelected( wxCommandEvent& event );

    /// wxEVT_COMMAND_RADIOBUTTON_SELECTED event handler for ID_RICHTEXTINDENTSSPACINGPAGE_ALIGNMENT_JUSTIFIED
    void OnAlignmentJustifiedSelected( wxCommandEvent& event );

    /// wxEVT_COMMAND_RADIOBUTTON_SELECTED event handler for ID_RICHTEXTINDENTSSPACINGPAGE_ALIGNMENT_CENTRED
    void OnAlignmentCentredSelected( wxCommandEvent& event );

    /// wxEVT_COMMAND_RADIOBUTTON_SELECTED event handler for ID_RICHTEXTINDENTSSPACINGPAGE_ALIGNMENT_INDETERMINATE
    void OnAlignmentIndeterminateSelected( wxCommandEvent& event );

    /// wxEVT_COMMAND_TEXT_UPDATED event handler for ID_RICHTEXTINDENTSSPACINGPAGE_INDENT_LEFT
    void OnIndentLeftUpdated( wxCommandEvent& event );

    /// wxEVT_COMMAND_TEXT_UPDATED event handler for ID_RICHTEXTINDENTSSPACINGPAGE_INDENT_LEFT_FIRST
    void OnIndentLeftFirstUpdated( wxCommandEvent& event );

    /// wxEVT_COMMAND_TEXT_UPDATED event handler for ID_RICHTEXTINDENTSSPACINGPAGE_INDENT_RIGHT
    void OnIndentRightUpdated( wxCommandEvent& event );

    /// wxEVT_COMMAND_COMBOBOX_SELECTED event handler for ID_RICHTEXTINDENTSSPACINGPAGE_OUTLINELEVEL
    void OnRichtextOutlinelevelSelected( wxCommandEvent& event );

    /// wxEVT_COMMAND_TEXT_UPDATED event handler for ID_RICHTEXTINDENTSSPACINGPAGE_SPACING_BEFORE
    void OnSpacingBeforeUpdated( wxCommandEvent& event );

    /// wxEVT_COMMAND_TEXT_UPDATED event handler for ID_RICHTEXTINDENTSSPACINGPAGE_SPACING_AFTER
    void OnSpacingAfterUpdated( wxCommandEvent& event );

    /// wxEVT_COMMAND_COMBOBOX_SELECTED event handler for ID_RICHTEXTINDENTSSPACINGPAGE_SPACING_LINE
    void OnSpacingLineSelected( wxCommandEvent& event );

////@end wxRichTextIndentsSpacingPage event handler declarations

////@begin wxRichTextIndentsSpacingPage member function declarations

    /// Retrieves bitmap resources
    wxBitmap GetBitmapResource( const wxString& name );

    /// Retrieves icon resources
    wxIcon GetIconResource( const wxString& name );
////@end wxRichTextIndentsSpacingPage member function declarations

    /// Should we show tooltips?
    static bool ShowToolTips();

////@begin wxRichTextIndentsSpacingPage member variables
    wxRadioButton* m_alignmentLeft;
    wxRadioButton* m_alignmentRight;
    wxRadioButton* m_alignmentJustified;
    wxRadioButton* m_alignmentCentred;
    wxRadioButton* m_alignmentIndeterminate;
    wxTextCtrl* m_indentLeft;
    wxTextCtrl* m_indentLeftFirst;
    wxTextCtrl* m_indentRight;
    wxComboBox* m_outlineLevelCtrl;
    wxTextCtrl* m_spacingBefore;
    wxTextCtrl* m_spacingAfter;
    wxComboBox* m_spacingLine;
    wxRichTextCtrl* m_previewCtrl;
    /// Control identifiers
    enum {
        ID_RICHTEXTINDENTSSPACINGPAGE = 10100,
        ID_RICHTEXTINDENTSSPACINGPAGE_ALIGNMENT_LEFT = 10102,
        ID_RICHTEXTINDENTSSPACINGPAGE_ALIGNMENT_RIGHT = 10110,
        ID_RICHTEXTINDENTSSPACINGPAGE_ALIGNMENT_JUSTIFIED = 10111,
        ID_RICHTEXTINDENTSSPACINGPAGE_ALIGNMENT_CENTRED = 10112,
        ID_RICHTEXTINDENTSSPACINGPAGE_ALIGNMENT_INDETERMINATE = 10101,
        ID_RICHTEXTINDENTSSPACINGPAGE_INDENT_LEFT = 10103,
        ID_RICHTEXTINDENTSSPACINGPAGE_INDENT_LEFT_FIRST = 10104,
        ID_RICHTEXTINDENTSSPACINGPAGE_INDENT_RIGHT = 10113,
        ID_RICHTEXTINDENTSSPACINGPAGE_OUTLINELEVEL = 10105,
        ID_RICHTEXTINDENTSSPACINGPAGE_SPACING_BEFORE = 10114,
        ID_RICHTEXTINDENTSSPACINGPAGE_SPACING_AFTER = 10116,
        ID_RICHTEXTINDENTSSPACINGPAGE_SPACING_LINE = 10115,
        ID_RICHTEXTINDENTSSPACINGPAGE_PREVIEW_CTRL = 10109
    };
////@end wxRichTextIndentsSpacingPage member variables

    bool m_dontUpdate;
};

#endif
    // _RICHTEXTINDENTSPAGE_H_
