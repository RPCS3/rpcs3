/////////////////////////////////////////////////////////////////////////////
// Name:        wx/richtext/richtextbulletspage.h
// Purpose:
// Author:      Julian Smart
// Modified by:
// Created:     10/4/2006 10:32:31 AM
// RCS-ID:      $Id: richtextbulletspage.h 50950 2007-12-30 16:11:12Z JS $
// Copyright:   (c) Julian Smart
// Licence:     wxWindows licence
/////////////////////////////////////////////////////////////////////////////

#ifndef _RICHTEXTBULLETSPAGE_H_
#define _RICHTEXTBULLETSPAGE_H_

/*!
 * Includes
 */

/*!
 * Forward declarations
 */

////@begin forward declarations
class wxSpinCtrl;
class wxRichTextCtrl;
////@end forward declarations

/*!
 * Control identifiers
 */

////@begin control identifiers
#define SYMBOL_WXRICHTEXTBULLETSPAGE_STYLE wxRESIZE_BORDER|wxTAB_TRAVERSAL
#define SYMBOL_WXRICHTEXTBULLETSPAGE_TITLE _("wxRichTextBulletsPage")
#define SYMBOL_WXRICHTEXTBULLETSPAGE_IDNAME ID_RICHTEXTBULLETSPAGE
#define SYMBOL_WXRICHTEXTBULLETSPAGE_SIZE wxSize(400, 300)
#define SYMBOL_WXRICHTEXTBULLETSPAGE_POSITION wxDefaultPosition
////@end control identifiers

/*!
 * wxRichTextBulletsPage class declaration
 */

class WXDLLIMPEXP_RICHTEXT wxRichTextBulletsPage: public wxPanel
{
    DECLARE_DYNAMIC_CLASS( wxRichTextBulletsPage )
    DECLARE_EVENT_TABLE()

public:
    /// Constructors
    wxRichTextBulletsPage( );
    wxRichTextBulletsPage( wxWindow* parent, wxWindowID id = wxID_ANY, const wxPoint& pos = SYMBOL_WXRICHTEXTBULLETSPAGE_POSITION, const wxSize& size = SYMBOL_WXRICHTEXTBULLETSPAGE_SIZE, long style = SYMBOL_WXRICHTEXTBULLETSPAGE_STYLE );

    /// Creation
    bool Create( wxWindow* parent, wxWindowID id = wxID_ANY, const wxPoint& pos = SYMBOL_WXRICHTEXTBULLETSPAGE_POSITION, const wxSize& size = SYMBOL_WXRICHTEXTBULLETSPAGE_SIZE, long style = SYMBOL_WXRICHTEXTBULLETSPAGE_STYLE );

    /// Initialise members
    void Init();

    /// Creates the controls and sizers
    void CreateControls();

    /// Updates the bullets preview
    void UpdatePreview();

    /// Transfer data from/to window
    virtual bool TransferDataFromWindow();
    virtual bool TransferDataToWindow();

    /// Gets the attributes associated with the main formatting dialog
    wxTextAttrEx* GetAttributes();

    /// Update for symbol-related controls
    void OnSymbolUpdate( wxUpdateUIEvent& event );

    /// Update for number-related controls
    void OnNumberUpdate( wxUpdateUIEvent& event );

    /// Update for standard bullet-related controls
    void OnStandardBulletUpdate( wxUpdateUIEvent& event );

////@begin wxRichTextBulletsPage event handler declarations

    /// wxEVT_COMMAND_LISTBOX_SELECTED event handler for ID_RICHTEXTBULLETSPAGE_STYLELISTBOX
    void OnStylelistboxSelected( wxCommandEvent& event );

    /// wxEVT_COMMAND_CHECKBOX_CLICKED event handler for ID_RICHTEXTBULLETSPAGE_PERIODCTRL
    void OnPeriodctrlClick( wxCommandEvent& event );

    /// wxEVT_UPDATE_UI event handler for ID_RICHTEXTBULLETSPAGE_PERIODCTRL
    void OnPeriodctrlUpdate( wxUpdateUIEvent& event );

    /// wxEVT_COMMAND_CHECKBOX_CLICKED event handler for ID_RICHTEXTBULLETSPAGE_PARENTHESESCTRL
    void OnParenthesesctrlClick( wxCommandEvent& event );

    /// wxEVT_UPDATE_UI event handler for ID_RICHTEXTBULLETSPAGE_PARENTHESESCTRL
    void OnParenthesesctrlUpdate( wxUpdateUIEvent& event );

    /// wxEVT_COMMAND_CHECKBOX_CLICKED event handler for ID_RICHTEXTBULLETSPAGE_RIGHTPARENTHESISCTRL
    void OnRightParenthesisCtrlClick( wxCommandEvent& event );

    /// wxEVT_UPDATE_UI event handler for ID_RICHTEXTBULLETSPAGE_RIGHTPARENTHESISCTRL
    void OnRightParenthesisCtrlUpdate( wxUpdateUIEvent& event );

    /// wxEVT_COMMAND_COMBOBOX_SELECTED event handler for ID_RICHTEXTBULLETSPAGE_BULLETALIGNMENTCTRL
    void OnBulletAlignmentCtrlSelected( wxCommandEvent& event );

    /// wxEVT_UPDATE_UI event handler for ID_RICHTEXTBULLETSPAGE_SYMBOLSTATIC
    void OnSymbolstaticUpdate( wxUpdateUIEvent& event );

    /// wxEVT_COMMAND_COMBOBOX_SELECTED event handler for ID_RICHTEXTBULLETSPAGE_SYMBOLCTRL
    void OnSymbolctrlSelected( wxCommandEvent& event );

    /// wxEVT_COMMAND_TEXT_UPDATED event handler for ID_RICHTEXTBULLETSPAGE_SYMBOLCTRL
    void OnSymbolctrlUpdated( wxCommandEvent& event );

    /// wxEVT_UPDATE_UI event handler for ID_RICHTEXTBULLETSPAGE_SYMBOLCTRL
    void OnSymbolctrlUpdate( wxUpdateUIEvent& event );

    /// wxEVT_COMMAND_BUTTON_CLICKED event handler for ID_RICHTEXTBULLETSPAGE_CHOOSE_SYMBOL
    void OnChooseSymbolClick( wxCommandEvent& event );

    /// wxEVT_UPDATE_UI event handler for ID_RICHTEXTBULLETSPAGE_CHOOSE_SYMBOL
    void OnChooseSymbolUpdate( wxUpdateUIEvent& event );

    /// wxEVT_COMMAND_COMBOBOX_SELECTED event handler for ID_RICHTEXTBULLETSPAGE_SYMBOLFONTCTRL
    void OnSymbolfontctrlSelected( wxCommandEvent& event );

    /// wxEVT_COMMAND_TEXT_UPDATED event handler for ID_RICHTEXTBULLETSPAGE_SYMBOLFONTCTRL
    void OnSymbolfontctrlUpdated( wxCommandEvent& event );

    /// wxEVT_UPDATE_UI event handler for ID_RICHTEXTBULLETSPAGE_SYMBOLFONTCTRL
    void OnSymbolfontctrlUIUpdate( wxUpdateUIEvent& event );

    /// wxEVT_UPDATE_UI event handler for ID_RICHTEXTBULLETSPAGE_NAMESTATIC
    void OnNamestaticUpdate( wxUpdateUIEvent& event );

    /// wxEVT_COMMAND_COMBOBOX_SELECTED event handler for ID_RICHTEXTBULLETSPAGE_NAMECTRL
    void OnNamectrlSelected( wxCommandEvent& event );

    /// wxEVT_COMMAND_TEXT_UPDATED event handler for ID_RICHTEXTBULLETSPAGE_NAMECTRL
    void OnNamectrlUpdated( wxCommandEvent& event );

    /// wxEVT_UPDATE_UI event handler for ID_RICHTEXTBULLETSPAGE_NAMECTRL
    void OnNamectrlUIUpdate( wxUpdateUIEvent& event );

    /// wxEVT_UPDATE_UI event handler for ID_RICHTEXTBULLETSPAGE_NUMBERSTATIC
    void OnNumberstaticUpdate( wxUpdateUIEvent& event );

    /// wxEVT_COMMAND_SPINCTRL_UPDATED event handler for ID_RICHTEXTBULLETSPAGE_NUMBERCTRL
    void OnNumberctrlUpdated( wxSpinEvent& event );

    /// wxEVT_SCROLL_LINEUP event handler for ID_RICHTEXTBULLETSPAGE_NUMBERCTRL
    void OnNumberctrlUp( wxSpinEvent& event );

    /// wxEVT_SCROLL_LINEDOWN event handler for ID_RICHTEXTBULLETSPAGE_NUMBERCTRL
    void OnNumberctrlDown( wxSpinEvent& event );

    /// wxEVT_COMMAND_TEXT_UPDATED event handler for ID_RICHTEXTBULLETSPAGE_NUMBERCTRL
    void OnNumberctrlTextUpdated( wxCommandEvent& event );

    /// wxEVT_UPDATE_UI event handler for ID_RICHTEXTBULLETSPAGE_NUMBERCTRL
    void OnNumberctrlUpdate( wxUpdateUIEvent& event );

////@end wxRichTextBulletsPage event handler declarations

////@begin wxRichTextBulletsPage member function declarations

    /// Retrieves bitmap resources
    wxBitmap GetBitmapResource( const wxString& name );

    /// Retrieves icon resources
    wxIcon GetIconResource( const wxString& name );
////@end wxRichTextBulletsPage member function declarations

    /// Should we show tooltips?
    static bool ShowToolTips();

////@begin wxRichTextBulletsPage member variables
    wxListBox* m_styleListBox;
    wxCheckBox* m_periodCtrl;
    wxCheckBox* m_parenthesesCtrl;
    wxCheckBox* m_rightParenthesisCtrl;
    wxComboBox* m_bulletAlignmentCtrl;
    wxComboBox* m_symbolCtrl;
    wxComboBox* m_symbolFontCtrl;
    wxComboBox* m_bulletNameCtrl;
    wxSpinCtrl* m_numberCtrl;
    wxRichTextCtrl* m_previewCtrl;
    /// Control identifiers
    enum {
        ID_RICHTEXTBULLETSPAGE = 10300,
        ID_RICHTEXTBULLETSPAGE_STYLELISTBOX = 10305,
        ID_RICHTEXTBULLETSPAGE_PERIODCTRL = 10313,
        ID_RICHTEXTBULLETSPAGE_PARENTHESESCTRL = 10311,
        ID_RICHTEXTBULLETSPAGE_RIGHTPARENTHESISCTRL = 10306,
        ID_RICHTEXTBULLETSPAGE_BULLETALIGNMENTCTRL = 10315,
        ID_RICHTEXTBULLETSPAGE_SYMBOLSTATIC = 10301,
        ID_RICHTEXTBULLETSPAGE_SYMBOLCTRL = 10307,
        ID_RICHTEXTBULLETSPAGE_CHOOSE_SYMBOL = 10308,
        ID_RICHTEXTBULLETSPAGE_SYMBOLFONTCTRL = 10309,
        ID_RICHTEXTBULLETSPAGE_NAMESTATIC = 10303,
        ID_RICHTEXTBULLETSPAGE_NAMECTRL = 10304,
        ID_RICHTEXTBULLETSPAGE_NUMBERSTATIC = 10302,
        ID_RICHTEXTBULLETSPAGE_NUMBERCTRL = 10310,
        ID_RICHTEXTBULLETSPAGE_PREVIEW_CTRL = 10314
    };
////@end wxRichTextBulletsPage member variables

    bool m_hasBulletStyle;
    bool m_hasBulletNumber;
    bool m_hasBulletSymbol;
    bool m_dontUpdate;
};

#endif
    // _RICHTEXTBULLETSPAGE_H_
