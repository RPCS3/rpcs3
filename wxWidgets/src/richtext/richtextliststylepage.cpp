/////////////////////////////////////////////////////////////////////////////
// Name:        src/richtext/richtextliststylepage.cpp
// Purpose:
// Author:      Julian Smart
// Modified by:
// Created:     10/18/2006 11:36:37 AM
// RCS-ID:      $Id: richtextliststylepage.cpp 54423 2008-06-29 17:04:04Z JS $
// Copyright:   (c) Julian Smart
// Licence:     wxWindows licence
/////////////////////////////////////////////////////////////////////////////

#include "wx/richtext/richtextliststylepage.h"

////@begin XPM images
////@end XPM images

/*!
 * wxRichTextListStylePage type definition
 */

IMPLEMENT_DYNAMIC_CLASS( wxRichTextListStylePage, wxPanel )

/*!
 * wxRichTextListStylePage event table definition
 */

BEGIN_EVENT_TABLE( wxRichTextListStylePage, wxPanel )

////@begin wxRichTextListStylePage event table entries
    EVT_SPINCTRL( ID_RICHTEXTLISTSTYLEPAGE_LEVEL, wxRichTextListStylePage::OnLevelUpdated )
    EVT_SPIN_UP( ID_RICHTEXTLISTSTYLEPAGE_LEVEL, wxRichTextListStylePage::OnLevelUp )
    EVT_SPIN_DOWN( ID_RICHTEXTLISTSTYLEPAGE_LEVEL, wxRichTextListStylePage::OnLevelDown )
    EVT_TEXT( ID_RICHTEXTLISTSTYLEPAGE_LEVEL, wxRichTextListStylePage::OnLevelTextUpdated )
    EVT_UPDATE_UI( ID_RICHTEXTLISTSTYLEPAGE_LEVEL, wxRichTextListStylePage::OnLevelUIUpdate )

    EVT_BUTTON( ID_RICHTEXTLISTSTYLEPAGE_CHOOSE_FONT, wxRichTextListStylePage::OnChooseFontClick )

    EVT_LISTBOX( ID_RICHTEXTLISTSTYLEPAGE_STYLELISTBOX, wxRichTextListStylePage::OnStylelistboxSelected )

    EVT_CHECKBOX( ID_RICHTEXTLISTSTYLEPAGE_PERIODCTRL, wxRichTextListStylePage::OnPeriodctrlClick )
    EVT_UPDATE_UI( ID_RICHTEXTLISTSTYLEPAGE_PERIODCTRL, wxRichTextListStylePage::OnPeriodctrlUpdate )

    EVT_CHECKBOX( ID_RICHTEXTLISTSTYLEPAGE_PARENTHESESCTRL, wxRichTextListStylePage::OnParenthesesctrlClick )
    EVT_UPDATE_UI( ID_RICHTEXTLISTSTYLEPAGE_PARENTHESESCTRL, wxRichTextListStylePage::OnParenthesesctrlUpdate )

    EVT_CHECKBOX( ID_RICHTEXTLISTSTYLEPAGE_RIGHTPARENTHESISCTRL, wxRichTextListStylePage::OnRightParenthesisCtrlClick )
    EVT_UPDATE_UI( ID_RICHTEXTLISTSTYLEPAGE_RIGHTPARENTHESISCTRL, wxRichTextListStylePage::OnRightParenthesisCtrlUpdate )

    EVT_COMBOBOX( ID_RICHTEXTLISTSTYLEPAGE_BULLETALIGNMENTCTRL, wxRichTextListStylePage::OnBulletAlignmentCtrlSelected )

    EVT_UPDATE_UI( ID_RICHTEXTLISTSTYLEPAGE_SYMBOLSTATIC, wxRichTextListStylePage::OnSymbolstaticUpdate )

    EVT_COMBOBOX( ID_RICHTEXTLISTSTYLEPAGE_SYMBOLCTRL, wxRichTextListStylePage::OnSymbolctrlSelected )
    EVT_TEXT( ID_RICHTEXTLISTSTYLEPAGE_SYMBOLCTRL, wxRichTextListStylePage::OnSymbolctrlUpdated )
    EVT_UPDATE_UI( ID_RICHTEXTLISTSTYLEPAGE_SYMBOLCTRL, wxRichTextListStylePage::OnSymbolctrlUIUpdate )

    EVT_BUTTON( ID_RICHTEXTLISTSTYLEPAGE_CHOOSE_SYMBOL, wxRichTextListStylePage::OnChooseSymbolClick )
    EVT_UPDATE_UI( ID_RICHTEXTLISTSTYLEPAGE_CHOOSE_SYMBOL, wxRichTextListStylePage::OnChooseSymbolUpdate )

    EVT_COMBOBOX( ID_RICHTEXTLISTSTYLEPAGE_SYMBOLFONTCTRL, wxRichTextListStylePage::OnSymbolfontctrlSelected )
    EVT_TEXT( ID_RICHTEXTLISTSTYLEPAGE_SYMBOLFONTCTRL, wxRichTextListStylePage::OnSymbolfontctrlUpdated )
    EVT_UPDATE_UI( ID_RICHTEXTLISTSTYLEPAGE_SYMBOLFONTCTRL, wxRichTextListStylePage::OnSymbolfontctrlUIUpdate )

    EVT_UPDATE_UI( ID_RICHTEXTLISTSTYLEPAGE_NAMESTATIC, wxRichTextListStylePage::OnNamestaticUpdate )

    EVT_COMBOBOX( ID_RICHTEXTLISTSTYLEPAGE_NAMECTRL, wxRichTextListStylePage::OnNamectrlSelected )
    EVT_TEXT( ID_RICHTEXTLISTSTYLEPAGE_NAMECTRL, wxRichTextListStylePage::OnNamectrlUpdated )
    EVT_UPDATE_UI( ID_RICHTEXTLISTSTYLEPAGE_NAMECTRL, wxRichTextListStylePage::OnNamectrlUIUpdate )

    EVT_RADIOBUTTON( ID_RICHTEXTLISTSTYLEPAGE_ALIGNLEFT, wxRichTextListStylePage::OnRichtextliststylepageAlignleftSelected )

    EVT_RADIOBUTTON( ID_RICHTEXTLISTSTYLEPAGE_ALIGNRIGHT, wxRichTextListStylePage::OnRichtextliststylepageAlignrightSelected )

    EVT_RADIOBUTTON( ID_RICHTEXTLISTSTYLEPAGE_JUSTIFIED, wxRichTextListStylePage::OnRichtextliststylepageJustifiedSelected )

    EVT_RADIOBUTTON( ID_RICHTEXTLISTSTYLEPAGE_CENTERED, wxRichTextListStylePage::OnRichtextliststylepageCenteredSelected )

    EVT_RADIOBUTTON( ID_RICHTEXTLISTSTYLEPAGE_ALIGNINDETERMINATE, wxRichTextListStylePage::OnRichtextliststylepageAlignindeterminateSelected )

    EVT_TEXT( ID_RICHTEXTLISTSTYLEPAGE_INDENTLEFT, wxRichTextListStylePage::OnIndentLeftUpdated )

    EVT_TEXT( ID_RICHTEXTLISTSTYLEPAGE_INDENTFIRSTLINE, wxRichTextListStylePage::OnIndentFirstLineUpdated )

    EVT_TEXT( ID_RICHTEXTLISTSTYLEPAGE_INDENTRIGHT, wxRichTextListStylePage::OnIndentRightUpdated )

    EVT_TEXT( ID_RICHTEXTLISTSTYLEPAGE_SPACINGBEFORE, wxRichTextListStylePage::OnSpacingBeforeUpdated )

    EVT_TEXT( ID_RICHTEXTLISTSTYLEPAGE_SPACINGAFTER, wxRichTextListStylePage::OnSpacingAfterUpdated )

    EVT_COMBOBOX( ID_RICHTEXTLISTSTYLEPAGE_LINESPACING, wxRichTextListStylePage::OnLineSpacingSelected )

////@end wxRichTextListStylePage event table entries

END_EVENT_TABLE()

/*!
 * wxRichTextListStylePage constructors
 */

wxRichTextListStylePage::wxRichTextListStylePage( )
{
    Init();
}

wxRichTextListStylePage::wxRichTextListStylePage( wxWindow* parent, wxWindowID id, const wxPoint& pos, const wxSize& size, long style )
{
    Init();
    Create(parent, id, pos, size, style);
}

/*!
 * wxRichTextListStylePage creator
 */

bool wxRichTextListStylePage::Create( wxWindow* parent, wxWindowID id, const wxPoint& pos, const wxSize& size, long style )
{
////@begin wxRichTextListStylePage creation
    wxPanel::Create( parent, id, pos, size, style );

    CreateControls();
    if (GetSizer())
    {
        GetSizer()->SetSizeHints(this);
    }
    Centre();
////@end wxRichTextListStylePage creation
    return true;
}

/*!
 * Member initialisation
 */

void wxRichTextListStylePage::Init()
{
    m_dontUpdate = false;
    m_currentLevel = 1;

////@begin wxRichTextListStylePage member initialisation
    m_levelCtrl = NULL;
    m_styleListBox = NULL;
    m_periodCtrl = NULL;
    m_parenthesesCtrl = NULL;
    m_rightParenthesisCtrl = NULL;
    m_bulletAlignmentCtrl = NULL;
    m_symbolCtrl = NULL;
    m_symbolFontCtrl = NULL;
    m_bulletNameCtrl = NULL;
    m_alignmentLeft = NULL;
    m_alignmentRight = NULL;
    m_alignmentJustified = NULL;
    m_alignmentCentred = NULL;
    m_alignmentIndeterminate = NULL;
    m_indentLeft = NULL;
    m_indentLeftFirst = NULL;
    m_indentRight = NULL;
    m_spacingBefore = NULL;
    m_spacingAfter = NULL;
    m_spacingLine = NULL;
    m_previewCtrl = NULL;
////@end wxRichTextListStylePage member initialisation
}

/*!
 * Control creation for wxRichTextListStylePage
 */

void wxRichTextListStylePage::CreateControls()
{
////@begin wxRichTextListStylePage content construction
    wxRichTextListStylePage* itemPanel1 = this;

    wxBoxSizer* itemBoxSizer2 = new wxBoxSizer(wxVERTICAL);
    itemPanel1->SetSizer(itemBoxSizer2);

    wxBoxSizer* itemBoxSizer3 = new wxBoxSizer(wxVERTICAL);
    itemBoxSizer2->Add(itemBoxSizer3, 1, wxGROW|wxALL, 5);

    wxBoxSizer* itemBoxSizer4 = new wxBoxSizer(wxHORIZONTAL);
    itemBoxSizer3->Add(itemBoxSizer4, 0, wxALIGN_CENTER_HORIZONTAL, 5);

    wxStaticText* itemStaticText5 = new wxStaticText( itemPanel1, wxID_STATIC, _("&List level:"), wxDefaultPosition, wxDefaultSize, 0 );
    itemBoxSizer4->Add(itemStaticText5, 0, wxALIGN_CENTER_VERTICAL|wxALL|wxADJUST_MINSIZE, 5);

    m_levelCtrl = new wxSpinCtrl( itemPanel1, ID_RICHTEXTLISTSTYLEPAGE_LEVEL, _T("1"), wxDefaultPosition, wxSize(60, -1), wxSP_ARROW_KEYS, 1, 10, 1 );
    m_levelCtrl->SetHelpText(_("Selects the list level to edit."));
    if (wxRichTextListStylePage::ShowToolTips())
        m_levelCtrl->SetToolTip(_("Selects the list level to edit."));
    itemBoxSizer4->Add(m_levelCtrl, 0, wxALIGN_CENTER_VERTICAL|wxALL, 5);

    itemBoxSizer4->Add(5, 5, 0, wxALIGN_CENTER_VERTICAL|wxALL, 5);

    wxButton* itemButton8 = new wxButton( itemPanel1, ID_RICHTEXTLISTSTYLEPAGE_CHOOSE_FONT, _("&Font for Level..."), wxDefaultPosition, wxDefaultSize, 0 );
    itemButton8->SetHelpText(_("Click to choose the font for this level."));
    if (wxRichTextListStylePage::ShowToolTips())
        itemButton8->SetToolTip(_("Click to choose the font for this level."));
    itemBoxSizer4->Add(itemButton8, 0, wxALIGN_CENTER_VERTICAL|wxALL, 5);

    wxNotebook* itemNotebook9 = new wxNotebook( itemPanel1, ID_RICHTEXTLISTSTYLEPAGE_NOTEBOOK, wxDefaultPosition, wxDefaultSize, wxNB_DEFAULT|wxNB_TOP );

    wxPanel* itemPanel10 = new wxPanel( itemNotebook9, ID_RICHTEXTLISTSTYLEPAGE_BULLETS, wxDefaultPosition, wxDefaultSize, wxNO_BORDER|wxTAB_TRAVERSAL );
    wxBoxSizer* itemBoxSizer11 = new wxBoxSizer(wxVERTICAL);
    itemPanel10->SetSizer(itemBoxSizer11);

    wxBoxSizer* itemBoxSizer12 = new wxBoxSizer(wxHORIZONTAL);
    itemBoxSizer11->Add(itemBoxSizer12, 1, wxGROW, 5);
    wxBoxSizer* itemBoxSizer13 = new wxBoxSizer(wxVERTICAL);
    itemBoxSizer12->Add(itemBoxSizer13, 0, wxGROW, 5);
    wxStaticText* itemStaticText14 = new wxStaticText( itemPanel10, wxID_STATIC, _("&Bullet style:"), wxDefaultPosition, wxDefaultSize, 0 );
    itemBoxSizer13->Add(itemStaticText14, 0, wxALIGN_LEFT|wxLEFT|wxRIGHT|wxTOP|wxADJUST_MINSIZE, 5);

    wxArrayString m_styleListBoxStrings;
    m_styleListBox = new wxListBox( itemPanel10, ID_RICHTEXTLISTSTYLEPAGE_STYLELISTBOX, wxDefaultPosition, wxSize(-1, 90), m_styleListBoxStrings, wxLB_SINGLE );
    m_styleListBox->SetHelpText(_("The available bullet styles."));
    if (wxRichTextListStylePage::ShowToolTips())
        m_styleListBox->SetToolTip(_("The available bullet styles."));
    itemBoxSizer13->Add(m_styleListBox, 1, wxGROW|wxALL, 5);

    wxBoxSizer* itemBoxSizer16 = new wxBoxSizer(wxHORIZONTAL);
    itemBoxSizer13->Add(itemBoxSizer16, 0, wxGROW, 5);
    m_periodCtrl = new wxCheckBox( itemPanel10, ID_RICHTEXTLISTSTYLEPAGE_PERIODCTRL, _("Peri&od"), wxDefaultPosition, wxDefaultSize, 0 );
    m_periodCtrl->SetValue(false);
    m_periodCtrl->SetHelpText(_("Check to add a period after the bullet."));
    if (wxRichTextListStylePage::ShowToolTips())
        m_periodCtrl->SetToolTip(_("Check to add a period after the bullet."));
    itemBoxSizer16->Add(m_periodCtrl, 0, wxALIGN_CENTER_VERTICAL|wxALL, 5);

    m_parenthesesCtrl = new wxCheckBox( itemPanel10, ID_RICHTEXTLISTSTYLEPAGE_PARENTHESESCTRL, _("(*)"), wxDefaultPosition, wxDefaultSize, 0 );
    m_parenthesesCtrl->SetValue(false);
    m_parenthesesCtrl->SetHelpText(_("Check to enclose the bullet in parentheses."));
    if (wxRichTextListStylePage::ShowToolTips())
        m_parenthesesCtrl->SetToolTip(_("Check to enclose the bullet in parentheses."));
    itemBoxSizer16->Add(m_parenthesesCtrl, 0, wxALIGN_CENTER_VERTICAL|wxALL, 5);

    m_rightParenthesisCtrl = new wxCheckBox( itemPanel10, ID_RICHTEXTLISTSTYLEPAGE_RIGHTPARENTHESISCTRL, _("*)"), wxDefaultPosition, wxDefaultSize, 0 );
    m_rightParenthesisCtrl->SetValue(false);
    m_rightParenthesisCtrl->SetHelpText(_("Check to add a right parenthesis."));
    if (wxRichTextListStylePage::ShowToolTips())
        m_rightParenthesisCtrl->SetToolTip(_("Check to add a right parenthesis."));
    itemBoxSizer16->Add(m_rightParenthesisCtrl, 0, wxALIGN_CENTER_VERTICAL|wxALL, 5);

    itemBoxSizer13->Add(2, 1, 0, wxALIGN_CENTER_HORIZONTAL, 5);

    wxStaticText* itemStaticText21 = new wxStaticText( itemPanel10, wxID_STATIC, _("Bullet &Alignment:"), wxDefaultPosition, wxDefaultSize, 0 );
    itemBoxSizer13->Add(itemStaticText21, 0, wxALIGN_LEFT|wxLEFT|wxRIGHT|wxTOP|wxADJUST_MINSIZE, 5);

    wxArrayString m_bulletAlignmentCtrlStrings;
    m_bulletAlignmentCtrlStrings.Add(_("Left"));
    m_bulletAlignmentCtrlStrings.Add(_("Centre"));
    m_bulletAlignmentCtrlStrings.Add(_("Right"));
    m_bulletAlignmentCtrl = new wxComboBox( itemPanel10, ID_RICHTEXTLISTSTYLEPAGE_BULLETALIGNMENTCTRL, _("Left"), wxDefaultPosition, wxSize(60, -1), m_bulletAlignmentCtrlStrings, wxCB_READONLY );
    m_bulletAlignmentCtrl->SetStringSelection(_("Left"));
    m_bulletAlignmentCtrl->SetHelpText(_("The bullet character."));
    if (wxRichTextListStylePage::ShowToolTips())
        m_bulletAlignmentCtrl->SetToolTip(_("The bullet character."));
    itemBoxSizer13->Add(m_bulletAlignmentCtrl, 0, wxGROW|wxALL|wxFIXED_MINSIZE, 5);

    itemBoxSizer12->Add(2, 1, 1, wxALIGN_CENTER_VERTICAL|wxTOP|wxBOTTOM, 5);

    wxStaticLine* itemStaticLine24 = new wxStaticLine( itemPanel10, wxID_STATIC, wxDefaultPosition, wxDefaultSize, wxLI_VERTICAL );
    itemBoxSizer12->Add(itemStaticLine24, 0, wxGROW|wxALL, 5);

    itemBoxSizer12->Add(2, 1, 1, wxALIGN_CENTER_VERTICAL|wxTOP|wxBOTTOM, 5);

    wxBoxSizer* itemBoxSizer26 = new wxBoxSizer(wxVERTICAL);
    itemBoxSizer12->Add(itemBoxSizer26, 0, wxGROW, 5);
    wxStaticText* itemStaticText27 = new wxStaticText( itemPanel10, ID_RICHTEXTLISTSTYLEPAGE_SYMBOLSTATIC, _("&Symbol:"), wxDefaultPosition, wxDefaultSize, 0 );
    itemBoxSizer26->Add(itemStaticText27, 0, wxALIGN_LEFT|wxLEFT|wxRIGHT|wxTOP|wxADJUST_MINSIZE, 5);

    wxBoxSizer* itemBoxSizer28 = new wxBoxSizer(wxHORIZONTAL);
    itemBoxSizer26->Add(itemBoxSizer28, 0, wxGROW, 5);
    wxArrayString m_symbolCtrlStrings;
    m_symbolCtrl = new wxComboBox( itemPanel10, ID_RICHTEXTLISTSTYLEPAGE_SYMBOLCTRL, _T(""), wxDefaultPosition, wxSize(60, -1), m_symbolCtrlStrings, wxCB_DROPDOWN );
    m_symbolCtrl->SetHelpText(_("The bullet character."));
    if (wxRichTextListStylePage::ShowToolTips())
        m_symbolCtrl->SetToolTip(_("The bullet character."));
    itemBoxSizer28->Add(m_symbolCtrl, 0, wxALIGN_CENTER_VERTICAL|wxALL|wxFIXED_MINSIZE, 5);

    wxButton* itemButton30 = new wxButton( itemPanel10, ID_RICHTEXTLISTSTYLEPAGE_CHOOSE_SYMBOL, _("Ch&oose..."), wxDefaultPosition, wxDefaultSize, 0 );
    itemButton30->SetHelpText(_("Click to browse for a symbol."));
    if (wxRichTextListStylePage::ShowToolTips())
        itemButton30->SetToolTip(_("Click to browse for a symbol."));
    itemBoxSizer28->Add(itemButton30, 0, wxALIGN_CENTER_VERTICAL|wxALL, 5);

    itemBoxSizer26->Add(5, 5, 0, wxALIGN_CENTER_HORIZONTAL, 5);

    wxStaticText* itemStaticText32 = new wxStaticText( itemPanel10, ID_RICHTEXTLISTSTYLEPAGE_SYMBOLSTATIC, _("Symbol &font:"), wxDefaultPosition, wxDefaultSize, 0 );
    itemBoxSizer26->Add(itemStaticText32, 0, wxALIGN_LEFT|wxLEFT|wxRIGHT|wxTOP|wxADJUST_MINSIZE, 5);

    wxArrayString m_symbolFontCtrlStrings;
    m_symbolFontCtrl = new wxComboBox( itemPanel10, ID_RICHTEXTLISTSTYLEPAGE_SYMBOLFONTCTRL, _T(""), wxDefaultPosition, wxDefaultSize, m_symbolFontCtrlStrings, wxCB_DROPDOWN );
    if (wxRichTextListStylePage::ShowToolTips())
        m_symbolFontCtrl->SetToolTip(_("Available fonts."));
    itemBoxSizer26->Add(m_symbolFontCtrl, 0, wxGROW|wxALL, 5);

    itemBoxSizer26->Add(5, 5, 1, wxALIGN_CENTER_HORIZONTAL, 5);

    wxStaticText* itemStaticText35 = new wxStaticText( itemPanel10, ID_RICHTEXTLISTSTYLEPAGE_NAMESTATIC, _("S&tandard bullet name:"), wxDefaultPosition, wxDefaultSize, 0 );
    itemBoxSizer26->Add(itemStaticText35, 0, wxALIGN_LEFT|wxLEFT|wxRIGHT|wxTOP|wxADJUST_MINSIZE, 5);

    wxArrayString m_bulletNameCtrlStrings;
    m_bulletNameCtrl = new wxComboBox( itemPanel10, ID_RICHTEXTLISTSTYLEPAGE_NAMECTRL, _T(""), wxDefaultPosition, wxDefaultSize, m_bulletNameCtrlStrings, wxCB_DROPDOWN );
    m_bulletNameCtrl->SetHelpText(_("A standard bullet name."));
    if (wxRichTextListStylePage::ShowToolTips())
        m_bulletNameCtrl->SetToolTip(_("A standard bullet name."));
    itemBoxSizer26->Add(m_bulletNameCtrl, 0, wxGROW|wxALL, 5);

    itemNotebook9->AddPage(itemPanel10, _("Bullet style"));

    wxPanel* itemPanel37 = new wxPanel( itemNotebook9, ID_RICHTEXTLISTSTYLEPAGE_SPACING, wxDefaultPosition, wxDefaultSize, wxNO_BORDER|wxTAB_TRAVERSAL );
    wxBoxSizer* itemBoxSizer38 = new wxBoxSizer(wxVERTICAL);
    itemPanel37->SetSizer(itemBoxSizer38);

    wxBoxSizer* itemBoxSizer39 = new wxBoxSizer(wxHORIZONTAL);
    itemBoxSizer38->Add(itemBoxSizer39, 0, wxGROW, 5);
    wxBoxSizer* itemBoxSizer40 = new wxBoxSizer(wxVERTICAL);
    itemBoxSizer39->Add(itemBoxSizer40, 0, wxGROW, 5);
    wxStaticText* itemStaticText41 = new wxStaticText( itemPanel37, wxID_STATIC, _("&Alignment"), wxDefaultPosition, wxDefaultSize, 0 );
    itemBoxSizer40->Add(itemStaticText41, 0, wxALIGN_LEFT|wxLEFT|wxRIGHT|wxTOP|wxADJUST_MINSIZE, 5);

    wxBoxSizer* itemBoxSizer42 = new wxBoxSizer(wxHORIZONTAL);
    itemBoxSizer40->Add(itemBoxSizer42, 0, wxALIGN_CENTER_HORIZONTAL|wxALL, 5);
    itemBoxSizer42->Add(5, 5, 0, wxALIGN_CENTER_VERTICAL, 5);

    wxBoxSizer* itemBoxSizer44 = new wxBoxSizer(wxVERTICAL);
    itemBoxSizer42->Add(itemBoxSizer44, 0, wxALIGN_CENTER_VERTICAL|wxTOP, 5);
    m_alignmentLeft = new wxRadioButton( itemPanel37, ID_RICHTEXTLISTSTYLEPAGE_ALIGNLEFT, _("&Left"), wxDefaultPosition, wxDefaultSize, wxRB_GROUP );
    m_alignmentLeft->SetValue(false);
    m_alignmentLeft->SetHelpText(_("Left-align text."));
    if (wxRichTextListStylePage::ShowToolTips())
        m_alignmentLeft->SetToolTip(_("Left-align text."));
    itemBoxSizer44->Add(m_alignmentLeft, 0, wxALIGN_LEFT|wxALL, 5);

    m_alignmentRight = new wxRadioButton( itemPanel37, ID_RICHTEXTLISTSTYLEPAGE_ALIGNRIGHT, _("&Right"), wxDefaultPosition, wxDefaultSize, 0 );
    m_alignmentRight->SetValue(false);
    m_alignmentRight->SetHelpText(_("Right-align text."));
    if (wxRichTextListStylePage::ShowToolTips())
        m_alignmentRight->SetToolTip(_("Right-align text."));
    itemBoxSizer44->Add(m_alignmentRight, 0, wxALIGN_LEFT|wxALL, 5);

    m_alignmentJustified = new wxRadioButton( itemPanel37, ID_RICHTEXTLISTSTYLEPAGE_JUSTIFIED, _("&Justified"), wxDefaultPosition, wxDefaultSize, 0 );
    m_alignmentJustified->SetValue(false);
    m_alignmentJustified->SetHelpText(_("Justify text left and right."));
    if (wxRichTextListStylePage::ShowToolTips())
        m_alignmentJustified->SetToolTip(_("Justify text left and right."));
    itemBoxSizer44->Add(m_alignmentJustified, 0, wxALIGN_LEFT|wxALL, 5);

    m_alignmentCentred = new wxRadioButton( itemPanel37, ID_RICHTEXTLISTSTYLEPAGE_CENTERED, _("Cen&tred"), wxDefaultPosition, wxDefaultSize, 0 );
    m_alignmentCentred->SetValue(false);
    m_alignmentCentred->SetHelpText(_("Centre text."));
    if (wxRichTextListStylePage::ShowToolTips())
        m_alignmentCentred->SetToolTip(_("Centre text."));
    itemBoxSizer44->Add(m_alignmentCentred, 0, wxALIGN_LEFT|wxALL, 5);

    m_alignmentIndeterminate = new wxRadioButton( itemPanel37, ID_RICHTEXTLISTSTYLEPAGE_ALIGNINDETERMINATE, _("&Indeterminate"), wxDefaultPosition, wxDefaultSize, 0 );
    m_alignmentIndeterminate->SetValue(false);
    m_alignmentIndeterminate->SetHelpText(_("Use the current alignment setting."));
    if (wxRichTextListStylePage::ShowToolTips())
        m_alignmentIndeterminate->SetToolTip(_("Use the current alignment setting."));
    itemBoxSizer44->Add(m_alignmentIndeterminate, 0, wxALIGN_LEFT|wxALL, 5);

    itemBoxSizer39->Add(2, 1, 1, wxALIGN_CENTER_VERTICAL|wxTOP|wxBOTTOM, 5);

    wxStaticLine* itemStaticLine51 = new wxStaticLine( itemPanel37, wxID_STATIC, wxDefaultPosition, wxDefaultSize, wxLI_VERTICAL );
    itemBoxSizer39->Add(itemStaticLine51, 0, wxGROW|wxLEFT|wxBOTTOM, 5);

    itemBoxSizer39->Add(2, 1, 1, wxALIGN_CENTER_VERTICAL|wxTOP|wxBOTTOM, 5);

    wxBoxSizer* itemBoxSizer53 = new wxBoxSizer(wxVERTICAL);
    itemBoxSizer39->Add(itemBoxSizer53, 0, wxGROW, 5);
    wxStaticText* itemStaticText54 = new wxStaticText( itemPanel37, wxID_STATIC, _("&Indentation (tenths of a mm)"), wxDefaultPosition, wxDefaultSize, 0 );
    itemBoxSizer53->Add(itemStaticText54, 0, wxALIGN_LEFT|wxLEFT|wxRIGHT|wxTOP|wxADJUST_MINSIZE, 5);

    wxBoxSizer* itemBoxSizer55 = new wxBoxSizer(wxHORIZONTAL);
    itemBoxSizer53->Add(itemBoxSizer55, 0, wxALIGN_LEFT|wxALL, 5);
    itemBoxSizer55->Add(5, 5, 0, wxALIGN_CENTER_VERTICAL, 5);

    wxFlexGridSizer* itemFlexGridSizer57 = new wxFlexGridSizer(2, 2, 0, 0);
    itemBoxSizer55->Add(itemFlexGridSizer57, 0, wxALIGN_CENTER_VERTICAL, 5);
    wxStaticText* itemStaticText58 = new wxStaticText( itemPanel37, wxID_STATIC, _("&Left:"), wxDefaultPosition, wxDefaultSize, 0 );
    itemFlexGridSizer57->Add(itemStaticText58, 0, wxALIGN_RIGHT|wxALIGN_CENTER_VERTICAL|wxALL|wxADJUST_MINSIZE, 5);

    wxBoxSizer* itemBoxSizer59 = new wxBoxSizer(wxHORIZONTAL);
    itemFlexGridSizer57->Add(itemBoxSizer59, 0, wxALIGN_CENTER_HORIZONTAL|wxALIGN_CENTER_VERTICAL, 5);
    m_indentLeft = new wxTextCtrl( itemPanel37, ID_RICHTEXTLISTSTYLEPAGE_INDENTLEFT, _T(""), wxDefaultPosition, wxSize(50, -1), 0 );
    m_indentLeft->SetHelpText(_("The left indent."));
    if (wxRichTextListStylePage::ShowToolTips())
        m_indentLeft->SetToolTip(_("The left indent."));
    itemBoxSizer59->Add(m_indentLeft, 0, wxALIGN_CENTER_VERTICAL|wxALL, 5);

    wxStaticText* itemStaticText61 = new wxStaticText( itemPanel37, wxID_STATIC, _("Left (&first line):"), wxDefaultPosition, wxDefaultSize, 0 );
    itemFlexGridSizer57->Add(itemStaticText61, 0, wxALIGN_RIGHT|wxALIGN_CENTER_VERTICAL|wxALL|wxADJUST_MINSIZE, 5);

    wxBoxSizer* itemBoxSizer62 = new wxBoxSizer(wxHORIZONTAL);
    itemFlexGridSizer57->Add(itemBoxSizer62, 0, wxALIGN_CENTER_HORIZONTAL|wxALIGN_CENTER_VERTICAL, 5);
    m_indentLeftFirst = new wxTextCtrl( itemPanel37, ID_RICHTEXTLISTSTYLEPAGE_INDENTFIRSTLINE, _T(""), wxDefaultPosition, wxSize(50, -1), 0 );
    m_indentLeftFirst->SetHelpText(_("The first line indent."));
    if (wxRichTextListStylePage::ShowToolTips())
        m_indentLeftFirst->SetToolTip(_("The first line indent."));
    itemBoxSizer62->Add(m_indentLeftFirst, 0, wxALIGN_CENTER_VERTICAL|wxALL, 5);

    wxStaticText* itemStaticText64 = new wxStaticText( itemPanel37, wxID_STATIC, _("&Right:"), wxDefaultPosition, wxDefaultSize, 0 );
    itemFlexGridSizer57->Add(itemStaticText64, 0, wxALIGN_RIGHT|wxALIGN_CENTER_VERTICAL|wxALL|wxADJUST_MINSIZE, 5);

    wxBoxSizer* itemBoxSizer65 = new wxBoxSizer(wxHORIZONTAL);
    itemFlexGridSizer57->Add(itemBoxSizer65, 0, wxALIGN_CENTER_HORIZONTAL|wxALIGN_CENTER_VERTICAL, 5);
    m_indentRight = new wxTextCtrl( itemPanel37, ID_RICHTEXTLISTSTYLEPAGE_INDENTRIGHT, _T(""), wxDefaultPosition, wxSize(50, -1), 0 );
    m_indentRight->SetHelpText(_("The right indent."));
    if (wxRichTextListStylePage::ShowToolTips())
        m_indentRight->SetToolTip(_("The right indent."));
    itemBoxSizer65->Add(m_indentRight, 0, wxALIGN_CENTER_VERTICAL|wxALL, 5);

    itemBoxSizer39->Add(2, 1, 1, wxALIGN_CENTER_VERTICAL|wxTOP|wxBOTTOM, 5);

    wxStaticLine* itemStaticLine68 = new wxStaticLine( itemPanel37, wxID_STATIC, wxDefaultPosition, wxDefaultSize, wxLI_VERTICAL );
    itemBoxSizer39->Add(itemStaticLine68, 0, wxGROW|wxTOP|wxBOTTOM, 5);

    itemBoxSizer39->Add(2, 1, 1, wxALIGN_CENTER_VERTICAL|wxTOP|wxBOTTOM, 5);

    wxBoxSizer* itemBoxSizer70 = new wxBoxSizer(wxVERTICAL);
    itemBoxSizer39->Add(itemBoxSizer70, 0, wxGROW, 5);
    wxStaticText* itemStaticText71 = new wxStaticText( itemPanel37, wxID_STATIC, _("&Spacing (tenths of a mm)"), wxDefaultPosition, wxDefaultSize, 0 );
    itemBoxSizer70->Add(itemStaticText71, 0, wxALIGN_LEFT|wxLEFT|wxRIGHT|wxTOP|wxADJUST_MINSIZE, 5);

    wxBoxSizer* itemBoxSizer72 = new wxBoxSizer(wxHORIZONTAL);
    itemBoxSizer70->Add(itemBoxSizer72, 0, wxALIGN_LEFT|wxALL, 5);
    itemBoxSizer72->Add(5, 5, 0, wxALIGN_CENTER_VERTICAL, 5);

    wxFlexGridSizer* itemFlexGridSizer74 = new wxFlexGridSizer(2, 2, 0, 0);
    itemBoxSizer72->Add(itemFlexGridSizer74, 0, wxALIGN_CENTER_VERTICAL, 5);
    wxStaticText* itemStaticText75 = new wxStaticText( itemPanel37, wxID_STATIC, _("Before a paragraph:"), wxDefaultPosition, wxDefaultSize, 0 );
    itemFlexGridSizer74->Add(itemStaticText75, 0, wxALIGN_RIGHT|wxALIGN_CENTER_VERTICAL|wxALL|wxADJUST_MINSIZE, 5);

    wxBoxSizer* itemBoxSizer76 = new wxBoxSizer(wxHORIZONTAL);
    itemFlexGridSizer74->Add(itemBoxSizer76, 0, wxALIGN_CENTER_HORIZONTAL|wxALIGN_CENTER_VERTICAL, 5);
    m_spacingBefore = new wxTextCtrl( itemPanel37, ID_RICHTEXTLISTSTYLEPAGE_SPACINGBEFORE, _T(""), wxDefaultPosition, wxSize(50, -1), 0 );
    m_spacingBefore->SetHelpText(_("The spacing before the paragraph."));
    if (wxRichTextListStylePage::ShowToolTips())
        m_spacingBefore->SetToolTip(_("The spacing before the paragraph."));
    itemBoxSizer76->Add(m_spacingBefore, 0, wxALIGN_CENTER_VERTICAL|wxALL, 5);

    wxStaticText* itemStaticText78 = new wxStaticText( itemPanel37, wxID_STATIC, _("After a paragraph:"), wxDefaultPosition, wxDefaultSize, 0 );
    itemFlexGridSizer74->Add(itemStaticText78, 0, wxALIGN_RIGHT|wxALIGN_CENTER_VERTICAL|wxALL|wxADJUST_MINSIZE, 5);

    wxBoxSizer* itemBoxSizer79 = new wxBoxSizer(wxHORIZONTAL);
    itemFlexGridSizer74->Add(itemBoxSizer79, 0, wxALIGN_CENTER_HORIZONTAL|wxALIGN_CENTER_VERTICAL, 5);
    m_spacingAfter = new wxTextCtrl( itemPanel37, ID_RICHTEXTLISTSTYLEPAGE_SPACINGAFTER, _T(""), wxDefaultPosition, wxSize(50, -1), 0 );
    m_spacingAfter->SetHelpText(_("The spacing after the paragraph."));
    if (wxRichTextListStylePage::ShowToolTips())
        m_spacingAfter->SetToolTip(_("The spacing after the paragraph."));
    itemBoxSizer79->Add(m_spacingAfter, 0, wxALIGN_CENTER_VERTICAL|wxALL, 5);

    wxStaticText* itemStaticText81 = new wxStaticText( itemPanel37, wxID_STATIC, _("Line spacing:"), wxDefaultPosition, wxDefaultSize, 0 );
    itemFlexGridSizer74->Add(itemStaticText81, 0, wxALIGN_RIGHT|wxALIGN_CENTER_VERTICAL|wxALL|wxADJUST_MINSIZE, 5);

    wxBoxSizer* itemBoxSizer82 = new wxBoxSizer(wxHORIZONTAL);
    itemFlexGridSizer74->Add(itemBoxSizer82, 0, wxALIGN_CENTER_HORIZONTAL|wxALIGN_CENTER_VERTICAL, 5);
    wxArrayString m_spacingLineStrings;
    m_spacingLineStrings.Add(_("Single"));
    m_spacingLineStrings.Add(_("1.5"));
    m_spacingLineStrings.Add(_("2"));
    m_spacingLine = new wxComboBox( itemPanel37, ID_RICHTEXTLISTSTYLEPAGE_LINESPACING, _("Single"), wxDefaultPosition, wxDefaultSize, m_spacingLineStrings, wxCB_READONLY );
    m_spacingLine->SetStringSelection(_("Single"));
    m_spacingLine->SetHelpText(_("The line spacing."));
    if (wxRichTextListStylePage::ShowToolTips())
        m_spacingLine->SetToolTip(_("The line spacing."));
    itemBoxSizer82->Add(m_spacingLine, 0, wxALIGN_CENTER_VERTICAL|wxALL, 5);

    itemNotebook9->AddPage(itemPanel37, _("Spacing"));

    itemBoxSizer3->Add(itemNotebook9, 0, wxGROW|wxALL, 5);

    m_previewCtrl = new wxRichTextCtrl( itemPanel1, ID_RICHTEXTLISTSTYLEPAGE_RICHTEXTCTRL, wxEmptyString, wxDefaultPosition, wxSize(350, 100), wxVSCROLL|wxTE_READONLY );
    m_previewCtrl->SetHelpText(_("Shows a preview of the bullet settings."));
    if (wxRichTextListStylePage::ShowToolTips())
        m_previewCtrl->SetToolTip(_("Shows a preview of the bullet settings."));
    itemBoxSizer3->Add(m_previewCtrl, 1, wxGROW|wxLEFT|wxRIGHT|wxBOTTOM, 5);

////@end wxRichTextListStylePage content construction

    m_dontUpdate = true;

    m_styleListBox->Append(_("(None)"));
    m_styleListBox->Append(_("Arabic"));
    m_styleListBox->Append(_("Upper case letters"));
    m_styleListBox->Append(_("Lower case letters"));
    m_styleListBox->Append(_("Upper case roman numerals"));
    m_styleListBox->Append(_("Lower case roman numerals"));
    m_styleListBox->Append(_("Numbered outline"));
    m_styleListBox->Append(_("Symbol"));
    m_styleListBox->Append(_("Bitmap"));
    m_styleListBox->Append(_("Standard"));

    m_symbolCtrl->Append(_("*"));
    m_symbolCtrl->Append(_("-"));
    m_symbolCtrl->Append(_(">"));
    m_symbolCtrl->Append(_("+"));
    m_symbolCtrl->Append(_("~"));

    wxArrayString standardBulletNames;
    if (wxRichTextBuffer::GetRenderer())
        wxRichTextBuffer::GetRenderer()->EnumerateStandardBulletNames(standardBulletNames);

    m_bulletNameCtrl->Append(standardBulletNames);
    wxArrayString facenames = wxRichTextCtrl::GetAvailableFontNames();
    facenames.Sort();

    m_symbolFontCtrl->Append(facenames);

    m_levelCtrl->SetValue(m_currentLevel);

    m_dontUpdate = false;
}

/// Updates the font preview
void wxRichTextListStylePage::UpdatePreview()
{
    static const wxChar* s_para1 = wxT("Lorem ipsum dolor sit amet, consectetuer adipiscing elit. \
Nullam ante sapien, vestibulum nonummy, pulvinar sed, luctus ut, lacus.\n");

    static const wxChar* s_para2 = wxT("Duis pharetra consequat dui. Nullam vitae justo id mauris lobortis interdum.\n");

    static const wxChar* s_para3 = wxT("Integer convallis dolor at augue \
iaculis malesuada. Donec bibendum ipsum ut ante porta fringilla.\n");

    wxRichTextListStyleDefinition* def = wxDynamicCast(wxRichTextFormattingDialog::GetDialogStyleDefinition(this),
        wxRichTextListStyleDefinition);

    wxRichTextStyleSheet* styleSheet = wxRichTextFormattingDialog::GetDialog(this)->GetStyleSheet();

    wxTextAttrEx attr(styleSheet ? def->GetStyle() : def->GetStyleMergedWithBase(styleSheet));

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
    long listStart = m_previewCtrl->GetInsertionPoint() + 1;

    int i;
    for (i = 0; i < 10; i++)
    {
        wxTextAttrEx levelAttr = * def->GetLevelAttributes(i);
        levelAttr.SetBulletNumber(1);
        m_previewCtrl->BeginStyle(levelAttr);
        m_previewCtrl->WriteText(wxString::Format(wxT("List level %d. "), i+1) + s_para2);
        m_previewCtrl->EndStyle();
    }
    m_previewCtrl->EndStyle();
    long listEnd = m_previewCtrl->GetInsertionPoint();

    m_previewCtrl->BeginStyle(normalParaAttr);
    m_previewCtrl->WriteText(s_para3);
    m_previewCtrl->EndStyle();

    m_previewCtrl->NumberList(wxRichTextRange(listStart, listEnd), def);

    m_previewCtrl->Thaw();
}

/// Transfer data from/to window
bool wxRichTextListStylePage::TransferDataFromWindow()
{
    wxPanel::TransferDataFromWindow();

    m_currentLevel = m_levelCtrl->GetValue();

    wxRichTextAttr* attr = GetAttributesForSelection();

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
    if (spacingIndex == 0)
        lineSpacing = 10;
    else if (spacingIndex == 1)
        lineSpacing = 15;
    else if (spacingIndex == 2)
        lineSpacing = 20;

    if (lineSpacing == 0)
        attr->SetFlags(attr->GetFlags() & (~wxTEXT_ATTR_LINE_SPACING));
    else
        attr->SetLineSpacing(lineSpacing);

    /// BULLETS
    // if (m_hasBulletStyle)
    {
        long bulletStyle = 0;

        int index = m_styleListBox->GetSelection();
        if (index == wxRICHTEXT_BULLETINDEX_ARABIC)
            bulletStyle |= wxTEXT_ATTR_BULLET_STYLE_ARABIC;

        else if (index == wxRICHTEXT_BULLETINDEX_UPPER_CASE)
            bulletStyle |= wxTEXT_ATTR_BULLET_STYLE_LETTERS_UPPER;

        else if (index == wxRICHTEXT_BULLETINDEX_LOWER_CASE)
            bulletStyle |= wxTEXT_ATTR_BULLET_STYLE_LETTERS_LOWER;

        else if (index == wxRICHTEXT_BULLETINDEX_UPPER_CASE_ROMAN)
            bulletStyle |= wxTEXT_ATTR_BULLET_STYLE_ROMAN_UPPER;

        else if (index == wxRICHTEXT_BULLETINDEX_LOWER_CASE_ROMAN)
            bulletStyle |= wxTEXT_ATTR_BULLET_STYLE_ROMAN_LOWER;

        else if (index == wxRICHTEXT_BULLETINDEX_OUTLINE)
            bulletStyle |= wxTEXT_ATTR_BULLET_STYLE_OUTLINE;

        else if (index == wxRICHTEXT_BULLETINDEX_SYMBOL)
            bulletStyle |= wxTEXT_ATTR_BULLET_STYLE_SYMBOL;

        else if (index == wxRICHTEXT_BULLETINDEX_BITMAP)
            bulletStyle |= wxTEXT_ATTR_BULLET_STYLE_BITMAP;

        else if (index == wxRICHTEXT_BULLETINDEX_STANDARD)
        {
            bulletStyle |= wxTEXT_ATTR_BULLET_STYLE_STANDARD;
            attr->SetBulletName(m_bulletNameCtrl->GetValue());
        }

        if (m_parenthesesCtrl->GetValue())
            bulletStyle |= wxTEXT_ATTR_BULLET_STYLE_PARENTHESES;
        if (m_rightParenthesisCtrl->GetValue())
            bulletStyle |= wxTEXT_ATTR_BULLET_STYLE_RIGHT_PARENTHESIS;
        if (m_periodCtrl->GetValue())
            bulletStyle |= wxTEXT_ATTR_BULLET_STYLE_PERIOD;

        if (m_bulletAlignmentCtrl->GetSelection() == 1)
            bulletStyle |= wxTEXT_ATTR_BULLET_STYLE_ALIGN_CENTRE;
        else if (m_bulletAlignmentCtrl->GetSelection() == 2)
            bulletStyle |= wxTEXT_ATTR_BULLET_STYLE_ALIGN_RIGHT;
        // Left is implied

        attr->SetBulletStyle(bulletStyle);
    }

    // if (m_hasBulletSymbol)
    {
        attr->SetBulletText(m_symbolCtrl->GetValue());
        attr->SetBulletFont(m_symbolFontCtrl->GetValue());
    }

    return true;
}

bool wxRichTextListStylePage::TransferDataToWindow()
{
    DoTransferDataToWindow();
    UpdatePreview();

    return true;
}

/// Just transfer to the window
void wxRichTextListStylePage::DoTransferDataToWindow()
{
    m_dontUpdate = true;

    wxPanel::TransferDataToWindow();

    wxRichTextAttr* attr = GetAttributesForSelection();

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
        if (lineSpacing == 10)
            index = 0;
        else if (lineSpacing == 15)
            index = 1;
        else if (lineSpacing == 20)
            index = 2;
        else
            index = -1;

        m_spacingLine->SetSelection(index);
    }
    else
        m_spacingLine->SetSelection(-1);

    /// BULLETS
    if (attr->HasBulletStyle())
    {
        int index = 0;
        if (attr->GetBulletStyle() & wxTEXT_ATTR_BULLET_STYLE_ARABIC)
            index = wxRICHTEXT_BULLETINDEX_ARABIC;

        else if (attr->GetBulletStyle() & wxTEXT_ATTR_BULLET_STYLE_LETTERS_UPPER)
            index = wxRICHTEXT_BULLETINDEX_UPPER_CASE;

        else if (attr->GetBulletStyle() & wxTEXT_ATTR_BULLET_STYLE_LETTERS_LOWER)
            index = wxRICHTEXT_BULLETINDEX_LOWER_CASE;

        else if (attr->GetBulletStyle() & wxTEXT_ATTR_BULLET_STYLE_ROMAN_UPPER)
            index = wxRICHTEXT_BULLETINDEX_UPPER_CASE_ROMAN;

        else if (attr->GetBulletStyle() & wxTEXT_ATTR_BULLET_STYLE_ROMAN_LOWER)
            index = wxRICHTEXT_BULLETINDEX_LOWER_CASE_ROMAN;

        else if (attr->GetBulletStyle() & wxTEXT_ATTR_BULLET_STYLE_OUTLINE)
            index = wxRICHTEXT_BULLETINDEX_OUTLINE;

        else if (attr->GetBulletStyle() & wxTEXT_ATTR_BULLET_STYLE_SYMBOL)
            index = wxRICHTEXT_BULLETINDEX_SYMBOL;

        else if (attr->GetBulletStyle() & wxTEXT_ATTR_BULLET_STYLE_BITMAP)
            index = wxRICHTEXT_BULLETINDEX_BITMAP;

        else if (attr->GetBulletStyle() & wxTEXT_ATTR_BULLET_STYLE_STANDARD)
            index = wxRICHTEXT_BULLETINDEX_STANDARD;

        m_styleListBox->SetSelection(index);

        if (attr->GetBulletStyle() & wxTEXT_ATTR_BULLET_STYLE_PARENTHESES)
            m_parenthesesCtrl->SetValue(true);
        else
            m_parenthesesCtrl->SetValue(false);

        if (attr->GetBulletStyle() & wxTEXT_ATTR_BULLET_STYLE_RIGHT_PARENTHESIS)
            m_rightParenthesisCtrl->SetValue(true);
        else
            m_rightParenthesisCtrl->SetValue(false);

        if (attr->GetBulletStyle() & wxTEXT_ATTR_BULLET_STYLE_PERIOD)
            m_periodCtrl->SetValue(true);
        else
            m_periodCtrl->SetValue(false);

        if (attr->GetBulletStyle() & wxTEXT_ATTR_BULLET_STYLE_ALIGN_CENTRE)
            m_bulletAlignmentCtrl->SetSelection(1);
        else if (attr->GetBulletStyle() & wxTEXT_ATTR_BULLET_STYLE_ALIGN_RIGHT)
            m_bulletAlignmentCtrl->SetSelection(2);
        else
            m_bulletAlignmentCtrl->SetSelection(0);
    }
    else
    {
        m_styleListBox->SetSelection(-1);
        m_bulletAlignmentCtrl->SetSelection(-1);
    }

    if (attr->HasBulletText())
    {
        m_symbolCtrl->SetValue(attr->GetBulletText());
        m_symbolFontCtrl->SetValue(attr->GetBulletFont());
    }
    else
        m_symbolCtrl->SetValue(wxEmptyString);

    if (attr->HasBulletName())
        m_bulletNameCtrl->SetValue(attr->GetBulletName());
    else
        m_bulletNameCtrl->SetValue(wxEmptyString);

    m_dontUpdate = false;
}

/// Get attributes for selected level
wxRichTextAttr* wxRichTextListStylePage::GetAttributesForSelection()
{
    wxRichTextListStyleDefinition* def = wxDynamicCast(wxRichTextFormattingDialog::GetDialogStyleDefinition(this),
        wxRichTextListStyleDefinition);

    int value = m_levelCtrl->GetValue();

    if (def)
        return def->GetLevelAttributes(value-1);
    else
        return NULL;
}

/// Just transfer from the window and update the preview
void wxRichTextListStylePage::TransferAndPreview()
{
    if (!m_dontUpdate)
    {
        TransferDataFromWindow();
        UpdatePreview();
    }
}


/*!
 * wxEVT_COMMAND_SPINCTRL_UPDATED event handler for ID_RICHTEXTLISTSTYLEPAGE_LEVEL
 */

void wxRichTextListStylePage::OnLevelUpdated( wxSpinEvent& WXUNUSED(event) )
{
    if (!m_dontUpdate)
    {
        m_currentLevel = m_levelCtrl->GetValue();
        TransferDataToWindow();
    }
}

/*!
 * wxEVT_SCROLL_LINEUP event handler for ID_RICHTEXTLISTSTYLEPAGE_LEVEL
 */

void wxRichTextListStylePage::OnLevelUp( wxSpinEvent& event )
{
    if (!m_dontUpdate)
    {
        m_currentLevel = event.GetPosition();
        TransferDataToWindow();
    }
}

/*!
 * wxEVT_SCROLL_LINEDOWN event handler for ID_RICHTEXTLISTSTYLEPAGE_LEVEL
 */

void wxRichTextListStylePage::OnLevelDown( wxSpinEvent& event )
{
    if (!m_dontUpdate)
    {
        m_currentLevel = event.GetPosition();
        TransferDataToWindow();
    }
}

/*!
 * wxEVT_COMMAND_TEXT_UPDATED event handler for ID_RICHTEXTLISTSTYLEPAGE_LEVEL
 */

void wxRichTextListStylePage::OnLevelTextUpdated( wxCommandEvent& WXUNUSED(event) )
{
    // Can cause problems
#if 0
    if (!m_dontUpdate)
    {
        m_currentLevel = event.GetInt();
        TransferDataToWindow();
    }
#endif
}

/*!
 * wxEVT_UPDATE_UI event handler for ID_RICHTEXTLISTSTYLEPAGE_LEVEL
 */

void wxRichTextListStylePage::OnLevelUIUpdate( wxUpdateUIEvent& event )
{
////@begin wxEVT_UPDATE_UI event handler for ID_RICHTEXTLISTSTYLEPAGE_LEVEL in wxRichTextListStylePage.
    // Before editing this code, remove the block markers.
    event.Skip();
////@end wxEVT_UPDATE_UI event handler for ID_RICHTEXTLISTSTYLEPAGE_LEVEL in wxRichTextListStylePage.
}

/*!
 * wxEVT_COMMAND_LISTBOX_SELECTED event handler for ID_RICHTEXTLISTSTYLEPAGE_STYLELISTBOX
 */

void wxRichTextListStylePage::OnStylelistboxSelected( wxCommandEvent& WXUNUSED(event) )
{
    TransferAndPreview();
}

/*!
 * wxEVT_UPDATE_UI event handler for ID_RICHTEXTLISTSTYLEPAGE_SYMBOLSTATIC
 */

void wxRichTextListStylePage::OnSymbolstaticUpdate( wxUpdateUIEvent& event )
{
    OnSymbolUpdate(event);
}

/*!
 * wxEVT_COMMAND_COMBOBOX_SELECTED event handler for ID_RICHTEXTBULLETSPAGE_SYMBOLCTRL
 */

void wxRichTextListStylePage::OnSymbolctrlSelected( wxCommandEvent& WXUNUSED(event) )
{
    TransferAndPreview();
}

/*!
 * wxEVT_COMMAND_TEXT_UPDATED event handler for ID_RICHTEXTBULLETSPAGE_SYMBOLCTRL
 */

void wxRichTextListStylePage::OnSymbolctrlUpdated( wxCommandEvent& WXUNUSED(event) )
{
    TransferAndPreview();
}

/*!
 * wxEVT_UPDATE_UI event handler for ID_RICHTEXTBULLETSPAGE_SYMBOLCTRL
 */

void wxRichTextListStylePage::OnSymbolctrlUIUpdate( wxUpdateUIEvent& event )
{
    OnSymbolUpdate(event);
}

/*!
 * wxEVT_COMMAND_BUTTON_CLICKED event handler for ID_RICHTEXTBULLETSPAGE_CHOOSE_SYMBOL
 */

void wxRichTextListStylePage::OnChooseSymbolClick( wxCommandEvent& WXUNUSED(event) )
{
    int sel = m_styleListBox->GetSelection();
    if (sel == wxRICHTEXT_BULLETINDEX_SYMBOL)
    {
        wxString symbol = m_symbolCtrl->GetValue();
        wxString fontName = m_symbolFontCtrl->GetValue();
        wxSymbolPickerDialog dlg(symbol, fontName, fontName, this);

        if (dlg.ShowModal() == wxID_OK)
        {
            m_dontUpdate = true;

            m_symbolCtrl->SetValue(dlg.GetSymbol());
            m_symbolFontCtrl->SetValue(dlg.GetFontName());

            TransferAndPreview();

            m_dontUpdate = false;
        }
    }
}

/*!
 * wxEVT_UPDATE_UI event handler for ID_RICHTEXTBULLETSPAGE_CHOOSE_SYMBOL
 */

void wxRichTextListStylePage::OnChooseSymbolUpdate( wxUpdateUIEvent& event )
{
    OnSymbolUpdate(event);
}

/*!
 * wxEVT_COMMAND_COMBOBOX_SELECTED event handler for ID_RICHTEXTLISTSTYLEPAGE_SYMBOLFONTCTRL
 */

void wxRichTextListStylePage::OnSymbolfontctrlSelected( wxCommandEvent& WXUNUSED(event) )
{
    TransferAndPreview();
}

/*!
 * wxEVT_COMMAND_TEXT_UPDATED event handler for ID_RICHTEXTLISTSTYLEPAGE_SYMBOLFONTCTRL
 */

void wxRichTextListStylePage::OnSymbolfontctrlUpdated( wxCommandEvent& WXUNUSED(event) )
{
    TransferAndPreview();
}

/*!
 * wxEVT_UPDATE_UI event handler for ID_RICHTEXTLISTSTYLEPAGE_SYMBOLFONTCTRL
 */

void wxRichTextListStylePage::OnSymbolfontctrlUIUpdate( wxUpdateUIEvent& event )
{
    OnSymbolUpdate(event);
}

/*!
 * wxEVT_COMMAND_CHECKBOX_CLICKED event handler for ID_RICHTEXTLISTSTYLEPAGE__PARENTHESESCTRL
 */

void wxRichTextListStylePage::OnParenthesesctrlClick( wxCommandEvent& WXUNUSED(event) )
{
    TransferAndPreview();
}

/*!
 * wxEVT_UPDATE_UI event handler for ID_RICHTEXTLISTSTYLEPAGE__PARENTHESESCTRL
 */

void wxRichTextListStylePage::OnParenthesesctrlUpdate( wxUpdateUIEvent& event )
{
    int sel = m_styleListBox->GetSelection();
    event.Enable((sel != wxRICHTEXT_BULLETINDEX_SYMBOL &&
                  sel != wxRICHTEXT_BULLETINDEX_BITMAP &&
                  sel != wxRICHTEXT_BULLETINDEX_NONE));
}

/*!
 * wxEVT_COMMAND_CHECKBOX_CLICKED event handler for ID_RICHTEXTLISTSTYLEPAGE_PERIODCTRL
 */

void wxRichTextListStylePage::OnPeriodctrlClick( wxCommandEvent& WXUNUSED(event) )
{
    TransferAndPreview();
}

/*!
 * wxEVT_UPDATE_UI event handler for ID_RICHTEXTLISTSTYLEPAGE_PERIODCTRL
 */

void wxRichTextListStylePage::OnPeriodctrlUpdate( wxUpdateUIEvent& event )
{
    int sel = m_styleListBox->GetSelection();
    event.Enable((sel != wxRICHTEXT_BULLETINDEX_SYMBOL &&
                  sel != wxRICHTEXT_BULLETINDEX_BITMAP &&
                  sel != wxRICHTEXT_BULLETINDEX_NONE));
}

/*!
 * wxEVT_COMMAND_RADIOBUTTON_SELECTED event handler for ID_RICHTEXTLISTSTYLEPAGE_ALIGNLEFT
 */

void wxRichTextListStylePage::OnRichtextliststylepageAlignleftSelected( wxCommandEvent& WXUNUSED(event) )
{
    TransferAndPreview();
}

/*!
 * wxEVT_COMMAND_RADIOBUTTON_SELECTED event handler for ID_RICHTEXTLISTSTYLEPAGE_ALIGNRIGHT
 */

void wxRichTextListStylePage::OnRichtextliststylepageAlignrightSelected( wxCommandEvent& WXUNUSED(event) )
{
    TransferAndPreview();
}

/*!
 * wxEVT_COMMAND_RADIOBUTTON_SELECTED event handler for ID_RICHTEXTLISTSTYLEPAGE_JUSTIFIED
 */

void wxRichTextListStylePage::OnRichtextliststylepageJustifiedSelected( wxCommandEvent& WXUNUSED(event) )
{
    TransferAndPreview();
}

/*!
 * wxEVT_COMMAND_RADIOBUTTON_SELECTED event handler for ID_RICHTEXTLISTSTYLEPAGE_CENTERED
 */

void wxRichTextListStylePage::OnRichtextliststylepageCenteredSelected( wxCommandEvent& WXUNUSED(event) )
{
    TransferAndPreview();
}

/*!
 * wxEVT_COMMAND_RADIOBUTTON_SELECTED event handler for ID_RICHTEXTLISTSTYLEPAGE_ALIGNINDETERMINATE
 */

void wxRichTextListStylePage::OnRichtextliststylepageAlignindeterminateSelected( wxCommandEvent& WXUNUSED(event) )
{
    TransferAndPreview();
}

/*!
 * wxEVT_COMMAND_TEXT_UPDATED event handler for ID_RICHTEXTLISTSTYLEPAGE_INDENTLEFT
 */

void wxRichTextListStylePage::OnIndentLeftUpdated( wxCommandEvent& WXUNUSED(event) )
{
    TransferAndPreview();
}

/*!
 * wxEVT_COMMAND_TEXT_UPDATED event handler for ID_RICHTEXTLISTSTYLEPAGE_INDENTFIRSTLINE
 */

void wxRichTextListStylePage::OnIndentFirstLineUpdated( wxCommandEvent& WXUNUSED(event) )
{
    TransferAndPreview();
}

/*!
 * wxEVT_COMMAND_TEXT_UPDATED event handler for ID_RICHTEXTLISTSTYLEPAGE_INDENTRIGHT
 */

void wxRichTextListStylePage::OnIndentRightUpdated( wxCommandEvent& WXUNUSED(event) )
{
    TransferAndPreview();
}

/*!
 * wxEVT_COMMAND_TEXT_UPDATED event handler for ID_RICHTEXTLISTSTYLEPAGE_SPACINGBEFORE
 */

void wxRichTextListStylePage::OnSpacingBeforeUpdated( wxCommandEvent& WXUNUSED(event) )
{
    TransferAndPreview();
}

/*!
 * wxEVT_COMMAND_TEXT_UPDATED event handler for ID_RICHTEXTLISTSTYLEPAGE_SPACINGAFTER
 */

void wxRichTextListStylePage::OnSpacingAfterUpdated( wxCommandEvent& WXUNUSED(event) )
{
    TransferAndPreview();
}

/*!
 * wxEVT_COMMAND_COMBOBOX_SELECTED event handler for ID_RICHTEXTLISTSTYLEPAGE_LINESPACING
 */

void wxRichTextListStylePage::OnLineSpacingSelected( wxCommandEvent& WXUNUSED(event) )
{
    TransferAndPreview();
}

/*!
 * Should we show tooltips?
 */

bool wxRichTextListStylePage::ShowToolTips()
{
    return wxRichTextFormattingDialog::ShowToolTips();
}

/*!
 * Get bitmap resources
 */

wxBitmap wxRichTextListStylePage::GetBitmapResource( const wxString& name )
{
    // Bitmap retrieval
////@begin wxRichTextListStylePage bitmap retrieval
    wxUnusedVar(name);
    return wxNullBitmap;
////@end wxRichTextListStylePage bitmap retrieval
}

/*!
 * Get icon resources
 */

wxIcon wxRichTextListStylePage::GetIconResource( const wxString& name )
{
    // Icon retrieval
////@begin wxRichTextListStylePage icon retrieval
    wxUnusedVar(name);
    return wxNullIcon;
////@end wxRichTextListStylePage icon retrieval
}

/// Update for symbol-related controls
void wxRichTextListStylePage::OnSymbolUpdate( wxUpdateUIEvent& event )
{
    int sel = m_styleListBox->GetSelection();
    event.Enable(sel == wxRICHTEXT_BULLETINDEX_SYMBOL);
}

/// Update for number-related controls
void wxRichTextListStylePage::OnNumberUpdate( wxUpdateUIEvent& event )
{
    int sel = m_styleListBox->GetSelection();
    event.Enable((sel != wxRICHTEXT_BULLETINDEX_SYMBOL &&
                  sel != wxRICHTEXT_BULLETINDEX_BITMAP &&
                  sel != wxRICHTEXT_BULLETINDEX_STANDARD &&
                  sel != wxRICHTEXT_BULLETINDEX_NONE));
}

/// Update for standard bullet-related controls
void wxRichTextListStylePage::OnStandardBulletUpdate( wxUpdateUIEvent& event )
{
    int sel = m_styleListBox->GetSelection();
    event.Enable( sel == wxRICHTEXT_BULLETINDEX_STANDARD );
}

/*!
 * wxEVT_COMMAND_BUTTON_CLICKED event handler for ID_RICHTEXTLISTSTYLEPAGE_CHOOSE_FONT
 */

void wxRichTextListStylePage::OnChooseFontClick( wxCommandEvent& WXUNUSED(event) )
{
    wxRichTextAttr* attr = GetAttributesForSelection();

    int pages = wxRICHTEXT_FORMAT_FONT;
    wxRichTextFormattingDialog formatDlg;
    formatDlg.SetStyle(*attr, false);
    formatDlg.Create(pages, this);

    if (formatDlg.ShowModal() == wxID_OK)
    {
        (*attr) = formatDlg.GetAttributes();
        TransferAndPreview();
    }
}
/*!
 * wxEVT_UPDATE_UI event handler for ID_RICHTEXTLISTSTYLEPAGE_NAMESTATIC
 */

void wxRichTextListStylePage::OnNamestaticUpdate( wxUpdateUIEvent& event )
{
    OnStandardBulletUpdate(event);
}

/*!
 * wxEVT_COMMAND_COMBOBOX_SELECTED event handler for ID_RICHTEXTLISTSTYLEPAGE_NAMECTRL
 */

void wxRichTextListStylePage::OnNamectrlSelected( wxCommandEvent& WXUNUSED(event) )
{
    TransferAndPreview();
}

/*!
 * wxEVT_COMMAND_TEXT_UPDATED event handler for ID_RICHTEXTLISTSTYLEPAGE_NAMECTRL
 */

void wxRichTextListStylePage::OnNamectrlUpdated( wxCommandEvent& WXUNUSED(event) )
{
    TransferAndPreview();
}

/*!
 * wxEVT_UPDATE_UI event handler for ID_RICHTEXTLISTSTYLEPAGE_NAMECTRL
 */

void wxRichTextListStylePage::OnNamectrlUIUpdate( wxUpdateUIEvent& event )
{
    OnStandardBulletUpdate(event);
}
/*!
 * wxEVT_COMMAND_CHECKBOX_CLICKED event handler for ID_RICHTEXTLISTSTYLEPAGE_RIGHTPARENTHESISCTRL
 */

void wxRichTextListStylePage::OnRightParenthesisCtrlClick( wxCommandEvent& WXUNUSED(event) )
{
    TransferAndPreview();
}

/*!
 * wxEVT_UPDATE_UI event handler for ID_RICHTEXTLISTSTYLEPAGE_RIGHTPARENTHESISCTRL
 */

void wxRichTextListStylePage::OnRightParenthesisCtrlUpdate( wxUpdateUIEvent& event )
{
    int sel = m_styleListBox->GetSelection();
    event.Enable((sel != wxRICHTEXT_BULLETINDEX_SYMBOL &&
                  sel != wxRICHTEXT_BULLETINDEX_BITMAP &&
                  sel != wxRICHTEXT_BULLETINDEX_NONE));
}

/*!
 * wxEVT_COMMAND_COMBOBOX_SELECTED event handler for ID_RICHTEXTLISTSTYLEPAGE_BULLETALIGNMENTCTRL
 */

void wxRichTextListStylePage::OnBulletAlignmentCtrlSelected( wxCommandEvent& WXUNUSED(event) )
{
    TransferAndPreview();
}


