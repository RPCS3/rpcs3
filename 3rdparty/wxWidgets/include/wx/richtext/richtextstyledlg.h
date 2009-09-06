/////////////////////////////////////////////////////////////////////////////
// Name:        wx/richtext/richtextstyledlg.h
// Purpose:
// Author:      Julian Smart
// Modified by:
// Created:     10/5/2006 12:05:31 PM
// RCS-ID:      $Id: richtextstyledlg.h 52117 2008-02-26 15:04:54Z JS $
// Copyright:   (c) Julian Smart
// Licence:     wxWindows licence
/////////////////////////////////////////////////////////////////////////////

#ifndef _RICHTEXTSTYLEDLG_H_
#define _RICHTEXTSTYLEDLG_H_

/*!
 * Includes
 */

////@begin includes
////@end includes

#include "wx/richtext/richtextbuffer.h"
#include "wx/richtext/richtextstyles.h"
#include "wx/richtext/richtextctrl.h"

/*!
 * Forward declarations
 */

////@begin forward declarations
class wxBoxSizer;
class wxRichTextStyleListCtrl;
class wxRichTextCtrl;
////@end forward declarations

class WXDLLIMPEXP_FWD_CORE wxButton;
class WXDLLIMPEXP_FWD_CORE wxCheckBox;

/*!
 * Control identifiers
 */

////@begin control identifiers
#define SYMBOL_WXRICHTEXTSTYLEORGANISERDIALOG_STYLE wxDEFAULT_DIALOG_STYLE|wxRESIZE_BORDER|wxSYSTEM_MENU|wxCLOSE_BOX
#define SYMBOL_WXRICHTEXTSTYLEORGANISERDIALOG_TITLE _("Style Organiser")
#define SYMBOL_WXRICHTEXTSTYLEORGANISERDIALOG_IDNAME ID_RICHTEXTSTYLEORGANISERDIALOG
#define SYMBOL_WXRICHTEXTSTYLEORGANISERDIALOG_SIZE wxSize(400, 300)
#define SYMBOL_WXRICHTEXTSTYLEORGANISERDIALOG_POSITION wxDefaultPosition
////@end control identifiers

/*!
 * Flags for specifying permitted operations
 */

#define wxRICHTEXT_ORGANISER_DELETE_STYLES  0x0001
#define wxRICHTEXT_ORGANISER_CREATE_STYLES  0x0002
#define wxRICHTEXT_ORGANISER_APPLY_STYLES   0x0004
#define wxRICHTEXT_ORGANISER_EDIT_STYLES    0x0008
#define wxRICHTEXT_ORGANISER_RENAME_STYLES  0x0010
#define wxRICHTEXT_ORGANISER_OK_CANCEL      0x0020
#define wxRICHTEXT_ORGANISER_RENUMBER       0x0040

// The permitted style types to show
#define wxRICHTEXT_ORGANISER_SHOW_CHARACTER 0x0100
#define wxRICHTEXT_ORGANISER_SHOW_PARAGRAPH 0x0200
#define wxRICHTEXT_ORGANISER_SHOW_LIST      0x0400
#define wxRICHTEXT_ORGANISER_SHOW_ALL       0x0800

// Common combinations
#define wxRICHTEXT_ORGANISER_ORGANISE (wxRICHTEXT_ORGANISER_SHOW_ALL|wxRICHTEXT_ORGANISER_DELETE_STYLES|wxRICHTEXT_ORGANISER_CREATE_STYLES|wxRICHTEXT_ORGANISER_APPLY_STYLES|wxRICHTEXT_ORGANISER_EDIT_STYLES|wxRICHTEXT_ORGANISER_RENAME_STYLES)
#define wxRICHTEXT_ORGANISER_BROWSE (wxRICHTEXT_ORGANISER_SHOW_ALL|wxRICHTEXT_ORGANISER_OK_CANCEL)
#define wxRICHTEXT_ORGANISER_BROWSE_NUMBERING (wxRICHTEXT_ORGANISER_SHOW_LIST|wxRICHTEXT_ORGANISER_OK_CANCEL|wxRICHTEXT_ORGANISER_RENUMBER)

/*!
 * wxRichTextStyleOrganiserDialog class declaration
 */

class WXDLLIMPEXP_RICHTEXT wxRichTextStyleOrganiserDialog: public wxDialog
{
    DECLARE_DYNAMIC_CLASS( wxRichTextStyleOrganiserDialog )
    DECLARE_EVENT_TABLE()

public:
    /// Constructors
    wxRichTextStyleOrganiserDialog( );
    wxRichTextStyleOrganiserDialog( int flags, wxRichTextStyleSheet* sheet, wxRichTextCtrl* ctrl, wxWindow* parent, wxWindowID id = wxID_ANY, const wxString& caption = SYMBOL_WXRICHTEXTSTYLEORGANISERDIALOG_TITLE, const wxPoint& pos = SYMBOL_WXRICHTEXTSTYLEORGANISERDIALOG_POSITION, const wxSize& size = SYMBOL_WXRICHTEXTSTYLEORGANISERDIALOG_SIZE, long style = SYMBOL_WXRICHTEXTSTYLEORGANISERDIALOG_STYLE );

    /// Creation
    bool Create( int flags, wxRichTextStyleSheet* sheet, wxRichTextCtrl* ctrl, wxWindow* parent, wxWindowID id = wxID_ANY, const wxString& caption = SYMBOL_WXRICHTEXTSTYLEORGANISERDIALOG_TITLE, const wxPoint& pos = SYMBOL_WXRICHTEXTSTYLEORGANISERDIALOG_POSITION, const wxSize& size = SYMBOL_WXRICHTEXTSTYLEORGANISERDIALOG_SIZE, long style = SYMBOL_WXRICHTEXTSTYLEORGANISERDIALOG_STYLE );

    /// Creates the controls and sizers
    void CreateControls();

    /// Initialise member variables
    void Init();

    /// Transfer data from/to window
    virtual bool TransferDataFromWindow();
    virtual bool TransferDataToWindow();

    /// Set/get style sheet
    void SetStyleSheet(wxRichTextStyleSheet* sheet) { m_richTextStyleSheet = sheet; }
    wxRichTextStyleSheet* GetStyleSheet() const { return m_richTextStyleSheet; }

    /// Set/get control
    void SetRichTextCtrl(wxRichTextCtrl* ctrl) { m_richTextCtrl = ctrl; }
    wxRichTextCtrl* GetRichTextCtrl() const { return m_richTextCtrl; }

    /// Set/get flags
    void SetFlags(int flags) { m_flags = flags; }
    int GetFlags() const { return m_flags; }

    /// Show preview for given or selected preview
    void ShowPreview(int sel = -1);

    /// Clears the preview
    void ClearPreview();

    /// List selection
    void OnListSelection(wxCommandEvent& event);

    /// Get/set restart numbering boolean
    bool GetRestartNumbering() const { return m_restartNumbering; }
    void SetRestartNumbering(bool restartNumbering) { m_restartNumbering = restartNumbering; }

    /// Get selected style name or definition
    wxString GetSelectedStyle() const;
    wxRichTextStyleDefinition* GetSelectedStyleDefinition() const;

    /// Apply the style
    bool ApplyStyle(wxRichTextCtrl* ctrl = NULL);

    /// Should we show tooltips?
    static bool ShowToolTips() { return sm_showToolTips; }

    /// Determines whether tooltips will be shown
    static void SetShowToolTips(bool show) { sm_showToolTips = show; }

////@begin wxRichTextStyleOrganiserDialog event handler declarations

    /// wxEVT_COMMAND_BUTTON_CLICKED event handler for ID_RICHTEXTSTYLEORGANISERDIALOG_NEW_CHAR
    void OnNewCharClick( wxCommandEvent& event );

    /// wxEVT_UPDATE_UI event handler for ID_RICHTEXTSTYLEORGANISERDIALOG_NEW_CHAR
    void OnNewCharUpdate( wxUpdateUIEvent& event );

    /// wxEVT_COMMAND_BUTTON_CLICKED event handler for ID_RICHTEXTSTYLEORGANISERDIALOG_NEW_PARA
    void OnNewParaClick( wxCommandEvent& event );

    /// wxEVT_UPDATE_UI event handler for ID_RICHTEXTSTYLEORGANISERDIALOG_NEW_PARA
    void OnNewParaUpdate( wxUpdateUIEvent& event );

    /// wxEVT_COMMAND_BUTTON_CLICKED event handler for ID_RICHTEXTSTYLEORGANISERDIALOG_NEW_LIST
    void OnNewListClick( wxCommandEvent& event );

    /// wxEVT_UPDATE_UI event handler for ID_RICHTEXTSTYLEORGANISERDIALOG_NEW_LIST
    void OnNewListUpdate( wxUpdateUIEvent& event );

    /// wxEVT_COMMAND_BUTTON_CLICKED event handler for ID_RICHTEXTSTYLEORGANISERDIALOG_APPLY
    void OnApplyClick( wxCommandEvent& event );

    /// wxEVT_UPDATE_UI event handler for ID_RICHTEXTSTYLEORGANISERDIALOG_APPLY
    void OnApplyUpdate( wxUpdateUIEvent& event );

    /// wxEVT_COMMAND_BUTTON_CLICKED event handler for ID_RICHTEXTSTYLEORGANISERDIALOG_RENAME
    void OnRenameClick( wxCommandEvent& event );

    /// wxEVT_UPDATE_UI event handler for ID_RICHTEXTSTYLEORGANISERDIALOG_RENAME
    void OnRenameUpdate( wxUpdateUIEvent& event );

    /// wxEVT_COMMAND_BUTTON_CLICKED event handler for ID_RICHTEXTSTYLEORGANISERDIALOG_EDIT
    void OnEditClick( wxCommandEvent& event );

    /// wxEVT_UPDATE_UI event handler for ID_RICHTEXTSTYLEORGANISERDIALOG_EDIT
    void OnEditUpdate( wxUpdateUIEvent& event );

    /// wxEVT_COMMAND_BUTTON_CLICKED event handler for ID_RICHTEXTSTYLEORGANISERDIALOG_DELETE
    void OnDeleteClick( wxCommandEvent& event );

    /// wxEVT_UPDATE_UI event handler for ID_RICHTEXTSTYLEORGANISERDIALOG_DELETE
    void OnDeleteUpdate( wxUpdateUIEvent& event );

////@end wxRichTextStyleOrganiserDialog event handler declarations

////@begin wxRichTextStyleOrganiserDialog member function declarations

    /// Retrieves bitmap resources
    wxBitmap GetBitmapResource( const wxString& name );

    /// Retrieves icon resources
    wxIcon GetIconResource( const wxString& name );
////@end wxRichTextStyleOrganiserDialog member function declarations

////@begin wxRichTextStyleOrganiserDialog member variables
    wxBoxSizer* m_innerSizer;
    wxBoxSizer* m_buttonSizerParent;
    wxRichTextStyleListCtrl* m_stylesListBox;
    wxRichTextCtrl* m_previewCtrl;
    wxBoxSizer* m_buttonSizer;
    wxButton* m_newCharacter;
    wxButton* m_newParagraph;
    wxButton* m_newList;
    wxButton* m_applyStyle;
    wxButton* m_renameStyle;
    wxButton* m_editStyle;
    wxButton* m_deleteStyle;
    wxButton* m_closeButton;
    wxBoxSizer* m_bottomButtonSizer;
    wxCheckBox* m_restartNumberingCtrl;
    wxButton* m_okButton;
    wxButton* m_cancelButton;
    /// Control identifiers
    enum {
        ID_RICHTEXTSTYLEORGANISERDIALOG = 10500,
        ID_RICHTEXTSTYLEORGANISERDIALOG_STYLES = 10501,
        ID_RICHTEXTSTYLEORGANISERDIALOG_CURRENT_STYLE = 10510,
        ID_RICHTEXTSTYLEORGANISERDIALOG_PREVIEW = 10509,
        ID_RICHTEXTSTYLEORGANISERDIALOG_NEW_CHAR = 10504,
        ID_RICHTEXTSTYLEORGANISERDIALOG_NEW_PARA = 10505,
        ID_RICHTEXTSTYLEORGANISERDIALOG_NEW_LIST = 10508,
        ID_RICHTEXTSTYLEORGANISERDIALOG_APPLY = 10503,
        ID_RICHTEXTSTYLEORGANISERDIALOG_RENAME = 10502,
        ID_RICHTEXTSTYLEORGANISERDIALOG_EDIT = 10506,
        ID_RICHTEXTSTYLEORGANISERDIALOG_DELETE = 10507,
        ID_RICHTEXTSTYLEORGANISERDIALOG_RESTART_NUMBERING = 10511
    };
////@end wxRichTextStyleOrganiserDialog member variables

private:

    wxRichTextCtrl*         m_richTextCtrl;
    wxRichTextStyleSheet*   m_richTextStyleSheet;

    bool                    m_dontUpdate;
    int                     m_flags;
    static bool             sm_showToolTips;
    bool                    m_restartNumbering;
};

#endif
    // _RICHTEXTSTYLEDLG_H_
