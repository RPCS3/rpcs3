/////////////////////////////////////////////////////////////////////////////
// Name:        src/richtext/richtextstyledlg.cpp
// Purpose:
// Author:      Julian Smart
// Modified by:
// Created:     10/5/2006 12:05:31 PM
// RCS-ID:      $Id: richtextstyledlg.cpp 61185 2009-06-24 10:35:46Z JS $
// Copyright:   (c) Julian Smart
// Licence:     wxWindows licence
/////////////////////////////////////////////////////////////////////////////

// For compilers that support precompilation, includes "wx/wx.h".
#include "wx/wxprec.h"

#ifdef __BORLANDC__
#pragma hdrstop
#endif

#if wxUSE_RICHTEXT

#ifndef WX_PRECOMP
#include "wx/wx.h"
#endif

#include "wx/richtext/richtextstyledlg.h"
#include "wx/richtext/richtextformatdlg.h"

/*!
 * wxRichTextStyleOrganiserDialog type definition
 */

bool wxRichTextStyleOrganiserDialog::sm_showToolTips = false;

IMPLEMENT_DYNAMIC_CLASS( wxRichTextStyleOrganiserDialog, wxDialog )

/*!
 * wxRichTextStyleOrganiserDialog event table definition
 */

BEGIN_EVENT_TABLE( wxRichTextStyleOrganiserDialog, wxDialog )

    EVT_LISTBOX(wxID_ANY, wxRichTextStyleOrganiserDialog::OnListSelection)

////@begin wxRichTextStyleOrganiserDialog event table entries
    EVT_BUTTON( ID_RICHTEXTSTYLEORGANISERDIALOG_NEW_CHAR, wxRichTextStyleOrganiserDialog::OnNewCharClick )
    EVT_UPDATE_UI( ID_RICHTEXTSTYLEORGANISERDIALOG_NEW_CHAR, wxRichTextStyleOrganiserDialog::OnNewCharUpdate )

    EVT_BUTTON( ID_RICHTEXTSTYLEORGANISERDIALOG_NEW_PARA, wxRichTextStyleOrganiserDialog::OnNewParaClick )
    EVT_UPDATE_UI( ID_RICHTEXTSTYLEORGANISERDIALOG_NEW_PARA, wxRichTextStyleOrganiserDialog::OnNewParaUpdate )

    EVT_BUTTON( ID_RICHTEXTSTYLEORGANISERDIALOG_NEW_LIST, wxRichTextStyleOrganiserDialog::OnNewListClick )
    EVT_UPDATE_UI( ID_RICHTEXTSTYLEORGANISERDIALOG_NEW_LIST, wxRichTextStyleOrganiserDialog::OnNewListUpdate )

    EVT_BUTTON( ID_RICHTEXTSTYLEORGANISERDIALOG_APPLY, wxRichTextStyleOrganiserDialog::OnApplyClick )
    EVT_UPDATE_UI( ID_RICHTEXTSTYLEORGANISERDIALOG_APPLY, wxRichTextStyleOrganiserDialog::OnApplyUpdate )

    EVT_BUTTON( ID_RICHTEXTSTYLEORGANISERDIALOG_RENAME, wxRichTextStyleOrganiserDialog::OnRenameClick )
    EVT_UPDATE_UI( ID_RICHTEXTSTYLEORGANISERDIALOG_RENAME, wxRichTextStyleOrganiserDialog::OnRenameUpdate )

    EVT_BUTTON( ID_RICHTEXTSTYLEORGANISERDIALOG_EDIT, wxRichTextStyleOrganiserDialog::OnEditClick )
    EVT_UPDATE_UI( ID_RICHTEXTSTYLEORGANISERDIALOG_EDIT, wxRichTextStyleOrganiserDialog::OnEditUpdate )

    EVT_BUTTON( ID_RICHTEXTSTYLEORGANISERDIALOG_DELETE, wxRichTextStyleOrganiserDialog::OnDeleteClick )
    EVT_UPDATE_UI( ID_RICHTEXTSTYLEORGANISERDIALOG_DELETE, wxRichTextStyleOrganiserDialog::OnDeleteUpdate )

////@end wxRichTextStyleOrganiserDialog event table entries

END_EVENT_TABLE()

/*!
 * wxRichTextStyleOrganiserDialog constructors
 */

wxRichTextStyleOrganiserDialog::wxRichTextStyleOrganiserDialog( )
{
    Init();
}

wxRichTextStyleOrganiserDialog::wxRichTextStyleOrganiserDialog( int flags, wxRichTextStyleSheet* sheet, wxRichTextCtrl* ctrl, wxWindow* parent, wxWindowID id, const wxString& caption, const wxPoint& pos, const wxSize& size, long style )
{
    Init();
    Create(flags, sheet, ctrl, parent, id, caption, pos, size, style);
}

/*!
 * Member initialisation for wxSymbolPickerDialog
 */

void wxRichTextStyleOrganiserDialog::Init()
{
    m_richTextStyleSheet = NULL;
    m_richTextCtrl = NULL;
    m_dontUpdate = false;
    m_flags = 0;
    m_restartNumbering = true;

////@begin wxRichTextStyleOrganiserDialog member initialisation
    m_innerSizer = NULL;
    m_buttonSizerParent = NULL;
    m_stylesListBox = NULL;
    m_previewCtrl = NULL;
    m_buttonSizer = NULL;
    m_newCharacter = NULL;
    m_newParagraph = NULL;
    m_newList = NULL;
    m_applyStyle = NULL;
    m_renameStyle = NULL;
    m_editStyle = NULL;
    m_deleteStyle = NULL;
    m_closeButton = NULL;
    m_bottomButtonSizer = NULL;
    m_restartNumberingCtrl = NULL;
    m_okButton = NULL;
    m_cancelButton = NULL;
////@end wxRichTextStyleOrganiserDialog member initialisation
}

/*!
 * wxRichTextStyleOrganiserDialog creator
 */

bool wxRichTextStyleOrganiserDialog::Create( int flags, wxRichTextStyleSheet* sheet, wxRichTextCtrl* ctrl, wxWindow* parent, wxWindowID id, const wxString& caption, const wxPoint& pos, const wxSize& size, long style )
{
    m_richTextStyleSheet = sheet;
    m_richTextCtrl = ctrl;
    m_flags = flags;

////@begin wxRichTextStyleOrganiserDialog creation
    SetExtraStyle(wxWS_EX_BLOCK_EVENTS|wxDIALOG_EX_CONTEXTHELP);
    wxDialog::Create( parent, id, caption, pos, size, style );

    CreateControls();
    if (GetSizer())
    {
        GetSizer()->SetSizeHints(this);
    }
    Centre();
////@end wxRichTextStyleOrganiserDialog creation
    return true;
}

/*!
 * Control creation for wxRichTextStyleOrganiserDialog
 */

void wxRichTextStyleOrganiserDialog::CreateControls()
{
#ifdef __WXMAC__
    SetWindowVariant(wxWINDOW_VARIANT_SMALL);
#endif

    bool hideTypeSelector = false;
    wxRichTextStyleListBox::wxRichTextStyleType typesToShow = wxRichTextStyleListBox::wxRICHTEXT_STYLE_ALL;

    if ((m_flags & wxRICHTEXT_ORGANISER_SHOW_CHARACTER) != 0)
    {
        typesToShow = wxRichTextStyleListBox::wxRICHTEXT_STYLE_CHARACTER;
        hideTypeSelector = true;
    }
    else if ((m_flags & wxRICHTEXT_ORGANISER_SHOW_PARAGRAPH) != 0)
    {
        typesToShow = wxRichTextStyleListBox::wxRICHTEXT_STYLE_PARAGRAPH;
        hideTypeSelector = true;
    }
    else if ((m_flags & wxRICHTEXT_ORGANISER_SHOW_LIST) != 0)
    {
        typesToShow = wxRichTextStyleListBox::wxRICHTEXT_STYLE_LIST;
        hideTypeSelector = true;
    }
    else
    {
        // wxRICHTEXT_ORGANISER_SHOW_ALL is implied if the other styles aren't included
    }

    long listCtrlStyle = 0;
    if (hideTypeSelector)
        listCtrlStyle = wxRICHTEXTSTYLELIST_HIDE_TYPE_SELECTOR;

////@begin wxRichTextStyleOrganiserDialog content construction
    wxRichTextStyleOrganiserDialog* itemDialog1 = this;

    wxBoxSizer* itemBoxSizer2 = new wxBoxSizer(wxVERTICAL);
    itemDialog1->SetSizer(itemBoxSizer2);

    m_innerSizer = new wxBoxSizer(wxVERTICAL);
    itemBoxSizer2->Add(m_innerSizer, 1, wxGROW|wxALL, 5);

    m_buttonSizerParent = new wxBoxSizer(wxHORIZONTAL);
    m_innerSizer->Add(m_buttonSizerParent, 1, wxGROW, 5);

    wxBoxSizer* itemBoxSizer5 = new wxBoxSizer(wxVERTICAL);
    m_buttonSizerParent->Add(itemBoxSizer5, 1, wxGROW, 5);

    wxStaticText* itemStaticText6 = new wxStaticText( itemDialog1, wxID_STATIC, _("&Styles:"), wxDefaultPosition, wxDefaultSize, 0 );
    itemBoxSizer5->Add(itemStaticText6, 0, wxALIGN_LEFT|wxLEFT|wxRIGHT|wxTOP|wxADJUST_MINSIZE, 5);

    m_stylesListBox = new wxRichTextStyleListCtrl( itemDialog1, ID_RICHTEXTSTYLEORGANISERDIALOG_STYLES, wxDefaultPosition, wxSize(280, 260), listCtrlStyle );
    m_stylesListBox->SetHelpText(_("The available styles."));
    if (wxRichTextStyleOrganiserDialog::ShowToolTips())
        m_stylesListBox->SetToolTip(_("The available styles."));
    itemBoxSizer5->Add(m_stylesListBox, 1, wxGROW|wxALL, 5);

    wxBoxSizer* itemBoxSizer8 = new wxBoxSizer(wxVERTICAL);
    m_buttonSizerParent->Add(itemBoxSizer8, 0, wxGROW, 5);

    wxStaticText* itemStaticText9 = new wxStaticText( itemDialog1, ID_RICHTEXTSTYLEORGANISERDIALOG_CURRENT_STYLE, _(" "), wxDefaultPosition, wxDefaultSize, 0 );
    itemBoxSizer8->Add(itemStaticText9, 0, wxGROW|wxLEFT|wxRIGHT|wxTOP|wxADJUST_MINSIZE, 5);

    m_previewCtrl = new wxRichTextCtrl( itemDialog1, ID_RICHTEXTSTYLEORGANISERDIALOG_PREVIEW, wxEmptyString, wxDefaultPosition, wxSize(250, 200), wxVSCROLL|wxTE_READONLY );
    m_previewCtrl->SetHelpText(_("The style preview."));
    if (wxRichTextStyleOrganiserDialog::ShowToolTips())
        m_previewCtrl->SetToolTip(_("The style preview."));
    itemBoxSizer8->Add(m_previewCtrl, 1, wxGROW|wxALL, 5);

    m_buttonSizer = new wxBoxSizer(wxVERTICAL);
    m_buttonSizerParent->Add(m_buttonSizer, 0, wxGROW, 5);

    wxStaticText* itemStaticText12 = new wxStaticText( itemDialog1, wxID_STATIC, _(" "), wxDefaultPosition, wxDefaultSize, 0 );
    m_buttonSizer->Add(itemStaticText12, 0, wxALIGN_LEFT|wxLEFT|wxRIGHT|wxTOP|wxADJUST_MINSIZE, 5);

    m_newCharacter = new wxButton( itemDialog1, ID_RICHTEXTSTYLEORGANISERDIALOG_NEW_CHAR, _("New &Character Style..."), wxDefaultPosition, wxDefaultSize, 0 );
    m_newCharacter->SetHelpText(_("Click to create a new character style."));
    if (wxRichTextStyleOrganiserDialog::ShowToolTips())
        m_newCharacter->SetToolTip(_("Click to create a new character style."));
    m_buttonSizer->Add(m_newCharacter, 0, wxGROW|wxALL, 5);

    m_newParagraph = new wxButton( itemDialog1, ID_RICHTEXTSTYLEORGANISERDIALOG_NEW_PARA, _("New &Paragraph Style..."), wxDefaultPosition, wxDefaultSize, 0 );
    m_newParagraph->SetHelpText(_("Click to create a new paragraph style."));
    if (wxRichTextStyleOrganiserDialog::ShowToolTips())
        m_newParagraph->SetToolTip(_("Click to create a new paragraph style."));
    m_buttonSizer->Add(m_newParagraph, 0, wxGROW|wxLEFT|wxRIGHT|wxBOTTOM, 5);

    m_newList = new wxButton( itemDialog1, ID_RICHTEXTSTYLEORGANISERDIALOG_NEW_LIST, _("New &List Style..."), wxDefaultPosition, wxDefaultSize, 0 );
    m_newList->SetHelpText(_("Click to create a new list style."));
    if (wxRichTextStyleOrganiserDialog::ShowToolTips())
        m_newList->SetToolTip(_("Click to create a new list style."));
    m_buttonSizer->Add(m_newList, 0, wxGROW|wxLEFT|wxRIGHT|wxBOTTOM, 5);

    m_buttonSizer->Add(5, 5, 1, wxALIGN_CENTER_HORIZONTAL|wxALL, 5);

    m_applyStyle = new wxButton( itemDialog1, ID_RICHTEXTSTYLEORGANISERDIALOG_APPLY, _("&Apply Style"), wxDefaultPosition, wxDefaultSize, 0 );
    m_applyStyle->SetHelpText(_("Click to apply the selected style."));
    if (wxRichTextStyleOrganiserDialog::ShowToolTips())
        m_applyStyle->SetToolTip(_("Click to apply the selected style."));
    m_buttonSizer->Add(m_applyStyle, 0, wxGROW|wxALL, 5);

    m_renameStyle = new wxButton( itemDialog1, ID_RICHTEXTSTYLEORGANISERDIALOG_RENAME, _("&Rename Style..."), wxDefaultPosition, wxDefaultSize, 0 );
    m_renameStyle->SetHelpText(_("Click to rename the selected style."));
    if (wxRichTextStyleOrganiserDialog::ShowToolTips())
        m_renameStyle->SetToolTip(_("Click to rename the selected style."));
    m_buttonSizer->Add(m_renameStyle, 0, wxGROW|wxLEFT|wxRIGHT|wxBOTTOM, 5);

    m_editStyle = new wxButton( itemDialog1, ID_RICHTEXTSTYLEORGANISERDIALOG_EDIT, _("&Edit Style..."), wxDefaultPosition, wxDefaultSize, 0 );
    m_editStyle->SetHelpText(_("Click to edit the selected style."));
    if (wxRichTextStyleOrganiserDialog::ShowToolTips())
        m_editStyle->SetToolTip(_("Click to edit the selected style."));
    m_buttonSizer->Add(m_editStyle, 0, wxGROW|wxLEFT|wxRIGHT|wxBOTTOM, 5);

    m_deleteStyle = new wxButton( itemDialog1, ID_RICHTEXTSTYLEORGANISERDIALOG_DELETE, _("&Delete Style..."), wxDefaultPosition, wxDefaultSize, 0 );
    m_deleteStyle->SetHelpText(_("Click to delete the selected style."));
    if (wxRichTextStyleOrganiserDialog::ShowToolTips())
        m_deleteStyle->SetToolTip(_("Click to delete the selected style."));
    m_buttonSizer->Add(m_deleteStyle, 0, wxGROW|wxLEFT|wxRIGHT|wxBOTTOM, 5);

    m_buttonSizer->Add(5, 5, 1, wxALIGN_CENTER_HORIZONTAL|wxALL, 5);

    m_closeButton = new wxButton( itemDialog1, wxID_CANCEL, _("Close"), wxDefaultPosition, wxDefaultSize, 0 );
    m_closeButton->SetDefault();
    m_closeButton->SetHelpText(_("Click to close this window."));
    if (wxRichTextStyleOrganiserDialog::ShowToolTips())
        m_closeButton->SetToolTip(_("Click to close this window."));
    m_buttonSizer->Add(m_closeButton, 0, wxGROW|wxALL, 5);

    m_bottomButtonSizer = new wxBoxSizer(wxHORIZONTAL);
    m_innerSizer->Add(m_bottomButtonSizer, 0, wxGROW, 5);

    m_restartNumberingCtrl = new wxCheckBox( itemDialog1, ID_RICHTEXTSTYLEORGANISERDIALOG_RESTART_NUMBERING, _("&Restart numbering"), wxDefaultPosition, wxDefaultSize, 0 );
    m_restartNumberingCtrl->SetValue(false);
    m_restartNumberingCtrl->SetHelpText(_("Check to restart numbering."));
    if (wxRichTextStyleOrganiserDialog::ShowToolTips())
        m_restartNumberingCtrl->SetToolTip(_("Check to restart numbering."));
    m_bottomButtonSizer->Add(m_restartNumberingCtrl, 0, wxALIGN_CENTER_VERTICAL|wxALL, 5);

    m_bottomButtonSizer->Add(5, 5, 1, wxALIGN_CENTER_VERTICAL|wxALL, 5);

    m_okButton = new wxButton( itemDialog1, wxID_OK, _("OK"), wxDefaultPosition, wxDefaultSize, 0 );
    m_okButton->SetDefault();
    m_okButton->SetHelpText(_("Click to confirm your selection."));
    if (wxRichTextStyleOrganiserDialog::ShowToolTips())
        m_okButton->SetToolTip(_("Click to confirm your selection."));
    m_bottomButtonSizer->Add(m_okButton, 0, wxALIGN_CENTER_VERTICAL|wxALL, 5);

    m_cancelButton = new wxButton( itemDialog1, wxID_CANCEL, _("Cancel"), wxDefaultPosition, wxDefaultSize, 0 );
    m_cancelButton->SetHelpText(_("Click to cancel this window."));
    if (wxRichTextStyleOrganiserDialog::ShowToolTips())
        m_cancelButton->SetToolTip(_("Click to cancel this window."));
    m_bottomButtonSizer->Add(m_cancelButton, 0, wxALIGN_CENTER_VERTICAL|wxALL, 5);

////@end wxRichTextStyleOrganiserDialog content construction

    if ((m_flags & wxRICHTEXT_ORGANISER_CREATE_STYLES) == 0)
    {
        m_buttonSizer->Show(m_newCharacter, false);
        m_buttonSizer->Show(m_newParagraph, false);
        m_buttonSizer->Show(m_newList, false);
    }
    if ((m_flags & wxRICHTEXT_ORGANISER_DELETE_STYLES) == 0)
    {
        m_buttonSizer->Show(m_deleteStyle, false);
    }
    if ((m_flags & wxRICHTEXT_ORGANISER_APPLY_STYLES) == 0)
    {
        m_buttonSizer->Show(m_applyStyle, false);
    }
    if ((m_flags & wxRICHTEXT_ORGANISER_EDIT_STYLES) == 0)
    {
        m_buttonSizer->Show(m_editStyle, false);
    }
    if ((m_flags & wxRICHTEXT_ORGANISER_RENAME_STYLES) == 0)
    {
        m_buttonSizer->Show(m_renameStyle, false);
    }
    if ((m_flags & wxRICHTEXT_ORGANISER_RENUMBER) == 0)
    {
        m_bottomButtonSizer->Show(m_restartNumberingCtrl, false);
    }

    if ((m_flags & wxRICHTEXT_ORGANISER_OK_CANCEL) == 0)
    {
        m_bottomButtonSizer->Show(m_okButton, false);
        m_bottomButtonSizer->Show(m_cancelButton, false);
    }
    else
    {
        m_buttonSizer->Show(m_closeButton, false);
    }

    // No buttons in the vertical group are shown, so hide the whole sizer
    if ((m_flags & wxRICHTEXT_ORGANISER_ORGANISE) == 0)
    {
        m_buttonSizerParent->Show(m_buttonSizer, false);
    }

    // No buttons in the horizontal group are shown, so hide the whole sizer
    if ((m_flags & (wxRICHTEXT_ORGANISER_OK_CANCEL|wxRICHTEXT_ORGANISER_RENUMBER)) == 0)
    {
        m_innerSizer->Show(m_bottomButtonSizer, false);
    }

    if (hideTypeSelector)
    {
        m_stylesListBox->GetStyleListBox()->SetStyleType(typesToShow);
    }

    m_stylesListBox->SetStyleSheet(m_richTextStyleSheet);
    m_stylesListBox->SetRichTextCtrl(m_richTextCtrl);
    m_stylesListBox->UpdateStyles();
    m_stylesListBox->GetStyleListBox()->SetAutoSetSelection(false); // stop idle-time auto selection
    if (m_stylesListBox->GetStyleListBox()->GetItemCount() > 0)
    {
        m_stylesListBox->GetStyleListBox()->SetSelection(0);
        ShowPreview();
    }
}

/*!
 * Should we show tooltips?
 */

/// Get selected style name or definition
wxString wxRichTextStyleOrganiserDialog::GetSelectedStyle() const
{
    wxRichTextStyleDefinition* def = GetSelectedStyleDefinition();
    if (def)
        return def->GetName();
    else
        return wxEmptyString;
}

wxRichTextStyleDefinition* wxRichTextStyleOrganiserDialog::GetSelectedStyleDefinition() const
{
    int sel = m_stylesListBox->GetStyleListBox()->GetSelection();
    return m_stylesListBox->GetStyleListBox()->GetStyle(sel);
}


/// Transfer data from/to window
bool wxRichTextStyleOrganiserDialog::TransferDataFromWindow()
{
    if (!wxDialog::TransferDataFromWindow())
        return false;

    m_restartNumbering = m_restartNumberingCtrl->GetValue();

    return true;
}

bool wxRichTextStyleOrganiserDialog::TransferDataToWindow()
{
    if (!wxDialog::TransferDataToWindow())
        return false;

    m_restartNumberingCtrl->SetValue(m_restartNumbering);

    return true;
}

/// Show preview for given or selected preview
void wxRichTextStyleOrganiserDialog::ShowPreview(int sel)
{
    static const wxChar* s_para1 = wxT("Lorem ipsum dolor sit amet, consectetuer adipiscing elit. \
Nullam ante sapien, vestibulum nonummy, pulvinar sed, luctus ut, lacus.\n");

    static const wxChar* s_para2List = wxT("Duis pharetra consequat dui. Nullam vitae justo id mauris lobortis interdum.\n");

    static const wxChar* s_para2 = wxT("Duis pharetra consequat dui. Cum sociis natoque penatibus \
et magnis dis parturient montes, nascetur ridiculus mus. Nullam vitae justo id mauris lobortis interdum.\n");

    static const wxChar* s_para3 = wxT("Integer convallis dolor at augue \
iaculis malesuada. Donec bibendum ipsum ut ante porta fringilla.\n");

    if (sel == -1)
    {
        sel = m_stylesListBox->GetStyleListBox()->GetSelection();
        if (sel == -1)
            return;
    }

    wxRichTextStyleDefinition* def = m_stylesListBox->GetStyleListBox()->GetStyle(sel);

    wxRichTextListStyleDefinition* listDef = wxDynamicCast(def, wxRichTextListStyleDefinition);

    wxStaticText* labelCtrl = (wxStaticText*) FindWindow(ID_RICHTEXTSTYLEORGANISERDIALOG_CURRENT_STYLE);
    if (labelCtrl)
        labelCtrl->SetLabel(def->GetName() + wxT(":"));

    wxTextAttrEx attr(def->GetStyleMergedWithBase(GetStyleSheet()));

    wxFont font(m_previewCtrl->GetFont());
    font.SetPointSize(9);
    m_previewCtrl->SetFont(font);

    wxTextAttrEx normalParaAttr;
    normalParaAttr.SetFont(font);
    normalParaAttr.SetTextColour(wxColour(wxT("LIGHT GREY")));

    m_previewCtrl->Freeze();
    m_previewCtrl->Clear();

    m_previewCtrl->BeginStyle(normalParaAttr);
    m_previewCtrl->WriteText(s_para1);
    m_previewCtrl->EndStyle();

    m_previewCtrl->BeginStyle(attr);

    if (listDef)
    {
        long listStart = m_previewCtrl->GetInsertionPoint() + 1;
        int i;
        for (i = 0; i < 10; i++)
        {
            wxTextAttrEx levelAttr = * listDef->GetLevelAttributes(i);
            levelAttr.SetBulletNumber(1);
            m_previewCtrl->BeginStyle(levelAttr);
            m_previewCtrl->WriteText(wxString::Format(wxT("List level %d. "), i+1) + s_para2List);
            m_previewCtrl->EndStyle();
        }
        long listEnd = m_previewCtrl->GetInsertionPoint();
        m_previewCtrl->NumberList(wxRichTextRange(listStart, listEnd), listDef);
    }
    else
    {
        m_previewCtrl->WriteText(s_para2);
    }

    m_previewCtrl->EndStyle();

    m_previewCtrl->BeginStyle(normalParaAttr);
    m_previewCtrl->WriteText(s_para3);
    m_previewCtrl->EndStyle();

    m_previewCtrl->Thaw();
}

/// Clears the preview
void wxRichTextStyleOrganiserDialog::ClearPreview()
{
    m_previewCtrl->Clear();
    wxStaticText* labelCtrl = (wxStaticText*) FindWindow(ID_RICHTEXTSTYLEORGANISERDIALOG_CURRENT_STYLE);
    if (labelCtrl)
        labelCtrl->SetLabel(wxEmptyString);
}

bool wxRichTextStyleOrganiserDialog::ApplyStyle(wxRichTextCtrl* ctrl)
{
    int sel = m_stylesListBox->GetStyleListBox()->GetSelection();
    if (sel == -1)
        return false;
    if (!ctrl)
        ctrl = GetRichTextCtrl();
    if (!ctrl)
        return false;
    if (!ctrl->HasSelection())
        return false;


    wxRichTextStyleDefinition* def = m_stylesListBox->GetStyleListBox()->GetStyle(sel);
    wxRichTextListStyleDefinition* listDef = wxDynamicCast(def, wxRichTextListStyleDefinition);

    if (listDef && m_restartNumberingCtrl->GetValue())
    {
        wxRichTextRange range = ctrl->GetSelectionRange();
        return ctrl->SetListStyle(range, listDef, wxRICHTEXT_SETSTYLE_WITH_UNDO|wxRICHTEXT_SETSTYLE_RENUMBER);
    }
    else
    {
        return ctrl->ApplyStyle(def);
    }
}

/*!
 * Get bitmap resources
 */

wxBitmap wxRichTextStyleOrganiserDialog::GetBitmapResource( const wxString& name )
{
    // Bitmap retrieval
////@begin wxRichTextStyleOrganiserDialog bitmap retrieval
    wxUnusedVar(name);
    return wxNullBitmap;
////@end wxRichTextStyleOrganiserDialog bitmap retrieval
}

/*!
 * Get icon resources
 */

wxIcon wxRichTextStyleOrganiserDialog::GetIconResource( const wxString& name )
{
    // Icon retrieval
////@begin wxRichTextStyleOrganiserDialog icon retrieval
    wxUnusedVar(name);
    return wxNullIcon;
////@end wxRichTextStyleOrganiserDialog icon retrieval
}

/*!
 * wxEVT_COMMAND_BUTTON_CLICKED event handler for ID_RICHTEXTSTYLEORGANISERDIALOG_APPLY
 */

void wxRichTextStyleOrganiserDialog::OnApplyClick( wxCommandEvent& WXUNUSED(event) )
{
    ApplyStyle();
}

/*!
 * wxEVT_UPDATE_UI event handler for ID_RICHTEXTSTYLEORGANISERDIALOG_APPLY
 */

void wxRichTextStyleOrganiserDialog::OnApplyUpdate( wxUpdateUIEvent& event )
{
    event.Enable(((GetFlags() & wxRICHTEXT_ORGANISER_APPLY_STYLES) != 0) && m_stylesListBox->GetStyleListBox()->GetSelection() != wxNOT_FOUND);
}


/*!
 * wxEVT_COMMAND_BUTTON_CLICKED event handler for ID_RICHTEXTSTYLEORGANISERDIALOG_NEW_CHAR
 */

void wxRichTextStyleOrganiserDialog::OnNewCharClick( wxCommandEvent& WXUNUSED(event) )
{
    wxString styleName = wxGetTextFromUser(_("Enter a character style name"), _("New Style"));
    if (!styleName.IsEmpty())
    {
        if (GetStyleSheet()->FindCharacterStyle(styleName))
        {
            wxMessageBox(_("Sorry, that name is taken. Please choose another."), _("New Style"), wxICON_EXCLAMATION|wxOK, this);
            return;
        }

        wxRichTextCharacterStyleDefinition* style = new wxRichTextCharacterStyleDefinition(styleName);

        int pages = wxRICHTEXT_FORMAT_FONT|wxRICHTEXT_FORMAT_STYLE_EDITOR;
        wxRichTextFormattingDialog formatDlg;
        formatDlg.SetStyleDefinition(*style, GetStyleSheet());
        formatDlg.Create(pages, this);

        if (formatDlg.ShowModal() == wxID_OK)
        {
            wxRichTextCharacterStyleDefinition* charDef = wxDynamicCast(formatDlg.GetStyleDefinition(), wxRichTextCharacterStyleDefinition);

            (*((wxRichTextCharacterStyleDefinition* ) style)) = (*charDef);

            GetStyleSheet()->AddCharacterStyle(style);

            m_stylesListBox->UpdateStyles();
            ShowPreview();
        }
        else
            delete style;
    }
}

/*!
 * wxEVT_UPDATE_UI event handler for ID_RICHTEXTSTYLEORGANISERDIALOG_NEW_CHAR
 */

void wxRichTextStyleOrganiserDialog::OnNewCharUpdate( wxUpdateUIEvent& event )
{
    event.Enable((GetFlags() & wxRICHTEXT_ORGANISER_CREATE_STYLES) != 0);
}


/*!
 * wxEVT_COMMAND_BUTTON_CLICKED event handler for ID_RICHTEXTSTYLEORGANISERDIALOG_NEW_PARA
 */

void wxRichTextStyleOrganiserDialog::OnNewParaClick( wxCommandEvent& WXUNUSED(event) )
{
    wxString styleName = wxGetTextFromUser(_("Enter a paragraph style name"), _("New Style"));
    if (!styleName.IsEmpty())
    {
        if (GetStyleSheet()->FindParagraphStyle(styleName))
        {
            wxMessageBox(_("Sorry, that name is taken. Please choose another."), _("New Style"), wxICON_EXCLAMATION|wxOK, this);
            return;
        }

        wxRichTextParagraphStyleDefinition* style = new wxRichTextParagraphStyleDefinition(styleName);

        int pages = wxRICHTEXT_FORMAT_STYLE_EDITOR|wxRICHTEXT_FORMAT_FONT|wxRICHTEXT_FORMAT_INDENTS_SPACING|wxRICHTEXT_FORMAT_TABS|wxRICHTEXT_FORMAT_BULLETS;
        wxRichTextFormattingDialog formatDlg;
        formatDlg.SetStyleDefinition(*style, GetStyleSheet());
        formatDlg.Create(pages, this);

        if (formatDlg.ShowModal() == wxID_OK)
        {
            wxRichTextParagraphStyleDefinition* paraDef = wxDynamicCast(formatDlg.GetStyleDefinition(), wxRichTextParagraphStyleDefinition);

            (*((wxRichTextParagraphStyleDefinition* ) style)) = (*paraDef);

            GetStyleSheet()->AddParagraphStyle(style);

            m_stylesListBox->UpdateStyles();
            ShowPreview();
        }
        else
            delete style;
    }
}

/*!
 * wxEVT_UPDATE_UI event handler for ID_RICHTEXTSTYLEORGANISERDIALOG_NEW_PARA
 */

void wxRichTextStyleOrganiserDialog::OnNewParaUpdate( wxUpdateUIEvent& event )
{
    event.Enable((GetFlags() & wxRICHTEXT_ORGANISER_CREATE_STYLES) != 0);
}


/*!
 * wxEVT_COMMAND_BUTTON_CLICKED event handler for ID_RICHTEXTSTYLEORGANISERDIALOG_EDIT
 */

void wxRichTextStyleOrganiserDialog::OnEditClick( wxCommandEvent& WXUNUSED(event) )
{
    int sel = m_stylesListBox->GetStyleListBox()->GetSelection();
    if (sel != wxNOT_FOUND)
    {
        wxRichTextStyleDefinition* def = m_stylesListBox->GetStyleListBox()->GetStyle(sel);

        int pages = wxRICHTEXT_FORMAT_STYLE_EDITOR;

        if (def->IsKindOf(CLASSINFO(wxRichTextCharacterStyleDefinition)))
        {
            pages |= wxRICHTEXT_FORMAT_FONT;
        }
        else if (def->IsKindOf(CLASSINFO(wxRichTextListStyleDefinition)))
        {
            pages |= wxRICHTEXT_FORMAT_LIST_STYLE|wxRICHTEXT_FORMAT_FONT|wxRICHTEXT_FORMAT_INDENTS_SPACING;
        }
        else if (def->IsKindOf(CLASSINFO(wxRichTextParagraphStyleDefinition)))
        {
            pages |= wxRICHTEXT_FORMAT_FONT|wxRICHTEXT_FORMAT_INDENTS_SPACING|wxRICHTEXT_FORMAT_TABS|wxRICHTEXT_FORMAT_BULLETS;
        }

        wxRichTextFormattingDialog formatDlg;
        formatDlg.SetStyleDefinition(*def, GetStyleSheet());
        formatDlg.Create(pages, this);

        if (formatDlg.ShowModal() == wxID_OK)
        {
            wxRichTextParagraphStyleDefinition* paraDef = wxDynamicCast(formatDlg.GetStyleDefinition(), wxRichTextParagraphStyleDefinition);
            wxRichTextCharacterStyleDefinition* charDef = wxDynamicCast(formatDlg.GetStyleDefinition(), wxRichTextCharacterStyleDefinition);
            wxRichTextListStyleDefinition* listDef = wxDynamicCast(formatDlg.GetStyleDefinition(), wxRichTextListStyleDefinition);

            if (listDef)
            {
                (*((wxRichTextListStyleDefinition* ) def)) = (*listDef);
            }
            else if (paraDef)
            {
                (*((wxRichTextParagraphStyleDefinition* ) def)) = (*paraDef);
            }
            else
            {
                (*((wxRichTextCharacterStyleDefinition* ) def)) = (*charDef);
            }

            m_stylesListBox->UpdateStyles();
            m_stylesListBox->GetStyleListBox()->SetSelection(sel);
            ShowPreview();
        }
    }
}

/*!
 * wxEVT_UPDATE_UI event handler for ID_RICHTEXTSTYLEORGANISERDIALOG_EDIT
 */

void wxRichTextStyleOrganiserDialog::OnEditUpdate( wxUpdateUIEvent& event )
{
    event.Enable(((GetFlags() & wxRICHTEXT_ORGANISER_EDIT_STYLES) != 0) && m_stylesListBox->GetStyleListBox()->GetSelection() != wxNOT_FOUND);
}


/*!
 * wxEVT_COMMAND_BUTTON_CLICKED event handler for ID_RICHTEXTSTYLEORGANISERDIALOG_DELETE
 */

void wxRichTextStyleOrganiserDialog::OnDeleteClick( wxCommandEvent& WXUNUSED(event) )
{
    int sel = m_stylesListBox->GetStyleListBox()->GetSelection();
    if (sel != wxNOT_FOUND)
    {
        wxRichTextStyleDefinition* def = m_stylesListBox->GetStyleListBox()->GetStyle(sel);
        wxString name(def->GetName());
        if (wxYES == wxMessageBox(wxString::Format(_("Delete style %s?"), (const wxChar*) name), _("Delete Style"), wxYES_NO|wxICON_QUESTION, this))
        {
            m_stylesListBox->GetStyleListBox()->SetItemCount(0);

            if (def->IsKindOf(CLASSINFO(wxRichTextListStyleDefinition)))
                GetStyleSheet()->RemoveListStyle((wxRichTextListStyleDefinition*) def, true);
            else if (def->IsKindOf(CLASSINFO(wxRichTextParagraphStyleDefinition)))
                GetStyleSheet()->RemoveParagraphStyle((wxRichTextParagraphStyleDefinition*) def, true);
            else if (def->IsKindOf(CLASSINFO(wxRichTextCharacterStyleDefinition)))
                GetStyleSheet()->RemoveCharacterStyle((wxRichTextCharacterStyleDefinition*) def, true);

            m_stylesListBox->UpdateStyles();

            if (m_stylesListBox->GetStyleListBox()->GetSelection() != -1)
                ShowPreview();
            else
                ClearPreview();
        }
    }
}

/*!
 * wxEVT_UPDATE_UI event handler for ID_RICHTEXTSTYLEORGANISERDIALOG_DELETE
 */

void wxRichTextStyleOrganiserDialog::OnDeleteUpdate( wxUpdateUIEvent& event )
{
    event.Enable(((GetFlags() & wxRICHTEXT_ORGANISER_DELETE_STYLES) != 0) && m_stylesListBox->GetStyleListBox()->GetSelection() != wxNOT_FOUND);
}

/*!
 * wxEVT_COMMAND_BUTTON_CLICKED event handler for ID_RICHTEXTSTYLEORGANISERDIALOG_NEW_LIST
 */

void wxRichTextStyleOrganiserDialog::OnNewListClick( wxCommandEvent& WXUNUSED(event) )
{
    wxString styleName = wxGetTextFromUser(_("Enter a list style name"), _("New Style"));
    if (!styleName.IsEmpty())
    {
        if (GetStyleSheet()->FindListStyle(styleName))
        {
            wxMessageBox(_("Sorry, that name is taken. Please choose another."), _("New Style"), wxICON_EXCLAMATION|wxOK, this);
            return;
        }

        wxRichTextListStyleDefinition* style = new wxRichTextListStyleDefinition(styleName);

        // Initialize the style to make it easier to edit
        int i;
        for (i = 0; i < 10; i++)
        {
            wxString bulletSymbol;
            if (i == 0)
                bulletSymbol = wxT("*");
            else if (i == 1)
                bulletSymbol = wxT("-");
            else if (i == 2)
                bulletSymbol = wxT("*");
            else if (i == 3)
                bulletSymbol = wxT("-");
            else
                bulletSymbol = wxT("*");

            style->SetAttributes(i, (i+1)*60, 60, wxTEXT_ATTR_BULLET_STYLE_SYMBOL, bulletSymbol);
        }

        int pages = wxRICHTEXT_FORMAT_LIST_STYLE|wxRICHTEXT_FORMAT_STYLE_EDITOR|wxRICHTEXT_FORMAT_FONT|wxRICHTEXT_FORMAT_INDENTS_SPACING;
        wxRichTextFormattingDialog formatDlg;
        formatDlg.SetStyleDefinition(*style, GetStyleSheet());
        formatDlg.Create(pages, this);

        if (formatDlg.ShowModal() == wxID_OK)
        {
            wxRichTextListStyleDefinition* listDef = wxDynamicCast(formatDlg.GetStyleDefinition(), wxRichTextListStyleDefinition);

            (*((wxRichTextListStyleDefinition* ) style)) = (*listDef);

            GetStyleSheet()->AddListStyle(style);

            m_stylesListBox->UpdateStyles();
            ShowPreview();
        }
        else
            delete style;
    }
}

/*!
 * wxEVT_UPDATE_UI event handler for ID_RICHTEXTSTYLEORGANISERDIALOG_NEW_LIST
 */

void wxRichTextStyleOrganiserDialog::OnNewListUpdate( wxUpdateUIEvent& event )
{
    event.Enable((GetFlags() & wxRICHTEXT_ORGANISER_CREATE_STYLES) != 0);
}

/*!
 * wxEVT_COMMAND_BUTTON_CLICKED event handler for ID_RICHTEXTSTYLEORGANISERDIALOG_RENAME
 */

void wxRichTextStyleOrganiserDialog::OnRenameClick( wxCommandEvent& WXUNUSED(event) )
{
    int sel = m_stylesListBox->GetStyleListBox()->GetSelection();
    if (sel == -1)
        return;
    wxRichTextStyleDefinition* def = m_stylesListBox->GetStyleListBox()->GetStyle(sel);
    if (!def)
        return;

    wxString styleName = wxGetTextFromUser(_("Enter a new style name"), _("New Style"), def->GetName());
    if (!styleName.IsEmpty())
    {
        if (styleName == def->GetName())
            return;

        if (GetStyleSheet()->FindParagraphStyle(styleName) || GetStyleSheet()->FindCharacterStyle(styleName) || GetStyleSheet()->FindListStyle(styleName))
        {
            wxMessageBox(_("Sorry, that name is taken. Please choose another."), _("New Style"), wxICON_EXCLAMATION|wxOK, this);
            return;
        }

        def->SetName(styleName);
        m_stylesListBox->UpdateStyles();
    }
}

/*!
 * wxEVT_UPDATE_UI event handler for ID_RICHTEXTSTYLEORGANISERDIALOG_RENAME
 */

void wxRichTextStyleOrganiserDialog::OnRenameUpdate( wxUpdateUIEvent& event )
{
    event.Enable(((GetFlags() & wxRICHTEXT_ORGANISER_RENAME_STYLES) != 0) && m_stylesListBox->GetStyleListBox()->GetSelection() != wxNOT_FOUND);
}

/// List selection
void wxRichTextStyleOrganiserDialog::OnListSelection(wxCommandEvent& event)
{
    if (event.GetEventObject() == m_stylesListBox->GetStyleListBox())
        ShowPreview();
    else
        event.Skip();
}

#endif
    // wxUSE_RICHTEXT

