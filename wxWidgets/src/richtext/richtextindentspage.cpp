/////////////////////////////////////////////////////////////////////////////
// Name:        src/richtext/richtextindentspage.cpp
// Purpose:
// Author:      Julian Smart
// Modified by:
// Created:     10/3/2006 2:28:21 PM
// RCS-ID:      $Id: richtextindentspage.cpp 64163 2010-04-27 16:16:21Z JS $
// Copyright:   (c) Julian Smart
// Licence:     wxWindows licence
/////////////////////////////////////////////////////////////////////////////

#if wxUSE_RICHTEXT

#include "wx/richtext/richtextindentspage.h"

/*!
 * wxRichTextIndentsSpacingPage type definition
 */

IMPLEMENT_DYNAMIC_CLASS( wxRichTextIndentsSpacingPage, wxPanel )

/*!
 * wxRichTextIndentsSpacingPage event table definition
 */

BEGIN_EVENT_TABLE( wxRichTextIndentsSpacingPage, wxPanel )

////@begin wxRichTextIndentsSpacingPage event table entries
    EVT_RADIOBUTTON( ID_RICHTEXTINDENTSSPACINGPAGE_ALIGNMENT_LEFT, wxRichTextIndentsSpacingPage::OnAlignmentLeftSelected )

    EVT_RADIOBUTTON( ID_RICHTEXTINDENTSSPACINGPAGE_ALIGNMENT_RIGHT, wxRichTextIndentsSpacingPage::OnAlignmentRightSelected )

    EVT_RADIOBUTTON( ID_RICHTEXTINDENTSSPACINGPAGE_ALIGNMENT_JUSTIFIED, wxRichTextIndentsSpacingPage::OnAlignmentJustifiedSelected )

    EVT_RADIOBUTTON( ID_RICHTEXTINDENTSSPACINGPAGE_ALIGNMENT_CENTRED, wxRichTextIndentsSpacingPage::OnAlignmentCentredSelected )

    EVT_RADIOBUTTON( ID_RICHTEXTINDENTSSPACINGPAGE_ALIGNMENT_INDETERMINATE, wxRichTextIndentsSpacingPage::OnAlignmentIndeterminateSelected )

    EVT_TEXT( ID_RICHTEXTINDENTSSPACINGPAGE_INDENT_LEFT, wxRichTextIndentsSpacingPage::OnIndentLeftUpdated )

    EVT_TEXT( ID_RICHTEXTINDENTSSPACINGPAGE_INDENT_LEFT_FIRST, wxRichTextIndentsSpacingPage::OnIndentLeftFirstUpdated )

    EVT_TEXT( ID_RICHTEXTINDENTSSPACINGPAGE_INDENT_RIGHT, wxRichTextIndentsSpacingPage::OnIndentRightUpdated )

    EVT_COMBOBOX( ID_RICHTEXTINDENTSSPACINGPAGE_OUTLINELEVEL, wxRichTextIndentsSpacingPage::OnRichtextOutlinelevelSelected )

    EVT_TEXT( ID_RICHTEXTINDENTSSPACINGPAGE_SPACING_BEFORE, wxRichTextIndentsSpacingPage::OnSpacingBeforeUpdated )

    EVT_TEXT( ID_RICHTEXTINDENTSSPACINGPAGE_SPACING_AFTER, wxRichTextIndentsSpacingPage::OnSpacingAfterUpdated )

    EVT_COMBOBOX( ID_RICHTEXTINDENTSSPACINGPAGE_SPACING_LINE, wxRichTextIndentsSpacingPage::OnSpacingLineSelected )

////@end wxRichTextIndentsSpacingPage event table entries

END_EVENT_TABLE()

/*!
 * wxRichTextIndentsSpacingPage constructors
 */

wxRichTextIndentsSpacingPage::wxRichTextIndentsSpacingPage( )
{
    Init();
}

wxRichTextIndentsSpacingPage::wxRichTextIndentsSpacingPage( wxWindow* parent, wxWindowID id, const wxPoint& pos, const wxSize& size, long style )
{
    Init();
    Create(parent, id, pos, size, style);
}

/*!
 * Initialise members
 */

void wxRichTextIndentsSpacingPage::Init()
{
    m_dontUpdate = false;

////@begin wxRichTextIndentsSpacingPage member initialisation
    m_alignmentLeft = NULL;
    m_alignmentRight = NULL;
    m_alignmentJustified = NULL;
    m_alignmentCentred = NULL;
    m_alignmentIndeterminate = NULL;
    m_indentLeft = NULL;
    m_indentLeftFirst = NULL;
    m_indentRight = NULL;
    m_outlineLevelCtrl = NULL;
    m_spacingBefore = NULL;
    m_spacingAfter = NULL;
    m_spacingLine = NULL;
    m_previewCtrl = NULL;
////@end wxRichTextIndentsSpacingPage member initialisation

}

/*!
 * wxRichTextIndentsSpacingPage creator
 */

bool wxRichTextIndentsSpacingPage::Create( wxWindow* parent, wxWindowID id, const wxPoint& pos, const wxSize& size, long style )
{
////@begin wxRichTextIndentsSpacingPage creation
    wxPanel::Create( parent, id, pos, size, style );

    CreateControls();
    if (GetSizer())
    {
        GetSizer()->SetSizeHints(this);
    }
    Centre();
////@end wxRichTextIndentsSpacingPage creation
    return true;
}

/*!
 * Control creation for wxRichTextIndentsSpacingPage
 */

void wxRichTextIndentsSpacingPage::CreateControls()
{
////@begin wxRichTextIndentsSpacingPage content construction
    wxRichTextIndentsSpacingPage* itemPanel1 = this;

    wxBoxSizer* itemBoxSizer2 = new wxBoxSizer(wxVERTICAL);
    itemPanel1->SetSizer(itemBoxSizer2);

    wxBoxSizer* itemBoxSizer3 = new wxBoxSizer(wxVERTICAL);
    itemBoxSizer2->Add(itemBoxSizer3, 1, wxGROW|wxALL, 5);

    wxBoxSizer* itemBoxSizer4 = new wxBoxSizer(wxHORIZONTAL);
    itemBoxSizer3->Add(itemBoxSizer4, 0, wxGROW, 5);

    wxBoxSizer* itemBoxSizer5 = new wxBoxSizer(wxVERTICAL);
    itemBoxSizer4->Add(itemBoxSizer5, 0, wxGROW, 5);

    wxStaticText* itemStaticText6 = new wxStaticText( itemPanel1, wxID_STATIC, _("&Alignment"), wxDefaultPosition, wxDefaultSize, 0 );
    itemBoxSizer5->Add(itemStaticText6, 0, wxALIGN_LEFT|wxLEFT|wxRIGHT|wxTOP|wxADJUST_MINSIZE, 5);

    wxBoxSizer* itemBoxSizer7 = new wxBoxSizer(wxHORIZONTAL);
    itemBoxSizer5->Add(itemBoxSizer7, 0, wxALIGN_CENTER_HORIZONTAL|wxALL, 5);

    itemBoxSizer7->Add(5, 5, 0, wxALIGN_CENTER_VERTICAL, 5);

    wxBoxSizer* itemBoxSizer9 = new wxBoxSizer(wxVERTICAL);
    itemBoxSizer7->Add(itemBoxSizer9, 0, wxALIGN_CENTER_VERTICAL|wxTOP, 5);

    m_alignmentLeft = new wxRadioButton( itemPanel1, ID_RICHTEXTINDENTSSPACINGPAGE_ALIGNMENT_LEFT, _("&Left"), wxDefaultPosition, wxDefaultSize, wxRB_GROUP );
    m_alignmentLeft->SetValue(false);
    m_alignmentLeft->SetHelpText(_("Left-align text."));
    if (wxRichTextIndentsSpacingPage::ShowToolTips())
        m_alignmentLeft->SetToolTip(_("Left-align text."));
    itemBoxSizer9->Add(m_alignmentLeft, 0, wxALIGN_LEFT|wxALL, 5);

    m_alignmentRight = new wxRadioButton( itemPanel1, ID_RICHTEXTINDENTSSPACINGPAGE_ALIGNMENT_RIGHT, _("&Right"), wxDefaultPosition, wxDefaultSize, 0 );
    m_alignmentRight->SetValue(false);
    m_alignmentRight->SetHelpText(_("Right-align text."));
    if (wxRichTextIndentsSpacingPage::ShowToolTips())
        m_alignmentRight->SetToolTip(_("Right-align text."));
    itemBoxSizer9->Add(m_alignmentRight, 0, wxALIGN_LEFT|wxALL, 5);

    m_alignmentJustified = new wxRadioButton( itemPanel1, ID_RICHTEXTINDENTSSPACINGPAGE_ALIGNMENT_JUSTIFIED, _("&Justified"), wxDefaultPosition, wxDefaultSize, 0 );
    m_alignmentJustified->SetValue(false);
    m_alignmentJustified->SetHelpText(_("Justify text left and right."));
    if (wxRichTextIndentsSpacingPage::ShowToolTips())
        m_alignmentJustified->SetToolTip(_("Justify text left and right."));
    itemBoxSizer9->Add(m_alignmentJustified, 0, wxALIGN_LEFT|wxALL, 5);

    m_alignmentCentred = new wxRadioButton( itemPanel1, ID_RICHTEXTINDENTSSPACINGPAGE_ALIGNMENT_CENTRED, _("Cen&tred"), wxDefaultPosition, wxDefaultSize, 0 );
    m_alignmentCentred->SetValue(false);
    m_alignmentCentred->SetHelpText(_("Centre text."));
    if (wxRichTextIndentsSpacingPage::ShowToolTips())
        m_alignmentCentred->SetToolTip(_("Centre text."));
    itemBoxSizer9->Add(m_alignmentCentred, 0, wxALIGN_LEFT|wxALL, 5);

    m_alignmentIndeterminate = new wxRadioButton( itemPanel1, ID_RICHTEXTINDENTSSPACINGPAGE_ALIGNMENT_INDETERMINATE, _("&Indeterminate"), wxDefaultPosition, wxDefaultSize, 0 );
    m_alignmentIndeterminate->SetValue(false);
    m_alignmentIndeterminate->SetHelpText(_("Use the current alignment setting."));
    if (wxRichTextIndentsSpacingPage::ShowToolTips())
        m_alignmentIndeterminate->SetToolTip(_("Use the current alignment setting."));
    itemBoxSizer9->Add(m_alignmentIndeterminate, 0, wxALIGN_LEFT|wxALL, 5);

    itemBoxSizer4->Add(2, 1, 1, wxALIGN_CENTER_VERTICAL|wxTOP|wxBOTTOM, 5);

    wxStaticLine* itemStaticLine16 = new wxStaticLine( itemPanel1, wxID_STATIC, wxDefaultPosition, wxDefaultSize, wxLI_VERTICAL );
    itemBoxSizer4->Add(itemStaticLine16, 0, wxGROW|wxLEFT|wxBOTTOM, 5);

    itemBoxSizer4->Add(2, 1, 1, wxALIGN_CENTER_VERTICAL|wxTOP|wxBOTTOM, 5);

    wxBoxSizer* itemBoxSizer18 = new wxBoxSizer(wxVERTICAL);
    itemBoxSizer4->Add(itemBoxSizer18, 0, wxGROW, 5);

    wxStaticText* itemStaticText19 = new wxStaticText( itemPanel1, wxID_STATIC, _("&Indentation (tenths of a mm)"), wxDefaultPosition, wxDefaultSize, 0 );
    itemBoxSizer18->Add(itemStaticText19, 0, wxALIGN_LEFT|wxLEFT|wxRIGHT|wxTOP|wxADJUST_MINSIZE, 5);

    wxBoxSizer* itemBoxSizer20 = new wxBoxSizer(wxHORIZONTAL);
    itemBoxSizer18->Add(itemBoxSizer20, 0, wxALIGN_LEFT|wxALL, 5);

    itemBoxSizer20->Add(5, 5, 0, wxALIGN_CENTER_VERTICAL, 5);

    wxFlexGridSizer* itemFlexGridSizer22 = new wxFlexGridSizer(4, 2, 0, 0);
    itemBoxSizer20->Add(itemFlexGridSizer22, 0, wxALIGN_CENTER_VERTICAL, 5);

    wxStaticText* itemStaticText23 = new wxStaticText( itemPanel1, wxID_STATIC, _("&Left:"), wxDefaultPosition, wxDefaultSize, 0 );
    itemFlexGridSizer22->Add(itemStaticText23, 0, wxALIGN_RIGHT|wxALIGN_CENTER_VERTICAL|wxALL|wxADJUST_MINSIZE, 5);

    wxBoxSizer* itemBoxSizer24 = new wxBoxSizer(wxHORIZONTAL);
    itemFlexGridSizer22->Add(itemBoxSizer24, 0, wxGROW|wxALIGN_CENTER_VERTICAL, 5);

    m_indentLeft = new wxTextCtrl( itemPanel1, ID_RICHTEXTINDENTSSPACINGPAGE_INDENT_LEFT, wxEmptyString, wxDefaultPosition, wxSize(50, -1), 0 );
    m_indentLeft->SetHelpText(_("The left indent."));
    if (wxRichTextIndentsSpacingPage::ShowToolTips())
        m_indentLeft->SetToolTip(_("The left indent."));
    itemBoxSizer24->Add(m_indentLeft, 1, wxALIGN_CENTER_VERTICAL|wxALL, 5);

    wxStaticText* itemStaticText26 = new wxStaticText( itemPanel1, wxID_STATIC, _("Left (&first line):"), wxDefaultPosition, wxDefaultSize, 0 );
    itemFlexGridSizer22->Add(itemStaticText26, 0, wxALIGN_RIGHT|wxALIGN_CENTER_VERTICAL|wxALL|wxADJUST_MINSIZE, 5);

    wxBoxSizer* itemBoxSizer27 = new wxBoxSizer(wxHORIZONTAL);
    itemFlexGridSizer22->Add(itemBoxSizer27, 0, wxGROW|wxALIGN_CENTER_VERTICAL, 5);

    m_indentLeftFirst = new wxTextCtrl( itemPanel1, ID_RICHTEXTINDENTSSPACINGPAGE_INDENT_LEFT_FIRST, wxEmptyString, wxDefaultPosition, wxSize(50, -1), 0 );
    m_indentLeftFirst->SetHelpText(_("The first line indent."));
    if (wxRichTextIndentsSpacingPage::ShowToolTips())
        m_indentLeftFirst->SetToolTip(_("The first line indent."));
    itemBoxSizer27->Add(m_indentLeftFirst, 1, wxALIGN_CENTER_VERTICAL|wxALL, 5);

    wxStaticText* itemStaticText29 = new wxStaticText( itemPanel1, wxID_STATIC, _("&Right:"), wxDefaultPosition, wxDefaultSize, 0 );
    itemFlexGridSizer22->Add(itemStaticText29, 0, wxALIGN_RIGHT|wxALIGN_CENTER_VERTICAL|wxALL|wxADJUST_MINSIZE, 5);

    wxBoxSizer* itemBoxSizer30 = new wxBoxSizer(wxHORIZONTAL);
    itemFlexGridSizer22->Add(itemBoxSizer30, 0, wxGROW|wxALIGN_CENTER_VERTICAL, 5);

    m_indentRight = new wxTextCtrl( itemPanel1, ID_RICHTEXTINDENTSSPACINGPAGE_INDENT_RIGHT, wxEmptyString, wxDefaultPosition, wxSize(50, -1), 0 );
    m_indentRight->SetHelpText(_("The right indent."));
    if (wxRichTextIndentsSpacingPage::ShowToolTips())
        m_indentRight->SetToolTip(_("The right indent."));
    itemBoxSizer30->Add(m_indentRight, 1, wxALIGN_CENTER_VERTICAL|wxALL, 5);

    wxStaticText* itemStaticText32 = new wxStaticText( itemPanel1, wxID_STATIC, _("&Outline level:"), wxDefaultPosition, wxDefaultSize, 0 );
    itemFlexGridSizer22->Add(itemStaticText32, 0, wxALIGN_RIGHT|wxALIGN_CENTER_VERTICAL|wxALL|wxADJUST_MINSIZE, 5);

    wxArrayString m_outlineLevelCtrlStrings;
    m_outlineLevelCtrlStrings.Add(_("Normal"));
    m_outlineLevelCtrlStrings.Add(_("1"));
    m_outlineLevelCtrlStrings.Add(_("2"));
    m_outlineLevelCtrlStrings.Add(_("3"));
    m_outlineLevelCtrlStrings.Add(_("4"));
    m_outlineLevelCtrlStrings.Add(_("5"));
    m_outlineLevelCtrlStrings.Add(_("6"));
    m_outlineLevelCtrlStrings.Add(_("7"));
    m_outlineLevelCtrlStrings.Add(_("8"));
    m_outlineLevelCtrlStrings.Add(_("9"));
    m_outlineLevelCtrl = new wxComboBox( itemPanel1, ID_RICHTEXTINDENTSSPACINGPAGE_OUTLINELEVEL, _("Normal"), wxDefaultPosition, wxSize(90, -1), m_outlineLevelCtrlStrings, wxCB_READONLY );
    m_outlineLevelCtrl->SetStringSelection(_("Normal"));
    m_outlineLevelCtrl->SetHelpText(_("The outline level."));
    if (wxRichTextIndentsSpacingPage::ShowToolTips())
        m_outlineLevelCtrl->SetToolTip(_("The outline level."));
    itemFlexGridSizer22->Add(m_outlineLevelCtrl, 1, wxGROW|wxALIGN_CENTER_VERTICAL|wxALL, 5);

    itemBoxSizer4->Add(2, 1, 1, wxALIGN_CENTER_VERTICAL|wxTOP|wxBOTTOM, 5);

    wxStaticLine* itemStaticLine35 = new wxStaticLine( itemPanel1, wxID_STATIC, wxDefaultPosition, wxDefaultSize, wxLI_VERTICAL );
    itemBoxSizer4->Add(itemStaticLine35, 0, wxGROW|wxTOP|wxBOTTOM, 5);

    itemBoxSizer4->Add(2, 1, 1, wxALIGN_CENTER_VERTICAL|wxTOP|wxBOTTOM, 5);

    wxBoxSizer* itemBoxSizer37 = new wxBoxSizer(wxVERTICAL);
    itemBoxSizer4->Add(itemBoxSizer37, 0, wxGROW, 5);

    wxStaticText* itemStaticText38 = new wxStaticText( itemPanel1, wxID_STATIC, _("&Spacing (tenths of a mm)"), wxDefaultPosition, wxDefaultSize, 0 );
    itemBoxSizer37->Add(itemStaticText38, 0, wxALIGN_LEFT|wxLEFT|wxRIGHT|wxTOP|wxADJUST_MINSIZE, 5);

    wxBoxSizer* itemBoxSizer39 = new wxBoxSizer(wxHORIZONTAL);
    itemBoxSizer37->Add(itemBoxSizer39, 0, wxALIGN_LEFT|wxALL, 5);

    itemBoxSizer39->Add(5, 5, 0, wxALIGN_CENTER_VERTICAL, 5);

    wxFlexGridSizer* itemFlexGridSizer41 = new wxFlexGridSizer(3, 2, 0, 0);
    itemFlexGridSizer41->AddGrowableCol(1);
    itemBoxSizer39->Add(itemFlexGridSizer41, 0, wxALIGN_CENTER_VERTICAL, 5);

    wxStaticText* itemStaticText42 = new wxStaticText( itemPanel1, wxID_STATIC, _("&Before a paragraph:"), wxDefaultPosition, wxDefaultSize, 0 );
    itemFlexGridSizer41->Add(itemStaticText42, 0, wxALIGN_RIGHT|wxALIGN_CENTER_VERTICAL|wxALL|wxADJUST_MINSIZE, 5);

    wxBoxSizer* itemBoxSizer43 = new wxBoxSizer(wxHORIZONTAL);
    itemFlexGridSizer41->Add(itemBoxSizer43, 0, wxGROW|wxALIGN_CENTER_VERTICAL, 5);

    m_spacingBefore = new wxTextCtrl( itemPanel1, ID_RICHTEXTINDENTSSPACINGPAGE_SPACING_BEFORE, wxEmptyString, wxDefaultPosition, wxSize(50, -1), 0 );
    m_spacingBefore->SetHelpText(_("The spacing before the paragraph."));
    if (wxRichTextIndentsSpacingPage::ShowToolTips())
        m_spacingBefore->SetToolTip(_("The spacing before the paragraph."));
    itemBoxSizer43->Add(m_spacingBefore, 1, wxALIGN_CENTER_VERTICAL|wxALL, 5);

    wxStaticText* itemStaticText45 = new wxStaticText( itemPanel1, wxID_STATIC, _("&After a paragraph:"), wxDefaultPosition, wxDefaultSize, 0 );
    itemFlexGridSizer41->Add(itemStaticText45, 0, wxALIGN_RIGHT|wxALIGN_CENTER_VERTICAL|wxALL|wxADJUST_MINSIZE, 5);

    wxBoxSizer* itemBoxSizer46 = new wxBoxSizer(wxHORIZONTAL);
    itemFlexGridSizer41->Add(itemBoxSizer46, 1, wxGROW|wxALIGN_CENTER_VERTICAL, 5);

    m_spacingAfter = new wxTextCtrl( itemPanel1, ID_RICHTEXTINDENTSSPACINGPAGE_SPACING_AFTER, wxEmptyString, wxDefaultPosition, wxSize(50, -1), 0 );
    if (wxRichTextIndentsSpacingPage::ShowToolTips())
        m_spacingAfter->SetToolTip(_("The spacing after the paragraph."));
    itemBoxSizer46->Add(m_spacingAfter, 1, wxALIGN_CENTER_VERTICAL|wxALL, 5);

    wxStaticText* itemStaticText48 = new wxStaticText( itemPanel1, wxID_STATIC, _("L&ine spacing:"), wxDefaultPosition, wxDefaultSize, 0 );
    itemFlexGridSizer41->Add(itemStaticText48, 0, wxALIGN_RIGHT|wxALIGN_CENTER_VERTICAL|wxALL|wxADJUST_MINSIZE, 5);

    wxBoxSizer* itemBoxSizer49 = new wxBoxSizer(wxHORIZONTAL);
    itemFlexGridSizer41->Add(itemBoxSizer49, 0, wxGROW|wxALIGN_CENTER_VERTICAL, 5);

    wxArrayString m_spacingLineStrings;
    m_spacingLineStrings.Add(_("Single"));
    m_spacingLineStrings.Add(_("1.1"));
    m_spacingLineStrings.Add(_("1.2"));
    m_spacingLineStrings.Add(_("1.3"));
    m_spacingLineStrings.Add(_("1.4"));
    m_spacingLineStrings.Add(_("1.5"));
    m_spacingLineStrings.Add(_("1.6"));
    m_spacingLineStrings.Add(_("1.7"));
    m_spacingLineStrings.Add(_("1.8"));
    m_spacingLineStrings.Add(_("1.9"));
    m_spacingLineStrings.Add(_("2"));
    m_spacingLine = new wxComboBox( itemPanel1, ID_RICHTEXTINDENTSSPACINGPAGE_SPACING_LINE, _("Single"), wxDefaultPosition, wxSize(90, -1), m_spacingLineStrings, wxCB_READONLY );
    m_spacingLine->SetStringSelection(_("Single"));
    m_spacingLine->SetHelpText(_("The line spacing."));
    if (wxRichTextIndentsSpacingPage::ShowToolTips())
        m_spacingLine->SetToolTip(_("The line spacing."));
    itemBoxSizer49->Add(m_spacingLine, 0, wxALIGN_CENTER_VERTICAL|wxALL, 5);

    itemBoxSizer3->Add(5, 5, 0, wxALIGN_CENTER_HORIZONTAL, 5);

    m_previewCtrl = new wxRichTextCtrl( itemPanel1, ID_RICHTEXTINDENTSSPACINGPAGE_PREVIEW_CTRL, wxEmptyString, wxDefaultPosition, wxSize(350, 100), wxVSCROLL|wxTE_READONLY );
    m_previewCtrl->SetHelpText(_("Shows a preview of the paragraph settings."));
    if (wxRichTextIndentsSpacingPage::ShowToolTips())
        m_previewCtrl->SetToolTip(_("Shows a preview of the paragraph settings."));
    itemBoxSizer3->Add(m_previewCtrl, 1, wxALIGN_CENTER_HORIZONTAL|wxALL, 5);

////@end wxRichTextIndentsSpacingPage content construction
}

wxTextAttrEx* wxRichTextIndentsSpacingPage::GetAttributes()
{
    return wxRichTextFormattingDialog::GetDialogAttributes(this);
}

/// Updates the font preview
void wxRichTextIndentsSpacingPage::UpdatePreview()
{
    static const wxChar* s_para1 = wxT("Lorem ipsum dolor sit amet, consectetuer adipiscing elit. \
Nullam ante sapien, vestibulum nonummy, pulvinar sed, luctus ut, lacus.\n");

    static const wxChar* s_para2 = wxT("Duis pharetra consequat dui. Cum sociis natoque penatibus \
et magnis dis parturient montes, nascetur ridiculus mus. Nullam vitae justo id mauris lobortis interdum.\n");

    static const wxChar* s_para3 = wxT("Integer convallis dolor at augue \
iaculis malesuada. Donec bibendum ipsum ut ante porta fringilla.\n");

    TransferDataFromWindow();
    wxTextAttrEx attr(*GetAttributes());
    attr.SetFlags(attr.GetFlags() &
      (wxTEXT_ATTR_ALIGNMENT|wxTEXT_ATTR_LEFT_INDENT|wxTEXT_ATTR_RIGHT_INDENT|wxTEXT_ATTR_PARA_SPACING_BEFORE|wxTEXT_ATTR_PARA_SPACING_AFTER|
       wxTEXT_ATTR_LINE_SPACING|
       wxTEXT_ATTR_BULLET_STYLE|wxTEXT_ATTR_BULLET_NUMBER|wxTEXT_ATTR_BULLET_TEXT));

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
    m_previewCtrl->WriteText(s_para2);
    m_previewCtrl->EndStyle();

    m_previewCtrl->BeginStyle(normalParaAttr);
    m_previewCtrl->WriteText(s_para3);
    m_previewCtrl->EndStyle();

    m_previewCtrl->Thaw();
}

/// Transfer data from/to window
bool wxRichTextIndentsSpacingPage::TransferDataFromWindow()
{
    wxPanel::TransferDataFromWindow();

    wxTextAttrEx* attr = GetAttributes();

    if (m_alignmentLeft->GetValue())
        attr->SetAlignment(wxTEXT_ALIGNMENT_LEFT);
    else if (m_alignmentCentred->GetValue())
        attr->SetAlignment(wxTEXT_ALIGNMENT_CENTRE);
    else if (m_alignmentRight->GetValue())
        attr->SetAlignment(wxTEXT_ALIGNMENT_RIGHT);
    else if (m_alignmentJustified->GetValue())
        attr->SetAlignment(wxTEXT_ALIGNMENT_JUSTIFIED);
    else
    {
        attr->SetAlignment(wxTEXT_ALIGNMENT_DEFAULT);
        attr->SetFlags(attr->GetFlags() & (~wxTEXT_ATTR_ALIGNMENT));
    }

    wxString leftIndent(m_indentLeft->GetValue());
    wxString leftFirstIndent(m_indentLeftFirst->GetValue());
    if (!leftIndent.empty())
    {
        int visualLeftIndent = wxAtoi(leftIndent);
        int visualLeftFirstIndent = wxAtoi(leftFirstIndent);
        int actualLeftIndent = visualLeftFirstIndent;
        int actualLeftSubIndent = visualLeftIndent - visualLeftFirstIndent;

        attr->SetLeftIndent(actualLeftIndent, actualLeftSubIndent);
    }
    else
        attr->SetFlags(attr->GetFlags() & (~wxTEXT_ATTR_LEFT_INDENT));

    wxString rightIndent(m_indentRight->GetValue());
    if (!rightIndent.empty())
        attr->SetRightIndent(wxAtoi(rightIndent));
    else
        attr->SetFlags(attr->GetFlags() & (~wxTEXT_ATTR_RIGHT_INDENT));

    wxString spacingAfter(m_spacingAfter->GetValue());
    if (!spacingAfter.empty())
        attr->SetParagraphSpacingAfter(wxAtoi(spacingAfter));
    else
        attr->SetFlags(attr->GetFlags() & (~wxTEXT_ATTR_PARA_SPACING_AFTER));

    wxString spacingBefore(m_spacingBefore->GetValue());
    if (!spacingBefore.empty())
        attr->SetParagraphSpacingBefore(wxAtoi(spacingBefore));
    else
        attr->SetFlags(attr->GetFlags() & (~wxTEXT_ATTR_PARA_SPACING_BEFORE));

    int spacingIndex = m_spacingLine->GetSelection();
    int lineSpacing = 0;
    if (spacingIndex != -1)
        lineSpacing = 10 + spacingIndex;

    if (lineSpacing == 0)
        attr->SetFlags(attr->GetFlags() & (~wxTEXT_ATTR_LINE_SPACING));
    else
        attr->SetLineSpacing(lineSpacing);

    int outlineLevel = m_outlineLevelCtrl->GetSelection();
    if (outlineLevel != wxNOT_FOUND)
        attr->SetOutlineLevel(outlineLevel);

    return true;
}

bool wxRichTextIndentsSpacingPage::TransferDataToWindow()
{
    m_dontUpdate = true;

    wxPanel::TransferDataToWindow();

    wxTextAttrEx* attr = GetAttributes();

    if (attr->HasAlignment())
    {
        if (attr->GetAlignment() == wxTEXT_ALIGNMENT_LEFT)
            m_alignmentLeft->SetValue(true);
        else if (attr->GetAlignment() == wxTEXT_ALIGNMENT_RIGHT)
            m_alignmentRight->SetValue(true);
        else if (attr->GetAlignment() == wxTEXT_ALIGNMENT_CENTRE)
            m_alignmentCentred->SetValue(true);
        else if (attr->GetAlignment() == wxTEXT_ALIGNMENT_JUSTIFIED)
            m_alignmentJustified->SetValue(true);
        else
            m_alignmentIndeterminate->SetValue(true);
    }
    else
        m_alignmentIndeterminate->SetValue(true);

    if (attr->HasLeftIndent())
    {
        wxString leftIndent(wxString::Format(wxT("%ld"), attr->GetLeftIndent() + attr->GetLeftSubIndent()));
        wxString leftFirstIndent(wxString::Format(wxT("%ld"), attr->GetLeftIndent()));

        m_indentLeft->SetValue(leftIndent);
        m_indentLeftFirst->SetValue(leftFirstIndent);
    }
    else
    {
        m_indentLeft->SetValue(wxEmptyString);
        m_indentLeftFirst->SetValue(wxEmptyString);
    }

    if (attr->HasRightIndent())
    {
        wxString rightIndent(wxString::Format(wxT("%ld"), attr->GetRightIndent()));

        m_indentRight->SetValue(rightIndent);
    }
    else
        m_indentRight->SetValue(wxEmptyString);

    if (attr->HasParagraphSpacingAfter())
    {
        wxString spacingAfter(wxString::Format(wxT("%d"), attr->GetParagraphSpacingAfter()));

        m_spacingAfter->SetValue(spacingAfter);
    }
    else
        m_spacingAfter->SetValue(wxEmptyString);

    if (attr->HasParagraphSpacingBefore())
    {
        wxString spacingBefore(wxString::Format(wxT("%d"), attr->GetParagraphSpacingBefore()));

        m_spacingBefore->SetValue(spacingBefore);
    }
    else
        m_spacingBefore->SetValue(wxEmptyString);

    if (attr->HasLineSpacing())
    {
        int index = 0;

        int lineSpacing = attr->GetLineSpacing();
        if (lineSpacing >= 10 && lineSpacing <= 20)
            index = lineSpacing - 10;
        else
            index = -1;

        m_spacingLine->SetSelection(index);
    }
    else
        m_spacingLine->SetSelection(-1);

    if (attr->HasOutlineLevel())
    {
        int outlineLevel = attr->GetOutlineLevel();
        if (outlineLevel < 0)
            outlineLevel = 0;
        if (outlineLevel > 9)
            outlineLevel = 9;

        m_outlineLevelCtrl->SetSelection(outlineLevel);
    }
    else
        m_outlineLevelCtrl->SetSelection(-1);

    UpdatePreview();

    m_dontUpdate = false;

    return true;
}


/*!
 * Should we show tooltips?
 */

bool wxRichTextIndentsSpacingPage::ShowToolTips()
{
    return wxRichTextFormattingDialog::ShowToolTips();
}

/*!
 * Get bitmap resources
 */

wxBitmap wxRichTextIndentsSpacingPage::GetBitmapResource( const wxString& name )
{
    // Bitmap retrieval
////@begin wxRichTextIndentsSpacingPage bitmap retrieval
    wxUnusedVar(name);
    return wxNullBitmap;
////@end wxRichTextIndentsSpacingPage bitmap retrieval
}

/*!
 * Get icon resources
 */

wxIcon wxRichTextIndentsSpacingPage::GetIconResource( const wxString& name )
{
    // Icon retrieval
////@begin wxRichTextIndentsSpacingPage icon retrieval
    wxUnusedVar(name);
    return wxNullIcon;
////@end wxRichTextIndentsSpacingPage icon retrieval
}
/*!
 * wxEVT_COMMAND_RADIOBUTTON_SELECTED event handler for ID_RICHTEXTINDENTSSPACINGPAGE_ALIGNMENT_LEFT
 */

void wxRichTextIndentsSpacingPage::OnAlignmentLeftSelected( wxCommandEvent& WXUNUSED(event) )
{
    if (!m_dontUpdate)
        UpdatePreview();
}


/*!
 * wxEVT_COMMAND_RADIOBUTTON_SELECTED event handler for ID_RICHTEXTINDENTSSPACINGPAGE_ALIGNMENT_RIGHT
 */

void wxRichTextIndentsSpacingPage::OnAlignmentRightSelected( wxCommandEvent& WXUNUSED(event) )
{
    if (!m_dontUpdate)
        UpdatePreview();
}


/*!
 * wxEVT_COMMAND_RADIOBUTTON_SELECTED event handler for ID_RICHTEXTINDENTSSPACINGPAGE_ALIGNMENT_JUSTIFIED
 */

void wxRichTextIndentsSpacingPage::OnAlignmentJustifiedSelected( wxCommandEvent& WXUNUSED(event) )
{
    if (!m_dontUpdate)
        UpdatePreview();
}


/*!
 * wxEVT_COMMAND_RADIOBUTTON_SELECTED event handler for ID_RICHTEXTINDENTSSPACINGPAGE_ALIGNMENT_CENTRED
 */

void wxRichTextIndentsSpacingPage::OnAlignmentCentredSelected( wxCommandEvent& WXUNUSED(event) )
{
    if (!m_dontUpdate)
        UpdatePreview();
}


/*!
 * wxEVT_COMMAND_RADIOBUTTON_SELECTED event handler for ID_RICHTEXTINDENTSSPACINGPAGE_ALIGNMENT_INDETERMINATE
 */

void wxRichTextIndentsSpacingPage::OnAlignmentIndeterminateSelected( wxCommandEvent& WXUNUSED(event) )
{
    if (!m_dontUpdate)
        UpdatePreview();
}


/*!
 * wxEVT_COMMAND_TEXT_UPDATED event handler for ID_RICHTEXTINDENTSSPACINGPAGE_INDENT_LEFT
 */

void wxRichTextIndentsSpacingPage::OnIndentLeftUpdated( wxCommandEvent& WXUNUSED(event) )
{
    if (!m_dontUpdate)
        UpdatePreview();
}


/*!
 * wxEVT_COMMAND_TEXT_UPDATED event handler for ID_RICHTEXTINDENTSSPACINGPAGE_INDENT_LEFT_FIRST
 */

void wxRichTextIndentsSpacingPage::OnIndentLeftFirstUpdated( wxCommandEvent& WXUNUSED(event) )
{
    if (!m_dontUpdate)
        UpdatePreview();
}


/*!
 * wxEVT_COMMAND_TEXT_UPDATED event handler for ID_RICHTEXTINDENTSSPACINGPAGE_INDENT_RIGHT
 */

void wxRichTextIndentsSpacingPage::OnIndentRightUpdated( wxCommandEvent& WXUNUSED(event) )
{
    if (!m_dontUpdate)
        UpdatePreview();
}


/*!
 * wxEVT_COMMAND_TEXT_UPDATED event handler for ID_RICHTEXTINDENTSSPACINGPAGE_SPACING_BEFORE
 */

void wxRichTextIndentsSpacingPage::OnSpacingBeforeUpdated( wxCommandEvent& WXUNUSED(event) )
{
    if (!m_dontUpdate)
        UpdatePreview();
}


/*!
 * wxEVT_COMMAND_TEXT_UPDATED event handler for ID_RICHTEXTINDENTSSPACINGPAGE_SPACING_AFTER
 */

void wxRichTextIndentsSpacingPage::OnSpacingAfterUpdated( wxCommandEvent& WXUNUSED(event) )
{
    if (!m_dontUpdate)
        UpdatePreview();
}

/*!
 * wxEVT_COMMAND_COMBOBOX_SELECTED event handler for ID_RICHTEXTINDENTSSPACINGPAGE_SPACING_LINE
 */

void wxRichTextIndentsSpacingPage::OnSpacingLineSelected( wxCommandEvent& WXUNUSED(event) )
{
    if (!m_dontUpdate)
        UpdatePreview();
}

/*!
 * wxEVT_COMMAND_COMBOBOX_SELECTED event handler for ID_RICHTEXTINDENTSSPACINGPAGE_OUTLINELEVEL
 */

void wxRichTextIndentsSpacingPage::OnRichtextOutlinelevelSelected( wxCommandEvent& WXUNUSED(event) )
{
    if (!m_dontUpdate)
        UpdatePreview();
}

#endif // wxUSE_RICHTEXT
