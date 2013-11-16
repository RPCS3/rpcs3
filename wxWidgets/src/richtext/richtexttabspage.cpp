/////////////////////////////////////////////////////////////////////////////
// Name:        src/richtext/richtexttabspage.cpp
// Purpose:
// Author:      Julian Smart
// Modified by:
// Created:     10/4/2006 8:03:20 AM
// RCS-ID:      $Id: richtexttabspage.cpp 59814 2009-03-24 19:05:15Z JS $
// Copyright:   (c) Julian Smart
// Licence:     wxWindows licence
/////////////////////////////////////////////////////////////////////////////

#if wxUSE_RICHTEXT

#include "wx/richtext/richtexttabspage.h"

/*!
 * wxRichTextTabsPage type definition
 */

IMPLEMENT_DYNAMIC_CLASS( wxRichTextTabsPage, wxPanel )

/*!
 * wxRichTextTabsPage event table definition
 */

BEGIN_EVENT_TABLE( wxRichTextTabsPage, wxPanel )

////@begin wxRichTextTabsPage event table entries
    EVT_LISTBOX( ID_RICHTEXTTABSPAGE_TABLIST, wxRichTextTabsPage::OnTablistSelected )

    EVT_BUTTON( ID_RICHTEXTTABSPAGE_NEW_TAB, wxRichTextTabsPage::OnNewTabClick )
    EVT_UPDATE_UI( ID_RICHTEXTTABSPAGE_NEW_TAB, wxRichTextTabsPage::OnNewTabUpdate )

    EVT_BUTTON( ID_RICHTEXTTABSPAGE_DELETE_TAB, wxRichTextTabsPage::OnDeleteTabClick )
    EVT_UPDATE_UI( ID_RICHTEXTTABSPAGE_DELETE_TAB, wxRichTextTabsPage::OnDeleteTabUpdate )

    EVT_BUTTON( ID_RICHTEXTTABSPAGE_DELETE_ALL_TABS, wxRichTextTabsPage::OnDeleteAllTabsClick )
    EVT_UPDATE_UI( ID_RICHTEXTTABSPAGE_DELETE_ALL_TABS, wxRichTextTabsPage::OnDeleteAllTabsUpdate )

////@end wxRichTextTabsPage event table entries

END_EVENT_TABLE()

/*!
 * wxRichTextTabsPage constructors
 */

wxRichTextTabsPage::wxRichTextTabsPage( )
{
    Init();
}

wxRichTextTabsPage::wxRichTextTabsPage( wxWindow* parent, wxWindowID id, const wxPoint& pos, const wxSize& size, long style )
{
    Init();
    Create(parent, id, pos, size, style);
}

/*!
 * Initialise members
 */

void wxRichTextTabsPage::Init()
{
    m_tabsPresent = false;

////@begin wxRichTextTabsPage member initialisation
    m_tabEditCtrl = NULL;
    m_tabListCtrl = NULL;
////@end wxRichTextTabsPage member initialisation
}

/*!
 * wxRichTextTabsPage creator
 */

bool wxRichTextTabsPage::Create( wxWindow* parent, wxWindowID id, const wxPoint& pos, const wxSize& size, long style )
{
////@begin wxRichTextTabsPage creation
    wxPanel::Create( parent, id, pos, size, style );

    CreateControls();
    if (GetSizer())
    {
        GetSizer()->SetSizeHints(this);
    }
    Centre();
////@end wxRichTextTabsPage creation
    return true;
}

/*!
 * Control creation for wxRichTextTabsPage
 */

void wxRichTextTabsPage::CreateControls()
{
////@begin wxRichTextTabsPage content construction
    wxRichTextTabsPage* itemPanel1 = this;

    wxBoxSizer* itemBoxSizer2 = new wxBoxSizer(wxVERTICAL);
    itemPanel1->SetSizer(itemBoxSizer2);

    wxBoxSizer* itemBoxSizer3 = new wxBoxSizer(wxVERTICAL);
    itemBoxSizer2->Add(itemBoxSizer3, 1, wxGROW|wxALL, 5);

    wxBoxSizer* itemBoxSizer4 = new wxBoxSizer(wxHORIZONTAL);
    itemBoxSizer3->Add(itemBoxSizer4, 1, wxGROW, 5);

    wxBoxSizer* itemBoxSizer5 = new wxBoxSizer(wxVERTICAL);
    itemBoxSizer4->Add(itemBoxSizer5, 0, wxGROW, 5);

    wxStaticText* itemStaticText6 = new wxStaticText( itemPanel1, wxID_STATIC, _("&Position (tenths of a mm):"), wxDefaultPosition, wxDefaultSize, 0 );
    itemBoxSizer5->Add(itemStaticText6, 0, wxALIGN_LEFT|wxLEFT|wxRIGHT|wxTOP|wxADJUST_MINSIZE, 5);

    m_tabEditCtrl = new wxTextCtrl( itemPanel1, ID_RICHTEXTTABSPAGE_TABEDIT, _T(""), wxDefaultPosition, wxDefaultSize, 0 );
    m_tabEditCtrl->SetHelpText(_("The tab position."));
    if (wxRichTextTabsPage::ShowToolTips())
        m_tabEditCtrl->SetToolTip(_("The tab position."));
    itemBoxSizer5->Add(m_tabEditCtrl, 0, wxGROW|wxALL, 5);

    wxArrayString m_tabListCtrlStrings;
    m_tabListCtrlStrings.Add(_("The tab positions."));
    m_tabListCtrl = new wxListBox( itemPanel1, ID_RICHTEXTTABSPAGE_TABLIST, wxDefaultPosition, wxSize(80, 180), m_tabListCtrlStrings, wxLB_SINGLE );
    itemBoxSizer5->Add(m_tabListCtrl, 1, wxGROW|wxLEFT|wxRIGHT|wxBOTTOM, 5);

    itemBoxSizer4->Add(2, 1, 1, wxALIGN_CENTER_VERTICAL|wxTOP|wxBOTTOM, 5);

    wxBoxSizer* itemBoxSizer10 = new wxBoxSizer(wxVERTICAL);
    itemBoxSizer4->Add(itemBoxSizer10, 0, wxGROW, 5);

    wxStaticText* itemStaticText11 = new wxStaticText( itemPanel1, wxID_STATIC, _T(""), wxDefaultPosition, wxDefaultSize, 0 );
    itemBoxSizer10->Add(itemStaticText11, 0, wxALIGN_CENTER_HORIZONTAL|wxBOTTOM|wxADJUST_MINSIZE, 5);

    wxButton* itemButton12 = new wxButton( itemPanel1, ID_RICHTEXTTABSPAGE_NEW_TAB, _("&New"), wxDefaultPosition, wxDefaultSize, 0 );
    itemButton12->SetHelpText(_("Click to create a new tab position."));
    if (wxRichTextTabsPage::ShowToolTips())
        itemButton12->SetToolTip(_("Click to create a new tab position."));
    itemBoxSizer10->Add(itemButton12, 0, wxGROW|wxALL, 5);

    wxButton* itemButton13 = new wxButton( itemPanel1, ID_RICHTEXTTABSPAGE_DELETE_TAB, _("&Delete"), wxDefaultPosition, wxDefaultSize, 0 );
    itemButton13->SetHelpText(_("Click to delete the selected tab position."));
    if (wxRichTextTabsPage::ShowToolTips())
        itemButton13->SetToolTip(_("Click to delete the selected tab position."));
    itemBoxSizer10->Add(itemButton13, 0, wxGROW|wxLEFT|wxRIGHT|wxBOTTOM, 5);

    wxButton* itemButton14 = new wxButton( itemPanel1, ID_RICHTEXTTABSPAGE_DELETE_ALL_TABS, _("Delete A&ll"), wxDefaultPosition, wxDefaultSize, 0 );
    itemButton14->SetHelpText(_("Click to delete all tab positions."));
    if (wxRichTextTabsPage::ShowToolTips())
        itemButton14->SetToolTip(_("Click to delete all tab positions."));
    itemBoxSizer10->Add(itemButton14, 0, wxGROW|wxLEFT|wxRIGHT|wxBOTTOM, 5);

////@end wxRichTextTabsPage content construction
}

/// Transfer data from/to window
bool wxRichTextTabsPage::TransferDataFromWindow()
{
    wxPanel::TransferDataFromWindow();

    wxTextAttrEx* attr = GetAttributes();

    if (m_tabsPresent)
    {
        wxArrayInt tabs;
        size_t i;
        for (i = 0; i < m_tabListCtrl->GetCount(); i++)
        {
            tabs.Add(wxAtoi(m_tabListCtrl->GetString(i)));
        }
        attr->SetTabs(tabs);
    }
    return true;
}

bool wxRichTextTabsPage::TransferDataToWindow()
{
    wxPanel::TransferDataToWindow();

    wxTextAttrEx* attr = GetAttributes();

    m_tabListCtrl->Clear();
    m_tabEditCtrl->SetValue(wxEmptyString);

    if (attr->HasTabs())
    {
        m_tabsPresent = true;
        size_t i;
        for (i = 0; i < attr->GetTabs().GetCount(); i++)
        {
            wxString s(wxString::Format(wxT("%d"), attr->GetTabs()[i]));
            m_tabListCtrl->Append(s);
        }
    }

    return true;
}

static int wxTabSortFunc(int* a, int* b)
{
    if ((*a) < (*b))
        return -1;
    else if ((*b) < (*a))
        return 1;
    else
        return 0;
}

/// Sorts the tab array
void wxRichTextTabsPage::SortTabs()
{
    wxArrayInt tabs;
    size_t i;
    for (i = 0; i < m_tabListCtrl->GetCount(); i++)
    {
        tabs.Add(wxAtoi(m_tabListCtrl->GetString(i)));
    }
    tabs.Sort(& wxTabSortFunc);

    m_tabListCtrl->Clear();
    for (i = 0; i < tabs.GetCount(); i++)
    {
        wxString s(wxString::Format(wxT("%d"), tabs[i]));
        m_tabListCtrl->Append(s);
    }
}

wxTextAttrEx* wxRichTextTabsPage::GetAttributes()
{
    return wxRichTextFormattingDialog::GetDialogAttributes(this);
}

/*!
 * Should we show tooltips?
 */

bool wxRichTextTabsPage::ShowToolTips()
{
    return wxRichTextFormattingDialog::ShowToolTips();
}

/*!
 * Get bitmap resources
 */

wxBitmap wxRichTextTabsPage::GetBitmapResource( const wxString& name )
{
    // Bitmap retrieval
////@begin wxRichTextTabsPage bitmap retrieval
    wxUnusedVar(name);
    return wxNullBitmap;
////@end wxRichTextTabsPage bitmap retrieval
}

/*!
 * Get icon resources
 */

wxIcon wxRichTextTabsPage::GetIconResource( const wxString& name )
{
    // Icon retrieval
////@begin wxRichTextTabsPage icon retrieval
    wxUnusedVar(name);
    return wxNullIcon;
////@end wxRichTextTabsPage icon retrieval
}

/*!
 * wxEVT_COMMAND_BUTTON_CLICKED event handler for ID_RICHTEXTTABSPAGE_NEW_TAB
 */

void wxRichTextTabsPage::OnNewTabClick( wxCommandEvent& WXUNUSED(event) )
{
    wxString str = m_tabEditCtrl->GetValue();
    if (!str.empty() && str.IsNumber())
    {
        wxString s(wxString::Format(wxT("%d"), wxAtoi(str)));

        m_tabListCtrl->Append(s);
        m_tabsPresent = true;

        SortTabs();
    }
}

/*!
 * wxEVT_UPDATE_UI event handler for ID_RICHTEXTTABSPAGE_NEW_TAB
 */

void wxRichTextTabsPage::OnNewTabUpdate( wxUpdateUIEvent& event )
{
    // This may be a bit expensive - consider updating New button when text
    // changes in edit control
    wxString str = m_tabEditCtrl->GetValue();
    if (!str.empty() && str.IsNumber())
    {
        wxString s(wxString::Format(wxT("%d"), wxAtoi(str)));
        event.Enable(m_tabListCtrl->FindString(s) == wxNOT_FOUND);
    }
    else
        event.Enable(false);
}


/*!
 * wxEVT_COMMAND_BUTTON_CLICKED event handler for ID_RICHTEXTTABSPAGE_DELETE_TAB
 */

void wxRichTextTabsPage::OnDeleteTabClick( wxCommandEvent& WXUNUSED(event) )
{
    if (m_tabsPresent && m_tabListCtrl->GetCount() > 0 && m_tabListCtrl->GetSelection() != wxNOT_FOUND)
    {
        m_tabListCtrl->Delete(m_tabListCtrl->GetSelection());
    }
}

/*!
 * wxEVT_UPDATE_UI event handler for ID_RICHTEXTTABSPAGE_DELETE_TAB
 */

void wxRichTextTabsPage::OnDeleteTabUpdate( wxUpdateUIEvent& event )
{
    event.Enable( m_tabsPresent && m_tabListCtrl->GetCount() > 0 && m_tabListCtrl->GetSelection() != wxNOT_FOUND );
}


/*!
 * wxEVT_COMMAND_BUTTON_CLICKED event handler for ID_RICHTEXTTABSPAGE_DELETE_ALL_TABS
 */

void wxRichTextTabsPage::OnDeleteAllTabsClick( wxCommandEvent& WXUNUSED(event) )
{
    if (m_tabsPresent && m_tabListCtrl->GetCount() > 0)
    {
        m_tabListCtrl->Clear();
        m_tabEditCtrl->SetValue(wxEmptyString);
    }
}

/*!
 * wxEVT_UPDATE_UI event handler for ID_RICHTEXTTABSPAGE_DELETE_ALL_TABS
 */

void wxRichTextTabsPage::OnDeleteAllTabsUpdate( wxUpdateUIEvent& event )
{
    event.Enable( m_tabsPresent && m_tabListCtrl->GetCount() > 0 );
}


/*!
 * wxEVT_COMMAND_LISTBOX_SELECTED event handler for ID_RICHTEXTTABSPAGE_TABLIST
 */

void wxRichTextTabsPage::OnTablistSelected( wxCommandEvent& WXUNUSED(event) )
{
    wxString str = m_tabListCtrl->GetStringSelection();
    if (!str.empty())
        m_tabEditCtrl->SetValue(str);
}

#endif // wxUSE_RICHTEXT
