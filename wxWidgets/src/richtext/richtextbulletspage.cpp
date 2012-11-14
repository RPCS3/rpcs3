/////////////////////////////////////////////////////////////////////////////
// Name:        src/richtext/richtextbulletspage.cpp
// Purpose:
// Author:      Julian Smart
// Modified by:
// Created:     10/4/2006 10:32:31 AM
// RCS-ID:      $Id: richtextbulletspage.cpp 66266 2010-11-26 16:31:44Z JS $
// Copyright:   (c) Julian Smart
// Licence:     wxWindows licence
/////////////////////////////////////////////////////////////////////////////

#if wxUSE_RICHTEXT

#include "wx/spinctrl.h"
#include "wx/richtext/richtextbulletspage.h"
#include "wx/richtext/richtextsymboldlg.h"
#include "wx/utils.h"

/*!
 * wxRichTextBulletsPage type definition
 */

IMPLEMENT_DYNAMIC_CLASS( wxRichTextBulletsPage, wxPanel )

/*!
 * wxRichTextBulletsPage event table definition
 */

BEGIN_EVENT_TABLE( wxRichTextBulletsPage, wxPanel )

////@begin wxRichTextBulletsPage event table entries
    EVT_LISTBOX( ID_RICHTEXTBULLETSPAGE_STYLELISTBOX, wxRichTextBulletsPage::OnStylelistboxSelected )

    EVT_CHECKBOX( ID_RICHTEXTBULLETSPAGE_PERIODCTRL, wxRichTextBulletsPage::OnPeriodctrlClick )
    EVT_UPDATE_UI( ID_RICHTEXTBULLETSPAGE_PERIODCTRL, wxRichTextBulletsPage::OnPeriodctrlUpdate )

    EVT_CHECKBOX( ID_RICHTEXTBULLETSPAGE_PARENTHESESCTRL, wxRichTextBulletsPage::OnParenthesesctrlClick )
    EVT_UPDATE_UI( ID_RICHTEXTBULLETSPAGE_PARENTHESESCTRL, wxRichTextBulletsPage::OnParenthesesctrlUpdate )

    EVT_CHECKBOX( ID_RICHTEXTBULLETSPAGE_RIGHTPARENTHESISCTRL, wxRichTextBulletsPage::OnRightParenthesisCtrlClick )
    EVT_UPDATE_UI( ID_RICHTEXTBULLETSPAGE_RIGHTPARENTHESISCTRL, wxRichTextBulletsPage::OnRightParenthesisCtrlUpdate )

    EVT_COMBOBOX( ID_RICHTEXTBULLETSPAGE_BULLETALIGNMENTCTRL, wxRichTextBulletsPage::OnBulletAlignmentCtrlSelected )

    EVT_UPDATE_UI( ID_RICHTEXTBULLETSPAGE_SYMBOLSTATIC, wxRichTextBulletsPage::OnSymbolstaticUpdate )

    EVT_COMBOBOX( ID_RICHTEXTBULLETSPAGE_SYMBOLCTRL, wxRichTextBulletsPage::OnSymbolctrlSelected )
    EVT_TEXT( ID_RICHTEXTBULLETSPAGE_SYMBOLCTRL, wxRichTextBulletsPage::OnSymbolctrlUpdated )
    EVT_UPDATE_UI( ID_RICHTEXTBULLETSPAGE_SYMBOLCTRL, wxRichTextBulletsPage::OnSymbolctrlUpdate )

    EVT_BUTTON( ID_RICHTEXTBULLETSPAGE_CHOOSE_SYMBOL, wxRichTextBulletsPage::OnChooseSymbolClick )
    EVT_UPDATE_UI( ID_RICHTEXTBULLETSPAGE_CHOOSE_SYMBOL, wxRichTextBulletsPage::OnChooseSymbolUpdate )

    EVT_COMBOBOX( ID_RICHTEXTBULLETSPAGE_SYMBOLFONTCTRL, wxRichTextBulletsPage::OnSymbolfontctrlSelected )
    EVT_TEXT( ID_RICHTEXTBULLETSPAGE_SYMBOLFONTCTRL, wxRichTextBulletsPage::OnSymbolfontctrlUpdated )
    EVT_UPDATE_UI( ID_RICHTEXTBULLETSPAGE_SYMBOLFONTCTRL, wxRichTextBulletsPage::OnSymbolfontctrlUIUpdate )

    EVT_UPDATE_UI( ID_RICHTEXTBULLETSPAGE_NAMESTATIC, wxRichTextBulletsPage::OnNamestaticUpdate )

    EVT_COMBOBOX( ID_RICHTEXTBULLETSPAGE_NAMECTRL, wxRichTextBulletsPage::OnNamectrlSelected )
    EVT_TEXT( ID_RICHTEXTBULLETSPAGE_NAMECTRL, wxRichTextBulletsPage::OnNamectrlUpdated )
    EVT_UPDATE_UI( ID_RICHTEXTBULLETSPAGE_NAMECTRL, wxRichTextBulletsPage::OnNamectrlUIUpdate )

    EVT_UPDATE_UI( ID_RICHTEXTBULLETSPAGE_NUMBERSTATIC, wxRichTextBulletsPage::OnNumberstaticUpdate )

    EVT_SPINCTRL( ID_RICHTEXTBULLETSPAGE_NUMBERCTRL, wxRichTextBulletsPage::OnNumberctrlUpdated )
    EVT_SPIN_UP( ID_RICHTEXTBULLETSPAGE_NUMBERCTRL, wxRichTextBulletsPage::OnNumberctrlUp )
    EVT_SPIN_DOWN( ID_RICHTEXTBULLETSPAGE_NUMBERCTRL, wxRichTextBulletsPage::OnNumberctrlDown )
    EVT_TEXT( ID_RICHTEXTBULLETSPAGE_NUMBERCTRL, wxRichTextBulletsPage::OnNumberctrlTextUpdated )
    EVT_UPDATE_UI( ID_RICHTEXTBULLETSPAGE_NUMBERCTRL, wxRichTextBulletsPage::OnNumberctrlUpdate )

////@end wxRichTextBulletsPage event table entries

END_EVENT_TABLE()

/*!
 * wxRichTextBulletsPage constructors
 */

wxRichTextBulletsPage::wxRichTextBulletsPage( )
{
    Init();
}

wxRichTextBulletsPage::wxRichTextBulletsPage( wxWindow* parent, wxWindowID id, const wxPoint& pos, const wxSize& size, long style )
{
    Init();
    Create(parent, id, pos, size, style);
}

/*!
 * Initialise members
 */

void wxRichTextBulletsPage::Init()
{
    m_hasBulletStyle = false;
    m_hasBulletNumber = false;
    m_hasBulletSymbol = false;
    m_dontUpdate = false;

////@begin wxRichTextBulletsPage member initialisation
    m_styleListBox = NULL;
    m_periodCtrl = NULL;
    m_parenthesesCtrl = NULL;
    m_rightParenthesisCtrl = NULL;
    m_bulletAlignmentCtrl = NULL;
    m_symbolCtrl = NULL;
    m_symbolFontCtrl = NULL;
    m_bulletNameCtrl = NULL;
    m_numberCtrl = NULL;
    m_previewCtrl = NULL;
////@end wxRichTextBulletsPage member initialisation
}

/*!
 * wxRichTextBulletsPage creator
 */

bool wxRichTextBulletsPage::Create( wxWindow* parent, wxWindowID id, const wxPoint& pos, const wxSize& size, long style )
{
////@begin wxRichTextBulletsPage creation
    wxPanel::Create( parent, id, pos, size, style );

    CreateControls();
    if (GetSizer())
    {
        GetSizer()->SetSizeHints(this);
    }
    Centre();
////@end wxRichTextBulletsPage creation
    return true;
}

/*!
 * Control creation for wxRichTextBulletsPage
 */

void wxRichTextBulletsPage::CreateControls()
{
////@begin wxRichTextBulletsPage content construction
    wxRichTextBulletsPage* itemPanel1 = this;

    wxBoxSizer* itemBoxSizer2 = new wxBoxSizer(wxVERTICAL);
    itemPanel1->SetSizer(itemBoxSizer2);

    wxBoxSizer* itemBoxSizer3 = new wxBoxSizer(wxVERTICAL);
    itemBoxSizer2->Add(itemBoxSizer3, 1, wxGROW|wxALL, 5);

    wxBoxSizer* itemBoxSizer4 = new wxBoxSizer(wxHORIZONTAL);
    itemBoxSizer3->Add(itemBoxSizer4, 0, wxGROW, 5);

    wxBoxSizer* itemBoxSizer5 = new wxBoxSizer(wxVERTICAL);
    itemBoxSizer4->Add(itemBoxSizer5, 0, wxGROW, 5);

    wxStaticText* itemStaticText6 = new wxStaticText( itemPanel1, wxID_STATIC, _("&Bullet style:"), wxDefaultPosition, wxDefaultSize, 0 );
    itemBoxSizer5->Add(itemStaticText6, 0, wxALIGN_LEFT|wxLEFT|wxRIGHT|wxTOP|wxADJUST_MINSIZE, 5);

    wxArrayString m_styleListBoxStrings;
    m_styleListBox = new wxListBox( itemPanel1, ID_RICHTEXTBULLETSPAGE_STYLELISTBOX, wxDefaultPosition, wxSize(-1, 90), m_styleListBoxStrings, wxLB_SINGLE );
    m_styleListBox->SetHelpText(_("The available bullet styles."));
    if (wxRichTextBulletsPage::ShowToolTips())
        m_styleListBox->SetToolTip(_("The available bullet styles."));
    itemBoxSizer5->Add(m_styleListBox, 1, wxGROW|wxALL, 5);

    wxBoxSizer* itemBoxSizer8 = new wxBoxSizer(wxHORIZONTAL);
    itemBoxSizer5->Add(itemBoxSizer8, 0, wxGROW, 5);

    m_periodCtrl = new wxCheckBox( itemPanel1, ID_RICHTEXTBULLETSPAGE_PERIODCTRL, _("Peri&od"), wxDefaultPosition, wxDefaultSize, 0 );
    m_periodCtrl->SetValue(false);
    m_periodCtrl->SetHelpText(_("Check to add a period after the bullet."));
    if (wxRichTextBulletsPage::ShowToolTips())
        m_periodCtrl->SetToolTip(_("Check to add a period after the bullet."));
    itemBoxSizer8->Add(m_periodCtrl, 0, wxALIGN_CENTER_VERTICAL|wxALL, 5);

    m_parenthesesCtrl = new wxCheckBox( itemPanel1, ID_RICHTEXTBULLETSPAGE_PARENTHESESCTRL, _("(*)"), wxDefaultPosition, wxDefaultSize, 0 );
    m_parenthesesCtrl->SetValue(false);
    m_parenthesesCtrl->SetHelpText(_("Check to enclose the bullet in parentheses."));
    if (wxRichTextBulletsPage::ShowToolTips())
        m_parenthesesCtrl->SetToolTip(_("Check to enclose the bullet in parentheses."));
    itemBoxSizer8->Add(m_parenthesesCtrl, 0, wxALIGN_CENTER_VERTICAL|wxALL, 5);

    m_rightParenthesisCtrl = new wxCheckBox( itemPanel1, ID_RICHTEXTBULLETSPAGE_RIGHTPARENTHESISCTRL, _("*)"), wxDefaultPosition, wxDefaultSize, 0 );
    m_rightParenthesisCtrl->SetValue(false);
    m_rightParenthesisCtrl->SetHelpText(_("Check to add a right parenthesis."));
    if (wxRichTextBulletsPage::ShowToolTips())
        m_rightParenthesisCtrl->SetToolTip(_("Check to add a right parenthesis."));
    itemBoxSizer8->Add(m_rightParenthesisCtrl, 0, wxALIGN_CENTER_VERTICAL|wxALL, 5);

    itemBoxSizer5->Add(5, 5, 0, wxALIGN_CENTER_HORIZONTAL, 5);

    wxStaticText* itemStaticText13 = new wxStaticText( itemPanel1, wxID_STATIC, _("Bullet &Alignment:"), wxDefaultPosition, wxDefaultSize, 0 );
    itemBoxSizer5->Add(itemStaticText13, 0, wxALIGN_LEFT|wxLEFT|wxRIGHT|wxTOP|wxADJUST_MINSIZE, 5);

    wxArrayString m_bulletAlignmentCtrlStrings;
    m_bulletAlignmentCtrlStrings.Add(_("Left"));
    m_bulletAlignmentCtrlStrings.Add(_("Centre"));
    m_bulletAlignmentCtrlStrings.Add(_("Right"));
    m_bulletAlignmentCtrl = new wxComboBox( itemPanel1, ID_RICHTEXTBULLETSPAGE_BULLETALIGNMENTCTRL, _("Left"), wxDefaultPosition, wxSize(60, -1), m_bulletAlignmentCtrlStrings, wxCB_READONLY );
    m_bulletAlignmentCtrl->SetStringSelection(_("Left"));
    m_bulletAlignmentCtrl->SetHelpText(_("The bullet character."));
    if (wxRichTextBulletsPage::ShowToolTips())
        m_bulletAlignmentCtrl->SetToolTip(_("The bullet character."));
    itemBoxSizer5->Add(m_bulletAlignmentCtrl, 0, wxGROW|wxALL|wxFIXED_MINSIZE, 5);

    itemBoxSizer4->Add(2, 1, 1, wxALIGN_CENTER_VERTICAL|wxTOP|wxBOTTOM, 5);

    wxStaticLine* itemStaticLine16 = new wxStaticLine( itemPanel1, wxID_STATIC, wxDefaultPosition, wxDefaultSize, wxLI_VERTICAL );
    itemBoxSizer4->Add(itemStaticLine16, 0, wxGROW|wxLEFT|wxRIGHT, 5);

    itemBoxSizer4->Add(2, 1, 1, wxALIGN_CENTER_VERTICAL|wxTOP|wxBOTTOM, 5);

    wxBoxSizer* itemBoxSizer18 = new wxBoxSizer(wxVERTICAL);
    itemBoxSizer4->Add(itemBoxSizer18, 0, wxGROW, 5);

    wxBoxSizer* itemBoxSizer19 = new wxBoxSizer(wxHORIZONTAL);
    itemBoxSizer18->Add(itemBoxSizer19, 0, wxGROW, 5);

    wxStaticText* itemStaticText20 = new wxStaticText( itemPanel1, ID_RICHTEXTBULLETSPAGE_SYMBOLSTATIC, _("&Symbol:"), wxDefaultPosition, wxDefaultSize, 0 );
    itemBoxSizer19->Add(itemStaticText20, 0, wxALIGN_CENTER_VERTICAL|wxALL|wxADJUST_MINSIZE, 5);

    wxArrayString m_symbolCtrlStrings;
    m_symbolCtrl = new wxComboBox( itemPanel1, ID_RICHTEXTBULLETSPAGE_SYMBOLCTRL, _T(""), wxDefaultPosition, wxSize(60, -1), m_symbolCtrlStrings, wxCB_DROPDOWN );
    m_symbolCtrl->SetHelpText(_("The bullet character."));
    if (wxRichTextBulletsPage::ShowToolTips())
        m_symbolCtrl->SetToolTip(_("The bullet character."));
    itemBoxSizer19->Add(m_symbolCtrl, 0, wxALIGN_CENTER_VERTICAL|wxALL|wxFIXED_MINSIZE, 5);

    wxButton* itemButton22 = new wxButton( itemPanel1, ID_RICHTEXTBULLETSPAGE_CHOOSE_SYMBOL, _("Ch&oose..."), wxDefaultPosition, wxDefaultSize, 0 );
    itemButton22->SetHelpText(_("Click to browse for a symbol."));
    if (wxRichTextBulletsPage::ShowToolTips())
        itemButton22->SetToolTip(_("Click to browse for a symbol."));
    itemBoxSizer19->Add(itemButton22, 0, wxALIGN_CENTER_VERTICAL|wxALL, 5);

    itemBoxSizer18->Add(5, 5, 0, wxALIGN_CENTER_HORIZONTAL, 5);

    wxStaticText* itemStaticText24 = new wxStaticText( itemPanel1, ID_RICHTEXTBULLETSPAGE_SYMBOLSTATIC, _("Symbol &font:"), wxDefaultPosition, wxDefaultSize, 0 );
    itemBoxSizer18->Add(itemStaticText24, 0, wxALIGN_LEFT|wxLEFT|wxRIGHT|wxTOP|wxADJUST_MINSIZE, 5);

    wxArrayString m_symbolFontCtrlStrings;
    m_symbolFontCtrl = new wxComboBox( itemPanel1, ID_RICHTEXTBULLETSPAGE_SYMBOLFONTCTRL, _T(""), wxDefaultPosition, wxDefaultSize, m_symbolFontCtrlStrings, wxCB_DROPDOWN );
    m_symbolFontCtrl->SetHelpText(_("Available fonts."));
    if (wxRichTextBulletsPage::ShowToolTips())
        m_symbolFontCtrl->SetToolTip(_("Available fonts."));
    itemBoxSizer18->Add(m_symbolFontCtrl, 0, wxGROW|wxALL, 5);

    itemBoxSizer18->Add(5, 5, 1, wxALIGN_CENTER_HORIZONTAL, 5);

    wxStaticText* itemStaticText27 = new wxStaticText( itemPanel1, ID_RICHTEXTBULLETSPAGE_NAMESTATIC, _("S&tandard bullet name:"), wxDefaultPosition, wxDefaultSize, 0 );
    itemBoxSizer18->Add(itemStaticText27, 0, wxALIGN_LEFT|wxLEFT|wxRIGHT|wxTOP|wxADJUST_MINSIZE, 5);

    wxArrayString m_bulletNameCtrlStrings;
    m_bulletNameCtrl = new wxComboBox( itemPanel1, ID_RICHTEXTBULLETSPAGE_NAMECTRL, _T(""), wxDefaultPosition, wxDefaultSize, m_bulletNameCtrlStrings, wxCB_DROPDOWN );
    m_bulletNameCtrl->SetHelpText(_("A standard bullet name."));
    if (wxRichTextBulletsPage::ShowToolTips())
        m_bulletNameCtrl->SetToolTip(_("A standard bullet name."));
    itemBoxSizer18->Add(m_bulletNameCtrl, 0, wxGROW|wxALL, 5);

    itemBoxSizer18->Add(5, 5, 1, wxALIGN_CENTER_HORIZONTAL, 5);

    wxStaticText* itemStaticText30 = new wxStaticText( itemPanel1, ID_RICHTEXTBULLETSPAGE_NUMBERSTATIC, _("&Number:"), wxDefaultPosition, wxDefaultSize, 0 );
    itemBoxSizer18->Add(itemStaticText30, 0, wxALIGN_LEFT|wxLEFT|wxRIGHT|wxTOP|wxADJUST_MINSIZE, 5);

    m_numberCtrl = new wxSpinCtrl( itemPanel1, ID_RICHTEXTBULLETSPAGE_NUMBERCTRL, _T("0"), wxDefaultPosition, wxSize(50, -1), wxSP_ARROW_KEYS, 0, 100000, 0 );
    m_numberCtrl->SetHelpText(_("The list item number."));
    if (wxRichTextBulletsPage::ShowToolTips())
        m_numberCtrl->SetToolTip(_("The list item number."));
    itemBoxSizer18->Add(m_numberCtrl, 0, wxGROW|wxALL, 5);

    itemBoxSizer3->Add(5, 5, 0, wxALIGN_CENTER_HORIZONTAL, 5);

    m_previewCtrl = new wxRichTextCtrl( itemPanel1, ID_RICHTEXTBULLETSPAGE_PREVIEW_CTRL, wxEmptyString, wxDefaultPosition, wxSize(350, 100), wxVSCROLL|wxTE_READONLY );
    m_previewCtrl->SetHelpText(_("Shows a preview of the bullet settings."));
    if (wxRichTextBulletsPage::ShowToolTips())
        m_previewCtrl->SetToolTip(_("Shows a preview of the bullet settings."));
    itemBoxSizer3->Add(m_previewCtrl, 1, wxGROW|wxALL, 5);

////@end wxRichTextBulletsPage content construction

    if (wxGetDisplaySize().y < 600)
        m_previewCtrl->SetMinSize(wxSize(350, 50));

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

    size_t i;
    for (i = 0; i < standardBulletNames.GetCount(); i++)
        m_bulletNameCtrl->Append(wxGetTranslation(standardBulletNames[i]));

    wxArrayString facenames = wxRichTextCtrl::GetAvailableFontNames();
    facenames.Sort();

    m_symbolFontCtrl->Append(facenames);
}

/// Transfer data from/to window
bool wxRichTextBulletsPage::TransferDataFromWindow()
{
    wxPanel::TransferDataFromWindow();

    wxTextAttrEx* attr = GetAttributes();

    int index = m_styleListBox->GetSelection();
    if (index < 1)
    {
        m_hasBulletStyle = false;
        m_hasBulletNumber = false;
        m_hasBulletSymbol = false;
        attr->SetBulletStyle(wxTEXT_ATTR_BULLET_STYLE_NONE);
        attr->SetFlags(attr->GetFlags() & ~(wxTEXT_ATTR_BULLET_STYLE|wxTEXT_ATTR_BULLET_NUMBER|wxTEXT_ATTR_BULLET_TEXT|wxTEXT_ATTR_BULLET_NAME));
    }
    else
    {
        m_hasBulletStyle = true;
    }

    if (m_hasBulletStyle)
    {
        long bulletStyle = wxRICHTEXT_BULLETINDEX_NONE;

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
            wxArrayString standardBulletNames;
            if (wxRichTextBuffer::GetRenderer() && m_bulletNameCtrl->GetSelection() != wxNOT_FOUND)
            {
                int sel = m_bulletNameCtrl->GetSelection();
                wxString selName = m_bulletNameCtrl->GetString(sel);

                // Try to get the untranslated name using the current selection index of the combobox.
                // into account.
                wxRichTextBuffer::GetRenderer()->EnumerateStandardBulletNames(standardBulletNames);
                if (sel < (int) standardBulletNames.GetCount() && m_bulletNameCtrl->GetValue() == selName)
                    attr->SetBulletName(standardBulletNames[sel]);
                else            
                    attr->SetBulletName(m_bulletNameCtrl->GetValue());
            }
            else
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

    if (m_hasBulletNumber)
    {
        attr->SetBulletNumber(m_numberCtrl->GetValue());
    }

    if (m_hasBulletSymbol)
    {
        attr->SetBulletText(m_symbolCtrl->GetValue());
        attr->SetBulletFont(m_symbolFontCtrl->GetValue());
    }

    return true;
}

bool wxRichTextBulletsPage::TransferDataToWindow()
{
    m_dontUpdate = true;

    wxPanel::TransferDataToWindow();

    wxTextAttrEx* attr = GetAttributes();

    if (attr->HasBulletStyle())
    {
        m_hasBulletStyle = true;
        int index = wxRICHTEXT_BULLETINDEX_NONE;

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
        m_hasBulletStyle = false;
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

    if (attr->HasBulletNumber())
        m_numberCtrl->SetValue(attr->GetBulletNumber());
    else
        m_numberCtrl->SetValue(0);

    if (attr->HasBulletName())
    {
        wxArrayString standardBulletNames;
        if (wxRichTextBuffer::GetRenderer())
        {
            // Try to set the control by index in order to take translated combo control strings
            // into account.
            wxRichTextBuffer::GetRenderer()->EnumerateStandardBulletNames(standardBulletNames);
            int idx = standardBulletNames.Index(attr->GetBulletName());
            if (idx != -1 && idx < (int) m_bulletNameCtrl->GetCount())
                m_bulletNameCtrl->SetSelection(idx);
            else
                m_bulletNameCtrl->SetValue(attr->GetBulletName());
        }
        else
            m_bulletNameCtrl->SetValue(attr->GetBulletName());
    }
    else
        m_bulletNameCtrl->SetValue(wxEmptyString);

    UpdatePreview();

    m_dontUpdate = false;

    return true;
}

/// Updates the bullet preview
void wxRichTextBulletsPage::UpdatePreview()
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
      (wxTEXT_ATTR_BULLET_STYLE|wxTEXT_ATTR_BULLET_NUMBER|wxTEXT_ATTR_BULLET_TEXT|wxTEXT_ATTR_BULLET_NAME|
       wxTEXT_ATTR_ALIGNMENT|wxTEXT_ATTR_LEFT_INDENT|wxTEXT_ATTR_RIGHT_INDENT|wxTEXT_ATTR_PARA_SPACING_BEFORE|wxTEXT_ATTR_PARA_SPACING_AFTER|
       wxTEXT_ATTR_LINE_SPACING));

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

    m_previewCtrl->NumberList(wxRichTextRange(0, m_previewCtrl->GetLastPosition()+1));

    m_previewCtrl->Thaw();
}

wxTextAttrEx* wxRichTextBulletsPage::GetAttributes()
{
    return wxRichTextFormattingDialog::GetDialogAttributes(this);
}

/*!
 * Should we show tooltips?
 */

bool wxRichTextBulletsPage::ShowToolTips()
{
    return wxRichTextFormattingDialog::ShowToolTips();
}

/*!
 * Get bitmap resources
 */

wxBitmap wxRichTextBulletsPage::GetBitmapResource( const wxString& name )
{
    // Bitmap retrieval
////@begin wxRichTextBulletsPage bitmap retrieval
    wxUnusedVar(name);
    return wxNullBitmap;
////@end wxRichTextBulletsPage bitmap retrieval
}

/*!
 * Get icon resources
 */

wxIcon wxRichTextBulletsPage::GetIconResource( const wxString& name )
{
    // Icon retrieval
////@begin wxRichTextBulletsPage icon retrieval
    wxUnusedVar(name);
    return wxNullIcon;
////@end wxRichTextBulletsPage icon retrieval
}

/*!
 * wxEVT_COMMAND_LISTBOX_SELECTED event handler for ID_RICHTEXTBULLETSPAGE_STYLELISTBOX
 */

void wxRichTextBulletsPage::OnStylelistboxSelected( wxCommandEvent& WXUNUSED(event) )
{
    if (!m_dontUpdate)
    {
        m_hasBulletStyle = true;

        if (m_styleListBox->GetSelection() == wxRICHTEXT_BULLETINDEX_SYMBOL)
            m_hasBulletSymbol = true;

        UpdatePreview();
    }
}

/*!
 * wxEVT_COMMAND_COMBOBOX_SELECTED event handler for ID_RICHTEXTBULLETSPAGE_SYMBOLCTRL
 */

void wxRichTextBulletsPage::OnSymbolctrlSelected( wxCommandEvent& WXUNUSED(event) )
{
    if (!m_dontUpdate)
    {
        m_hasBulletSymbol = true;
        UpdatePreview();
    }
}

/*!
 * wxEVT_COMMAND_TEXT_UPDATED event handler for ID_RICHTEXTBULLETSPAGE_SYMBOLCTRL
 */

void wxRichTextBulletsPage::OnSymbolctrlUpdated( wxCommandEvent& WXUNUSED(event) )
{
    if (!m_dontUpdate)
    {
        m_hasBulletSymbol = true;
        UpdatePreview();
    }
}

/*!
 * wxEVT_UPDATE_UI event handler for ID_RICHTEXTBULLETSPAGE_SYMBOLCTRL
 */

void wxRichTextBulletsPage::OnSymbolctrlUpdate( wxUpdateUIEvent& event )
{
    OnSymbolUpdate(event);
}

/*!
 * wxEVT_COMMAND_SPINCTRL_UPDATED event handler for ID_RICHTEXTBULLETSPAGE_NUMBERCTRL
 */

void wxRichTextBulletsPage::OnNumberctrlUpdated( wxSpinEvent& WXUNUSED(event) )
{
    if (!m_dontUpdate)
    {
        m_hasBulletNumber = true;
        UpdatePreview();
    }
}

/*!
 * wxEVT_SCROLL_LINEUP event handler for ID_RICHTEXTBULLETSPAGE_NUMBERCTRL
 */

void wxRichTextBulletsPage::OnNumberctrlUp( wxSpinEvent& WXUNUSED(event) )
{
    if (!m_dontUpdate)
    {
        m_hasBulletNumber = true;
        UpdatePreview();
    }
}

/*!
 * wxEVT_SCROLL_LINEDOWN event handler for ID_RICHTEXTBULLETSPAGE_NUMBERCTRL
 */

void wxRichTextBulletsPage::OnNumberctrlDown( wxSpinEvent& WXUNUSED(event) )
{
    if (!m_dontUpdate)
    {
        m_hasBulletNumber = true;
        UpdatePreview();
    }
}

/*!
 * wxEVT_COMMAND_TEXT_UPDATED event handler for ID_RICHTEXTBULLETSPAGE_NUMBERCTRL
 */

void wxRichTextBulletsPage::OnNumberctrlTextUpdated( wxCommandEvent& WXUNUSED(event) )
{
    if (!m_dontUpdate)
    {
        m_hasBulletNumber = true;
        UpdatePreview();
    }
}

/*!
 * wxEVT_UPDATE_UI event handler for ID_RICHTEXTBULLETSPAGE_NUMBERCTRL
 */

void wxRichTextBulletsPage::OnNumberctrlUpdate( wxUpdateUIEvent& event )
{
    OnNumberUpdate(event);
}

/*!
 * wxEVT_COMMAND_CHECKBOX_CLICKED event handler for ID_RICHTEXTBULLETSPAGE_PARENTHESESCTRL
 */

void wxRichTextBulletsPage::OnParenthesesctrlClick( wxCommandEvent& WXUNUSED(event) )
{
    if (!m_dontUpdate)
    {
        m_hasBulletStyle = true;
        UpdatePreview();
    }
}

/*!
 * wxEVT_UPDATE_UI event handler for ID_RICHTEXTBULLETSPAGE_PARENTHESESCTRL
 */

void wxRichTextBulletsPage::OnParenthesesctrlUpdate( wxUpdateUIEvent& event )
{
    int sel = m_styleListBox->GetSelection();
    event.Enable(m_hasBulletStyle && (sel != wxRICHTEXT_BULLETINDEX_SYMBOL &&
                                      sel != wxRICHTEXT_BULLETINDEX_BITMAP &&
                                      sel != wxRICHTEXT_BULLETINDEX_NONE));
}

/*!
 * wxEVT_COMMAND_CHECKBOX_CLICKED event handler for ID_RICHTEXTBULLETSPAGE_PERIODCTRL
 */

void wxRichTextBulletsPage::OnPeriodctrlClick( wxCommandEvent& WXUNUSED(event) )
{
    if (!m_dontUpdate)
    {
        m_hasBulletStyle = true;
        UpdatePreview();
    }
}

/*!
 * wxEVT_UPDATE_UI event handler for ID_RICHTEXTBULLETSPAGE_PERIODCTRL
 */

void wxRichTextBulletsPage::OnPeriodctrlUpdate( wxUpdateUIEvent& event )
{
    int sel = m_styleListBox->GetSelection();
    event.Enable(m_hasBulletStyle && (sel != wxRICHTEXT_BULLETINDEX_SYMBOL &&
                                      sel != wxRICHTEXT_BULLETINDEX_BITMAP &&
                                      sel != wxRICHTEXT_BULLETINDEX_NONE));
}

/*!
 * wxEVT_COMMAND_BUTTON_CLICKED event handler for ID_RICHTEXTBULLETSPAGE_CHOOSE_SYMBOL
 */

void wxRichTextBulletsPage::OnChooseSymbolClick( wxCommandEvent& WXUNUSED(event) )
{
    int sel = m_styleListBox->GetSelection();
    if (m_hasBulletStyle && sel == wxRICHTEXT_BULLETINDEX_SYMBOL)
    {
        wxString symbol = m_symbolCtrl->GetValue();
        wxString fontName = m_symbolFontCtrl->GetValue();
        wxSymbolPickerDialog dlg(symbol, fontName, fontName, this);

        if (dlg.ShowModal() == wxID_OK)
        {
            m_dontUpdate = true;

            m_symbolCtrl->SetValue(dlg.GetSymbol());
            m_symbolFontCtrl->SetValue(dlg.GetFontName());

            UpdatePreview();

            m_dontUpdate = false;
        }
    }
}

/*!
 * wxEVT_UPDATE_UI event handler for ID_RICHTEXTBULLETSPAGE_CHOOSE_SYMBOL
 */

void wxRichTextBulletsPage::OnChooseSymbolUpdate( wxUpdateUIEvent& event )
{
    OnSymbolUpdate(event);
}
/*!
 * wxEVT_COMMAND_COMBOBOX_SELECTED event handler for ID_RICHTEXTBULLETSPAGE_SYMBOLFONTCTRL
 */

void wxRichTextBulletsPage::OnSymbolfontctrlSelected( wxCommandEvent& WXUNUSED(event) )
{
    if (m_dontUpdate)
        return;
    UpdatePreview();
}

/*!
 * wxEVT_COMMAND_TEXT_UPDATED event handler for ID_RICHTEXTBULLETSPAGE_SYMBOLFONTCTRL
 */

void wxRichTextBulletsPage::OnSymbolfontctrlUpdated( wxCommandEvent& WXUNUSED(event) )
{
    if (m_dontUpdate)
        return;
    UpdatePreview();
}


/*!
 * wxEVT_UPDATE_UI event handler for ID_RICHTEXTBULLETSPAGE_SYMBOLFONTCTRL
 */

void wxRichTextBulletsPage::OnSymbolfontctrlUIUpdate( wxUpdateUIEvent& event )
{
    OnSymbolUpdate(event);
}

/// Update for symbol-related controls
void wxRichTextBulletsPage::OnSymbolUpdate( wxUpdateUIEvent& event )
{
    int sel = m_styleListBox->GetSelection();
    event.Enable(m_hasBulletStyle && (sel == wxRICHTEXT_BULLETINDEX_SYMBOL));
}

/// Update for number-related controls
void wxRichTextBulletsPage::OnNumberUpdate( wxUpdateUIEvent& event )
{
    int sel = m_styleListBox->GetSelection();
    event.Enable( m_hasBulletStyle && (sel != wxRICHTEXT_BULLETINDEX_SYMBOL &&
                                       sel != wxRICHTEXT_BULLETINDEX_BITMAP &&
                                       sel != wxRICHTEXT_BULLETINDEX_STANDARD &&
                                       sel != wxRICHTEXT_BULLETINDEX_NONE));
}

/// Update for standard bullet-related controls
void wxRichTextBulletsPage::OnStandardBulletUpdate( wxUpdateUIEvent& event )
{
    int sel = m_styleListBox->GetSelection();
    event.Enable( sel == wxRICHTEXT_BULLETINDEX_STANDARD );
}


/*!
 * wxEVT_UPDATE_UI event handler for ID_RICHTEXTBULLETSPAGE_SYMBOLSTATIC
 */

void wxRichTextBulletsPage::OnSymbolstaticUpdate( wxUpdateUIEvent& event )
{
    OnSymbolUpdate(event);
}


/*!
 * wxEVT_UPDATE_UI event handler for ID_RICHTEXTBULLETSPAGE_NUMBERSTATIC
 */

void wxRichTextBulletsPage::OnNumberstaticUpdate( wxUpdateUIEvent& event )
{
    OnNumberUpdate(event);
}

/*!
 * wxEVT_UPDATE_UI event handler for ID_RICHTEXTBULLETSPAGE_NAMESTATIC
 */

void wxRichTextBulletsPage::OnNamestaticUpdate( wxUpdateUIEvent& event )
{
    OnStandardBulletUpdate(event);
}


/*!
 * wxEVT_COMMAND_COMBOBOX_SELECTED event handler for ID_RICHTEXTBULLETSPAGE_NAMECTRL
 */

void wxRichTextBulletsPage::OnNamectrlSelected( wxCommandEvent& WXUNUSED(event) )
{
    if (m_dontUpdate)
        return;
    UpdatePreview();
}

/*!
 * wxEVT_COMMAND_TEXT_UPDATED event handler for ID_RICHTEXTBULLETSPAGE_NAMECTRL
 */

void wxRichTextBulletsPage::OnNamectrlUpdated( wxCommandEvent& WXUNUSED(event) )
{
    if (m_dontUpdate)
        return;
    UpdatePreview();
}

/*!
 * wxEVT_UPDATE_UI event handler for ID_RICHTEXTBULLETSPAGE_NAMECTRL
 */

void wxRichTextBulletsPage::OnNamectrlUIUpdate( wxUpdateUIEvent& event )
{
    OnStandardBulletUpdate(event);
}

#endif // wxUSE_RICHTEXT
/*!
 * wxEVT_COMMAND_CHECKBOX_CLICKED event handler for ID_RICHTEXTBULLETSPAGE_RIGHT_PARENTHESIS_CTRL
 */

void wxRichTextBulletsPage::OnRightParenthesisCtrlClick( wxCommandEvent& WXUNUSED(event) )
{
    if (!m_dontUpdate)
    {
        m_hasBulletStyle = true;
        UpdatePreview();
    }
}

/*!
 * wxEVT_UPDATE_UI event handler for ID_RICHTEXTBULLETSPAGE_RIGHT_PARENTHESIS_CTRL
 */

void wxRichTextBulletsPage::OnRightParenthesisCtrlUpdate( wxUpdateUIEvent& event )
{
    int sel = m_styleListBox->GetSelection();
    event.Enable(m_hasBulletStyle && (sel != wxRICHTEXT_BULLETINDEX_SYMBOL &&
                                      sel != wxRICHTEXT_BULLETINDEX_BITMAP &&
                                      sel != wxRICHTEXT_BULLETINDEX_NONE));
}

/*!
 * wxEVT_COMMAND_COMBOBOX_SELECTED event handler for ID_COMBOBOX
 */

void wxRichTextBulletsPage::OnBulletAlignmentCtrlSelected( wxCommandEvent& WXUNUSED(event) )
{
    if (m_dontUpdate)
        return;
    UpdatePreview();
}


