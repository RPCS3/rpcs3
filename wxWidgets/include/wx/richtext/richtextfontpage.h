/////////////////////////////////////////////////////////////////////////////
// Name:        wx/richtext/richeditfontpage.h
// Purpose:     Font page for wxRichTextFormattingDialog
// Author:      Julian Smart
// Modified by:
// Created:     2006-10-02
// RCS-ID:      $Id: richtextfontpage.h 60641 2009-05-15 11:22:40Z JS $
// Copyright:   (c) Julian Smart
// Licence:     wxWindows licence
/////////////////////////////////////////////////////////////////////////////

#ifndef _RICHTEXTFONTPAGE_H_
#define _RICHTEXTFONTPAGE_H_

/*!
 * Includes
 */

////@begin includes
////@end includes

/*!
 * Forward declarations
 */

////@begin forward declarations
class wxRichTextFontListBox;
class wxRichTextColourSwatchCtrl;
class wxRichTextFontPreviewCtrl;
////@end forward declarations

/*!
 * Control identifiers
 */

////@begin control identifiers
#define SYMBOL_WXRICHTEXTFONTPAGE_STYLE wxRESIZE_BORDER|wxTAB_TRAVERSAL
#define SYMBOL_WXRICHTEXTFONTPAGE_TITLE _("wxRichTextFontPage")
#define SYMBOL_WXRICHTEXTFONTPAGE_IDNAME ID_RICHTEXTFONTPAGE
#define SYMBOL_WXRICHTEXTFONTPAGE_SIZE wxSize(400, 300)
#define SYMBOL_WXRICHTEXTFONTPAGE_POSITION wxDefaultPosition
////@end control identifiers

/*!
 * wxRichTextFontPage class declaration
 */

class WXDLLIMPEXP_RICHTEXT wxRichTextFontPage: public wxPanel
{
    DECLARE_DYNAMIC_CLASS( wxRichTextFontPage )
    DECLARE_EVENT_TABLE()

public:
    /// Constructors
    wxRichTextFontPage( );
    wxRichTextFontPage( wxWindow* parent, wxWindowID id = wxID_ANY, const wxPoint& pos = SYMBOL_WXRICHTEXTFONTPAGE_POSITION, const wxSize& size = SYMBOL_WXRICHTEXTFONTPAGE_SIZE, long style = SYMBOL_WXRICHTEXTFONTPAGE_STYLE );

    /// Initialise members
    void Init();

    /// Creation
    bool Create( wxWindow* parent, wxWindowID id = wxID_ANY, const wxPoint& pos = SYMBOL_WXRICHTEXTFONTPAGE_POSITION, const wxSize& size = SYMBOL_WXRICHTEXTFONTPAGE_SIZE, long style = SYMBOL_WXRICHTEXTFONTPAGE_STYLE );

    /// Creates the controls and sizers
    void CreateControls();

    /// Transfer data from/to window
    virtual bool TransferDataFromWindow();
    virtual bool TransferDataToWindow();

    /// Updates the font preview
    void UpdatePreview();

    void OnFaceListBoxSelected( wxCommandEvent& event );
    void OnColourClicked( wxCommandEvent& event );

    /// Gets the attributes associated with the main formatting dialog
    wxTextAttrEx* GetAttributes();

////@begin wxRichTextFontPage event handler declarations

    /// wxEVT_COMMAND_TEXT_UPDATED event handler for ID_RICHTEXTFONTPAGE_FACETEXTCTRL
    void OnFaceTextCtrlUpdated( wxCommandEvent& event );

    /// wxEVT_COMMAND_TEXT_UPDATED event handler for ID_RICHTEXTFONTPAGE_SIZETEXTCTRL
    void OnSizeTextCtrlUpdated( wxCommandEvent& event );

    /// wxEVT_COMMAND_LISTBOX_SELECTED event handler for ID_RICHTEXTFONTPAGE_SIZELISTBOX
    void OnSizeListBoxSelected( wxCommandEvent& event );

    /// wxEVT_COMMAND_COMBOBOX_SELECTED event handler for ID_RICHTEXTFONTPAGE_STYLECTRL
    void OnStyleCtrlSelected( wxCommandEvent& event );

    /// wxEVT_COMMAND_COMBOBOX_SELECTED event handler for ID_RICHTEXTFONTPAGE_WEIGHTCTRL
    void OnWeightCtrlSelected( wxCommandEvent& event );

    /// wxEVT_COMMAND_COMBOBOX_SELECTED event handler for ID_RICHTEXTFONTPAGE_UNDERLINING_CTRL
    void OnUnderliningCtrlSelected( wxCommandEvent& event );

    /// wxEVT_COMMAND_CHECKBOX_CLICKED event handler for ID_RICHTEXTFONTPAGE_STRIKETHROUGHCTRL
    void OnStrikethroughctrlClick( wxCommandEvent& event );

    /// wxEVT_COMMAND_CHECKBOX_CLICKED event handler for ID_RICHTEXTFONTPAGE_CAPSCTRL
    void OnCapsctrlClick( wxCommandEvent& event );

    /// wxEVT_COMMAND_CHECKBOX_CLICKED event handler for ID_RICHTEXTFONTPAGE_SUPERSCRIPT
    void OnRichtextfontpageSuperscriptClick( wxCommandEvent& event );

    /// wxEVT_COMMAND_CHECKBOX_CLICKED event handler for ID_RICHTEXTFONTPAGE_SUBSCRIPT
    void OnRichtextfontpageSubscriptClick( wxCommandEvent& event );

////@end wxRichTextFontPage event handler declarations

////@begin wxRichTextFontPage member function declarations

    /// Retrieves bitmap resources
    wxBitmap GetBitmapResource( const wxString& name );

    /// Retrieves icon resources
    wxIcon GetIconResource( const wxString& name );
////@end wxRichTextFontPage member function declarations

    /// Should we show tooltips?
    static bool ShowToolTips();

////@begin wxRichTextFontPage member variables
    wxTextCtrl* m_faceTextCtrl;
    wxRichTextFontListBox* m_faceListBox;
    wxTextCtrl* m_sizeTextCtrl;
    wxListBox* m_sizeListBox;
    wxComboBox* m_styleCtrl;
    wxComboBox* m_weightCtrl;
    wxComboBox* m_underliningCtrl;
    wxRichTextColourSwatchCtrl* m_colourCtrl;
    wxRichTextColourSwatchCtrl* m_bgColourCtrl;
    wxCheckBox* m_strikethroughCtrl;
    wxCheckBox* m_capitalsCtrl;
    wxCheckBox* m_superscriptCtrl;
    wxCheckBox* m_subscriptCtrl;
    wxRichTextFontPreviewCtrl* m_previewCtrl;
    /// Control identifiers
    enum {
        ID_RICHTEXTFONTPAGE = 10000,
        ID_RICHTEXTFONTPAGE_FACETEXTCTRL = 10001,
        ID_RICHTEXTFONTPAGE_FACELISTBOX = 10002,
        ID_RICHTEXTFONTPAGE_SIZETEXTCTRL = 10005,
        ID_RICHTEXTFONTPAGE_SIZELISTBOX = 10006,
        ID_RICHTEXTFONTPAGE_STYLECTRL = 10007,
        ID_RICHTEXTFONTPAGE_WEIGHTCTRL = 10004,
        ID_RICHTEXTFONTPAGE_UNDERLINING_CTRL = 10008,
        ID_RICHTEXTFONTPAGE_COLOURCTRL = 10009,
        ID_RICHTEXTFONTPAGE_BGCOLOURCTRL = 10014,
        ID_RICHTEXTFONTPAGE_STRIKETHROUGHCTRL = 10010,
        ID_RICHTEXTFONTPAGE_CAPSCTRL = 10011,
        ID_RICHTEXTFONTPAGE_SUPERSCRIPT = 10012,
        ID_RICHTEXTFONTPAGE_SUBSCRIPT = 10013,
        ID_RICHTEXTFONTPAGE_PREVIEWCTRL = 10003
    };
////@end wxRichTextFontPage member variables

    bool m_dontUpdate;
    bool m_colourPresent;
    bool m_bgColourPresent;
};

#endif
    // _RICHTEXTFONTPAGE_H_
