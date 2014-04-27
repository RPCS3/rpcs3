/////////////////////////////////////////////////////////////////////////////
// Name:        wx/richtext/richtextliststylepage.h
// Purpose:
// Author:      Julian Smart
// Modified by:
// Created:     10/18/2006 11:36:37 AM
// RCS-ID:      $Id: richtextliststylepage.h 42678 2006-10-29 22:01:06Z JS $
// Copyright:   (c) Julian Smart
// Licence:     wxWindows licence
/////////////////////////////////////////////////////////////////////////////

#ifndef _RICHTEXTLISTSTYLEPAGE_H_
#define _RICHTEXTLISTSTYLEPAGE_H_

/*!
 * Includes
 */

////@begin includes
#include "wx/spinctrl.h"
#include "wx/notebook.h"
#include "wx/statline.h"
////@end includes

/*!
 * Control identifiers
 */

////@begin control identifiers
#define SYMBOL_WXRICHTEXTLISTSTYLEPAGE_STYLE wxRESIZE_BORDER|wxTAB_TRAVERSAL
#define SYMBOL_WXRICHTEXTLISTSTYLEPAGE_TITLE _("wxRichTextListStylePage")
#define SYMBOL_WXRICHTEXTLISTSTYLEPAGE_IDNAME ID_RICHTEXTLISTSTYLEPAGE
#define SYMBOL_WXRICHTEXTLISTSTYLEPAGE_SIZE wxSize(400, 300)
#define SYMBOL_WXRICHTEXTLISTSTYLEPAGE_POSITION wxDefaultPosition
////@end control identifiers

/*!
 * wxRichTextListStylePage class declaration
 */

class WXDLLIMPEXP_RICHTEXT wxRichTextListStylePage: public wxPanel
{
    DECLARE_DYNAMIC_CLASS( wxRichTextListStylePage )
    DECLARE_EVENT_TABLE()

public:
    /// Constructors
    wxRichTextListStylePage( );
    wxRichTextListStylePage( wxWindow* parent, wxWindowID id = wxID_ANY, const wxPoint& pos = SYMBOL_WXRICHTEXTLISTSTYLEPAGE_POSITION, const wxSize& size = SYMBOL_WXRICHTEXTLISTSTYLEPAGE_SIZE, long style = SYMBOL_WXRICHTEXTLISTSTYLEPAGE_STYLE );

    /// Creation
    bool Create( wxWindow* parent, wxWindowID id = wxID_ANY, const wxPoint& pos = SYMBOL_WXRICHTEXTLISTSTYLEPAGE_POSITION, const wxSize& size = SYMBOL_WXRICHTEXTLISTSTYLEPAGE_SIZE, long style = SYMBOL_WXRICHTEXTLISTSTYLEPAGE_STYLE );

    /// Initialises member variables
    void Init();

    /// Creates the controls and sizers
    void CreateControls();

    /// Updates the bullets preview
    void UpdatePreview();

    /// Transfer data from/to window
    virtual bool TransferDataFromWindow();
    virtual bool TransferDataToWindow();

    /// Get attributes for selected level
    wxRichTextAttr* GetAttributesForSelection();

    /// Update for symbol-related controls
    void OnSymbolUpdate( wxUpdateUIEvent& event );

    /// Update for number-related controls
    void OnNumberUpdate( wxUpdateUIEvent& event );

    /// Update for standard bullet-related controls
    void OnStandardBulletUpdate( wxUpdateUIEvent& event );

    /// Just transfer to the window
    void DoTransferDataToWindow();

    /// Transfer from the window and preview
    void TransferAndPreview();

////@begin wxRichTextListStylePage event handler declarations

    /// wxEVT_COMMAND_SPINCTRL_UPDATED event handler for ID_RICHTEXTLISTSTYLEPAGE_LEVEL
    void OnLevelUpdated( wxSpinEvent& event );

    /// wxEVT_SCROLL_LINEUP event handler for ID_RICHTEXTLISTSTYLEPAGE_LEVEL
    void OnLevelUp( wxSpinEvent& event );

    /// wxEVT_SCROLL_LINEDOWN event handler for ID_RICHTEXTLISTSTYLEPAGE_LEVEL
    void OnLevelDown( wxSpinEvent& event );

    /// wxEVT_COMMAND_TEXT_UPDATED event handler for ID_RICHTEXTLISTSTYLEPAGE_LEVEL
    void OnLevelTextUpdated( wxCommandEvent& event );

    /// wxEVT_UPDATE_UI event handler for ID_RICHTEXTLISTSTYLEPAGE_LEVEL
    void OnLevelUIUpdate( wxUpdateUIEvent& event );

    /// wxEVT_COMMAND_BUTTON_CLICKED event handler for ID_RICHTEXTLISTSTYLEPAGE_CHOOSE_FONT
    void OnChooseFontClick( wxCommandEvent& event );

    /// wxEVT_COMMAND_LISTBOX_SELECTED event handler for ID_RICHTEXTLISTSTYLEPAGE_STYLELISTBOX
    void OnStylelistboxSelected( wxCommandEvent& event );

    /// wxEVT_COMMAND_CHECKBOX_CLICKED event handler for ID_RICHTEXTLISTSTYLEPAGE_PERIODCTRL
    void OnPeriodctrlClick( wxCommandEvent& event );

    /// wxEVT_UPDATE_UI event handler for ID_RICHTEXTLISTSTYLEPAGE_PERIODCTRL
    void OnPeriodctrlUpdate( wxUpdateUIEvent& event );

    /// wxEVT_COMMAND_CHECKBOX_CLICKED event handler for ID_RICHTEXTLISTSTYLEPAGE_PARENTHESESCTRL
    void OnParenthesesctrlClick( wxCommandEvent& event );

    /// wxEVT_UPDATE_UI event handler for ID_RICHTEXTLISTSTYLEPAGE_PARENTHESESCTRL
    void OnParenthesesctrlUpdate( wxUpdateUIEvent& event );

    /// wxEVT_COMMAND_CHECKBOX_CLICKED event handler for ID_RICHTEXTLISTSTYLEPAGE_RIGHTPARENTHESISCTRL
    void OnRightParenthesisCtrlClick( wxCommandEvent& event );

    /// wxEVT_UPDATE_UI event handler for ID_RICHTEXTLISTSTYLEPAGE_RIGHTPARENTHESISCTRL
    void OnRightParenthesisCtrlUpdate( wxUpdateUIEvent& event );

    /// wxEVT_COMMAND_COMBOBOX_SELECTED event handler for ID_RICHTEXTLISTSTYLEPAGE_BULLETALIGNMENTCTRL
    void OnBulletAlignmentCtrlSelected( wxCommandEvent& event );

    /// wxEVT_UPDATE_UI event handler for ID_RICHTEXTLISTSTYLEPAGE_SYMBOLSTATIC
    void OnSymbolstaticUpdate( wxUpdateUIEvent& event );

    /// wxEVT_COMMAND_COMBOBOX_SELECTED event handler for ID_RICHTEXTLISTSTYLEPAGE_SYMBOLCTRL
    void OnSymbolctrlSelected( wxCommandEvent& event );

    /// wxEVT_COMMAND_TEXT_UPDATED event handler for ID_RICHTEXTLISTSTYLEPAGE_SYMBOLCTRL
    void OnSymbolctrlUpdated( wxCommandEvent& event );

    /// wxEVT_UPDATE_UI event handler for ID_RICHTEXTLISTSTYLEPAGE_SYMBOLCTRL
    void OnSymbolctrlUIUpdate( wxUpdateUIEvent& event );

    /// wxEVT_COMMAND_BUTTON_CLICKED event handler for ID_RICHTEXTLISTSTYLEPAGE_CHOOSE_SYMBOL
    void OnChooseSymbolClick( wxCommandEvent& event );

    /// wxEVT_UPDATE_UI event handler for ID_RICHTEXTLISTSTYLEPAGE_CHOOSE_SYMBOL
    void OnChooseSymbolUpdate( wxUpdateUIEvent& event );

    /// wxEVT_COMMAND_COMBOBOX_SELECTED event handler for ID_RICHTEXTLISTSTYLEPAGE_SYMBOLFONTCTRL
    void OnSymbolfontctrlSelected( wxCommandEvent& event );

    /// wxEVT_COMMAND_TEXT_UPDATED event handler for ID_RICHTEXTLISTSTYLEPAGE_SYMBOLFONTCTRL
    void OnSymbolfontctrlUpdated( wxCommandEvent& event );

    /// wxEVT_UPDATE_UI event handler for ID_RICHTEXTLISTSTYLEPAGE_SYMBOLFONTCTRL
    void OnSymbolfontctrlUIUpdate( wxUpdateUIEvent& event );

    /// wxEVT_UPDATE_UI event handler for ID_RICHTEXTLISTSTYLEPAGE_NAMESTATIC
    void OnNamestaticUpdate( wxUpdateUIEvent& event );

    /// wxEVT_COMMAND_COMBOBOX_SELECTED event handler for ID_RICHTEXTLISTSTYLEPAGE_NAMECTRL
    void OnNamectrlSelected( wxCommandEvent& event );

    /// wxEVT_COMMAND_TEXT_UPDATED event handler for ID_RICHTEXTLISTSTYLEPAGE_NAMECTRL
    void OnNamectrlUpdated( wxCommandEvent& event );

    /// wxEVT_UPDATE_UI event handler for ID_RICHTEXTLISTSTYLEPAGE_NAMECTRL
    void OnNamectrlUIUpdate( wxUpdateUIEvent& event );

    /// wxEVT_COMMAND_RADIOBUTTON_SELECTED event handler for ID_RICHTEXTLISTSTYLEPAGE_ALIGNLEFT
    void OnRichtextliststylepageAlignleftSelected( wxCommandEvent& event );

    /// wxEVT_COMMAND_RADIOBUTTON_SELECTED event handler for ID_RICHTEXTLISTSTYLEPAGE_ALIGNRIGHT
    void OnRichtextliststylepageAlignrightSelected( wxCommandEvent& event );

    /// wxEVT_COMMAND_RADIOBUTTON_SELECTED event handler for ID_RICHTEXTLISTSTYLEPAGE_JUSTIFIED
    void OnRichtextliststylepageJustifiedSelected( wxCommandEvent& event );

    /// wxEVT_COMMAND_RADIOBUTTON_SELECTED event handler for ID_RICHTEXTLISTSTYLEPAGE_CENTERED
    void OnRichtextliststylepageCenteredSelected( wxCommandEvent& event );

    /// wxEVT_COMMAND_RADIOBUTTON_SELECTED event handler for ID_RICHTEXTLISTSTYLEPAGE_ALIGNINDETERMINATE
    void OnRichtextliststylepageAlignindeterminateSelected( wxCommandEvent& event );

    /// wxEVT_COMMAND_TEXT_UPDATED event handler for ID_RICHTEXTLISTSTYLEPAGE_INDENTLEFT
    void OnIndentLeftUpdated( wxCommandEvent& event );

    /// wxEVT_COMMAND_TEXT_UPDATED event handler for ID_RICHTEXTLISTSTYLEPAGE_INDENTFIRSTLINE
    void OnIndentFirstLineUpdated( wxCommandEvent& event );

    /// wxEVT_COMMAND_TEXT_UPDATED event handler for ID_RICHTEXTLISTSTYLEPAGE_INDENTRIGHT
    void OnIndentRightUpdated( wxCommandEvent& event );

    /// wxEVT_COMMAND_TEXT_UPDATED event handler for ID_RICHTEXTLISTSTYLEPAGE_SPACINGBEFORE
    void OnSpacingBeforeUpdated( wxCommandEvent& event );

    /// wxEVT_COMMAND_TEXT_UPDATED event handler for ID_RICHTEXTLISTSTYLEPAGE_SPACINGAFTER
    void OnSpacingAfterUpdated( wxCommandEvent& event );

    /// wxEVT_COMMAND_COMBOBOX_SELECTED event handler for ID_RICHTEXTLISTSTYLEPAGE_LINESPACING
    void OnLineSpacingSelected( wxCommandEvent& event );

////@end wxRichTextListStylePage event handler declarations

////@begin wxRichTextListStylePage member function declarations

    /// Retrieves bitmap resources
    wxBitmap GetBitmapResource( const wxString& name );

    /// Retrieves icon resources
    wxIcon GetIconResource( const wxString& name );
////@end wxRichTextListStylePage member function declarations

    /// Should we show tooltips?
    static bool ShowToolTips();

////@begin wxRichTextListStylePage member variables
    wxSpinCtrl* m_levelCtrl;
    wxListBox* m_styleListBox;
    wxCheckBox* m_periodCtrl;
    wxCheckBox* m_parenthesesCtrl;
    wxCheckBox* m_rightParenthesisCtrl;
    wxComboBox* m_bulletAlignmentCtrl;
    wxComboBox* m_symbolCtrl;
    wxComboBox* m_symbolFontCtrl;
    wxComboBox* m_bulletNameCtrl;
    wxRadioButton* m_alignmentLeft;
    wxRadioButton* m_alignmentRight;
    wxRadioButton* m_alignmentJustified;
    wxRadioButton* m_alignmentCentred;
    wxRadioButton* m_alignmentIndeterminate;
    wxTextCtrl* m_indentLeft;
    wxTextCtrl* m_indentLeftFirst;
    wxTextCtrl* m_indentRight;
    wxTextCtrl* m_spacingBefore;
    wxTextCtrl* m_spacingAfter;
    wxComboBox* m_spacingLine;
    wxRichTextCtrl* m_previewCtrl;
    /// Control identifiers
    enum {
        ID_RICHTEXTLISTSTYLEPAGE = 10616,
        ID_RICHTEXTLISTSTYLEPAGE_LEVEL = 10617,
        ID_RICHTEXTLISTSTYLEPAGE_CHOOSE_FONT = 10604,
        ID_RICHTEXTLISTSTYLEPAGE_NOTEBOOK = 10618,
        ID_RICHTEXTLISTSTYLEPAGE_BULLETS = 10619,
        ID_RICHTEXTLISTSTYLEPAGE_STYLELISTBOX = 10620,
        ID_RICHTEXTLISTSTYLEPAGE_PERIODCTRL = 10627,
        ID_RICHTEXTLISTSTYLEPAGE_PARENTHESESCTRL = 10626,
        ID_RICHTEXTLISTSTYLEPAGE_RIGHTPARENTHESISCTRL = 10602,
        ID_RICHTEXTLISTSTYLEPAGE_BULLETALIGNMENTCTRL = 10603,
        ID_RICHTEXTLISTSTYLEPAGE_SYMBOLSTATIC = 10621,
        ID_RICHTEXTLISTSTYLEPAGE_SYMBOLCTRL = 10622,
        ID_RICHTEXTLISTSTYLEPAGE_CHOOSE_SYMBOL = 10623,
        ID_RICHTEXTLISTSTYLEPAGE_SYMBOLFONTCTRL = 10625,
        ID_RICHTEXTLISTSTYLEPAGE_NAMESTATIC = 10600,
        ID_RICHTEXTLISTSTYLEPAGE_NAMECTRL = 10601,
        ID_RICHTEXTLISTSTYLEPAGE_SPACING = 10628,
        ID_RICHTEXTLISTSTYLEPAGE_ALIGNLEFT = 10629,
        ID_RICHTEXTLISTSTYLEPAGE_ALIGNRIGHT = 10630,
        ID_RICHTEXTLISTSTYLEPAGE_JUSTIFIED = 10631,
        ID_RICHTEXTLISTSTYLEPAGE_CENTERED = 10632,
        ID_RICHTEXTLISTSTYLEPAGE_ALIGNINDETERMINATE = 10633,
        ID_RICHTEXTLISTSTYLEPAGE_INDENTLEFT = 10634,
        ID_RICHTEXTLISTSTYLEPAGE_INDENTFIRSTLINE = 10635,
        ID_RICHTEXTLISTSTYLEPAGE_INDENTRIGHT = 10636,
        ID_RICHTEXTLISTSTYLEPAGE_SPACINGBEFORE = 10637,
        ID_RICHTEXTLISTSTYLEPAGE_SPACINGAFTER = 10638,
        ID_RICHTEXTLISTSTYLEPAGE_LINESPACING = 10639,
        ID_RICHTEXTLISTSTYLEPAGE_RICHTEXTCTRL = 10640
    };
////@end wxRichTextListStylePage member variables

    bool m_dontUpdate;
    int m_currentLevel;
};

#endif
    // _RICHTEXTLISTSTYLEPAGE_H_
