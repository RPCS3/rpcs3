/////////////////////////////////////////////////////////////////////////////
// Name:        src/generic/choicdgg.cpp
// Purpose:     Choice dialogs
// Author:      Julian Smart
// Modified by: 03.11.00: VZ to add wxArrayString and multiple sel functions
// Created:     04/01/98
// RCS-ID:      $Id: choicdgg.cpp 44620 2007-03-06 09:56:43Z JS $
// Copyright:   (c) wxWidgets team
// Licence:     wxWindows licence
/////////////////////////////////////////////////////////////////////////////

// ============================================================================
// declarations
// ============================================================================

// ----------------------------------------------------------------------------
// headers
// ----------------------------------------------------------------------------

// For compilers that support precompilation, includes "wx.h".
#include "wx/wxprec.h"

#ifdef __BORLANDC__
    #pragma hdrstop
#endif

#if wxUSE_CHOICEDLG

#ifndef WX_PRECOMP
    #include <stdio.h>
    #include "wx/utils.h"
    #include "wx/dialog.h"
    #include "wx/button.h"
    #include "wx/listbox.h"
    #include "wx/checklst.h"
    #include "wx/stattext.h"
    #include "wx/intl.h"
    #include "wx/sizer.h"
    #include "wx/arrstr.h"
#endif

#include "wx/statline.h"
#include "wx/settings.h"
#include "wx/generic/choicdgg.h"

// ----------------------------------------------------------------------------
// constants
// ----------------------------------------------------------------------------

#define wxID_LISTBOX 3000

// ----------------------------------------------------------------------------
// private functions
// ----------------------------------------------------------------------------

// convert wxArrayString into a wxString[] which must be delete[]d by caller
static int ConvertWXArrayToC(const wxArrayString& aChoices, wxString **choices);

// ============================================================================
// implementation
// ============================================================================

// ----------------------------------------------------------------------------
// helpers
// ----------------------------------------------------------------------------

int ConvertWXArrayToC(const wxArrayString& aChoices, wxString **choices)
{
    int n = aChoices.GetCount();
    *choices = new wxString[n];

    for ( int i = 0; i < n; i++ )
    {
        (*choices)[i] = aChoices[i];
    }

    return n;
}

// ----------------------------------------------------------------------------
// wrapper functions
// ----------------------------------------------------------------------------

wxString wxGetSingleChoice( const wxString& message,
                            const wxString& caption,
                            int n, const wxString *choices,
                            wxWindow *parent,
                            int WXUNUSED(x), int WXUNUSED(y),
                            bool WXUNUSED(centre),
                            int WXUNUSED(width), int WXUNUSED(height) )
{
    wxSingleChoiceDialog dialog(parent, message, caption, n, choices);
    wxString choice;
    if ( dialog.ShowModal() == wxID_OK )
        choice = dialog.GetStringSelection();

    return choice;
}

wxString wxGetSingleChoice( const wxString& message,
                            const wxString& caption,
                            const wxArrayString& aChoices,
                            wxWindow *parent,
                            int x, int y,
                            bool centre,
                            int width, int height)
{
    wxString *choices;
    int n = ConvertWXArrayToC(aChoices, &choices);
    wxString res = wxGetSingleChoice(message, caption, n, choices, parent,
                                     x, y, centre, width, height);
    delete [] choices;

    return res;
}

int wxGetSingleChoiceIndex( const wxString& message,
                            const wxString& caption,
                            int n, const wxString *choices,
                            wxWindow *parent,
                            int WXUNUSED(x), int WXUNUSED(y),
                            bool WXUNUSED(centre),
                            int WXUNUSED(width), int WXUNUSED(height) )
{
    wxSingleChoiceDialog dialog(parent, message, caption, n, choices);
    int choice;
    if ( dialog.ShowModal() == wxID_OK )
        choice = dialog.GetSelection();
    else
        choice = -1;

    return choice;
}

int wxGetSingleChoiceIndex( const wxString& message,
                            const wxString& caption,
                            const wxArrayString& aChoices,
                            wxWindow *parent,
                            int x, int y,
                            bool centre,
                            int width, int height)
{
    wxString *choices;
    int n = ConvertWXArrayToC(aChoices, &choices);
    int res = wxGetSingleChoiceIndex(message, caption, n, choices, parent,
                                     x, y, centre, width, height);
    delete [] choices;

    return res;
}

void *wxGetSingleChoiceData( const wxString& message,
                             const wxString& caption,
                             int n, const wxString *choices,
                             void **client_data,
                             wxWindow *parent,
                             int WXUNUSED(x), int WXUNUSED(y),
                             bool WXUNUSED(centre),
                             int WXUNUSED(width), int WXUNUSED(height) )
{
    wxSingleChoiceDialog dialog(parent, message, caption, n, choices,
                                (char **)client_data);
    void *data;
    if ( dialog.ShowModal() == wxID_OK )
        data = dialog.GetSelectionClientData();
    else
        data = NULL;

    return data;
}

void *wxGetSingleChoiceData( const wxString& message,
                             const wxString& caption,
                             const wxArrayString& aChoices,
                             void **client_data,
                             wxWindow *parent,
                             int x, int y,
                             bool centre,
                             int width, int height)
{
    wxString *choices;
    int n = ConvertWXArrayToC(aChoices, &choices);
    void *res = wxGetSingleChoiceData(message, caption, n, choices,
                                      client_data, parent,
                                      x, y, centre, width, height);
    delete [] choices;

    return res;
}

size_t wxGetMultipleChoices(wxArrayInt& selections,
                            const wxString& message,
                            const wxString& caption,
                            int n, const wxString *choices,
                            wxWindow *parent,
                            int WXUNUSED(x), int WXUNUSED(y),
                            bool WXUNUSED(centre),
                            int WXUNUSED(width), int WXUNUSED(height))
{
    wxMultiChoiceDialog dialog(parent, message, caption, n, choices);

    // call this even if selections array is empty and this then (correctly)
    // deselects the first item which is selected by default
    dialog.SetSelections(selections);

    if ( dialog.ShowModal() == wxID_OK )
        selections = dialog.GetSelections();
    else
        selections.Empty();

    return selections.GetCount();
}

size_t wxGetMultipleChoices(wxArrayInt& selections,
                            const wxString& message,
                            const wxString& caption,
                            const wxArrayString& aChoices,
                            wxWindow *parent,
                            int x, int y,
                            bool centre,
                            int width, int height)
{
    wxString *choices;
    int n = ConvertWXArrayToC(aChoices, &choices);
    size_t res = wxGetMultipleChoices(selections, message, caption,
                                      n, choices, parent,
                                      x, y, centre, width, height);
    delete [] choices;

    return res;
}

// ----------------------------------------------------------------------------
// wxAnyChoiceDialog
// ----------------------------------------------------------------------------

bool wxAnyChoiceDialog::Create(wxWindow *parent,
                               const wxString& message,
                               const wxString& caption,
                               int n, const wxString *choices,
                               long styleDlg,
                               const wxPoint& pos,
                               long styleLbox)
{
#ifdef __WXMAC__
    // FIXME: why??
    if ( !wxDialog::Create(parent, wxID_ANY, caption, pos, wxDefaultSize, styleDlg & (~wxCANCEL) ) )
        return false;
#else
    if ( !wxDialog::Create(parent, wxID_ANY, caption, pos, wxDefaultSize, styleDlg) )
        return false;
#endif

    wxBoxSizer *topsizer = new wxBoxSizer( wxVERTICAL );

    // 1) text message
    topsizer->
        Add(CreateTextSizer(message), wxSizerFlags().Expand().TripleBorder());

    // 2) list box
    m_listbox = CreateList(n, choices, styleLbox);

    if ( n > 0 )
        m_listbox->SetSelection(0);

    topsizer->
        Add(m_listbox, wxSizerFlags().Expand().Proportion(1).TripleBorder(wxLEFT | wxRIGHT));

    // 3) buttons if any
    wxSizer *
        buttonSizer = CreateSeparatedButtonSizer(styleDlg & ButtonSizerFlags);
    if ( buttonSizer )
    {
        topsizer->Add(buttonSizer, wxSizerFlags().Expand().DoubleBorder());
    }

    SetSizer( topsizer );

    topsizer->SetSizeHints( this );
    topsizer->Fit( this );

    if ( styleDlg & wxCENTRE )
        Centre(wxBOTH);

    m_listbox->SetFocus();

    return true;
}

bool wxAnyChoiceDialog::Create(wxWindow *parent,
                               const wxString& message,
                               const wxString& caption,
                               const wxArrayString& choices,
                               long styleDlg,
                               const wxPoint& pos,
                               long styleLbox)
{
    wxCArrayString chs(choices);
    return Create(parent, message, caption, chs.GetCount(), chs.GetStrings(),
                  styleDlg, pos, styleLbox);
}

wxListBoxBase *wxAnyChoiceDialog::CreateList(int n, const wxString *choices, long styleLbox)
{
	wxSize size = wxDefaultSize;
	if (wxSystemSettings::GetScreenType() > wxSYS_SCREEN_PDA)
		size = wxSize(300, 200);
    return new wxListBox( this, wxID_LISTBOX,
                          wxDefaultPosition, size,
                          n, choices,
                          styleLbox );
}

// ----------------------------------------------------------------------------
// wxSingleChoiceDialog
// ----------------------------------------------------------------------------

BEGIN_EVENT_TABLE(wxSingleChoiceDialog, wxDialog)
    EVT_BUTTON(wxID_OK, wxSingleChoiceDialog::OnOK)
#ifndef __SMARTPHONE__
    EVT_LISTBOX_DCLICK(wxID_LISTBOX, wxSingleChoiceDialog::OnListBoxDClick)
#endif
#ifdef __WXWINCE__
    EVT_JOY_BUTTON_DOWN(wxSingleChoiceDialog::OnJoystickButtonDown)
#endif
END_EVENT_TABLE()

IMPLEMENT_DYNAMIC_CLASS(wxSingleChoiceDialog, wxDialog)

wxSingleChoiceDialog::wxSingleChoiceDialog(wxWindow *parent,
                                           const wxString& message,
                                           const wxString& caption,
                                           int n,
                                           const wxString *choices,
                                           char **clientData,
                                           long style,
                                           const wxPoint& WXUNUSED(pos))
{
    Create(parent, message, caption, n, choices, clientData, style);
}

wxSingleChoiceDialog::wxSingleChoiceDialog(wxWindow *parent,
                                           const wxString& message,
                                           const wxString& caption,
                                           const wxArrayString& choices,
                                           char **clientData,
                                           long style,
                                           const wxPoint& WXUNUSED(pos))
{
    Create(parent, message, caption, choices, clientData, style);
}

bool wxSingleChoiceDialog::Create( wxWindow *parent,
                                   const wxString& message,
                                   const wxString& caption,
                                   int n,
                                   const wxString *choices,
                                   char **clientData,
                                   long style,
                                   const wxPoint& pos )
{
    if ( !wxAnyChoiceDialog::Create(parent, message, caption,
                                    n, choices,
                                    style, pos) )
        return false;

    m_selection = n > 0 ? 0 : -1;

    if (clientData)
    {
        for (int i = 0; i < n; i++)
            m_listbox->SetClientData(i, clientData[i]);
    }

    return true;
}

bool wxSingleChoiceDialog::Create( wxWindow *parent,
                                   const wxString& message,
                                   const wxString& caption,
                                   const wxArrayString& choices,
                                   char **clientData,
                                   long style,
                                   const wxPoint& pos )
{
    wxCArrayString chs(choices);
    return Create( parent, message, caption, chs.GetCount(), chs.GetStrings(),
                   clientData, style, pos );
}

// Set the selection
void wxSingleChoiceDialog::SetSelection(int sel)
{
    m_listbox->SetSelection(sel);
    m_selection = sel;
}

void wxSingleChoiceDialog::OnOK(wxCommandEvent& WXUNUSED(event))
{
    DoChoice();
}

#ifndef __SMARTPHONE__
void wxSingleChoiceDialog::OnListBoxDClick(wxCommandEvent& WXUNUSED(event))
{
    DoChoice();
}
#endif

#ifdef __WXWINCE__
void wxSingleChoiceDialog::OnJoystickButtonDown(wxJoystickEvent& WXUNUSED(event))
{
    DoChoice();
}
#endif

void wxSingleChoiceDialog::DoChoice()
{
    m_selection = m_listbox->GetSelection();
    m_stringSelection = m_listbox->GetStringSelection();

    if ( m_listbox->HasClientUntypedData() )
        SetClientData(m_listbox->GetClientData(m_selection));

    EndModal(wxID_OK);
}

// ----------------------------------------------------------------------------
// wxMultiChoiceDialog
// ----------------------------------------------------------------------------

IMPLEMENT_DYNAMIC_CLASS(wxMultiChoiceDialog, wxDialog)

bool wxMultiChoiceDialog::Create( wxWindow *parent,
                                  const wxString& message,
                                  const wxString& caption,
                                  int n,
                                  const wxString *choices,
                                  long style,
                                  const wxPoint& pos )
{
    long styleLbox;
#if wxUSE_CHECKLISTBOX
    styleLbox = wxLB_ALWAYS_SB;
#else
    styleLbox = wxLB_ALWAYS_SB | wxLB_EXTENDED;
#endif

    if ( !wxAnyChoiceDialog::Create(parent, message, caption,
                                    n, choices,
                                    style, pos,
                                    styleLbox) )
        return false;

    return true;
}

bool wxMultiChoiceDialog::Create( wxWindow *parent,
                                  const wxString& message,
                                  const wxString& caption,
                                  const wxArrayString& choices,
                                  long style,
                                  const wxPoint& pos )
{
    wxCArrayString chs(choices);
    return Create( parent, message, caption, chs.GetCount(),
                   chs.GetStrings(), style, pos );
}

void wxMultiChoiceDialog::SetSelections(const wxArrayInt& selections)
{
#if wxUSE_CHECKLISTBOX
    wxCheckListBox* checkListBox = wxDynamicCast(m_listbox, wxCheckListBox);
    if (checkListBox)
    {
        // first clear all currently selected items
        size_t n,
            count = checkListBox->GetCount();
        for ( n = 0; n < count; ++n )
        {
            if (checkListBox->IsChecked(n))
                checkListBox->Check(n, false);
        }

        // now select the ones which should be selected
        count = selections.GetCount();
        for ( n = 0; n < count; n++ )
        {
            checkListBox->Check(selections[n]);
        }

        return;
    }
#endif

    // first clear all currently selected items
    size_t n,
           count = m_listbox->GetCount();
    for ( n = 0; n < count; ++n )
    {
        m_listbox->Deselect(n);
    }

    // now select the ones which should be selected
    count = selections.GetCount();
    for ( n = 0; n < count; n++ )
    {
        m_listbox->Select(selections[n]);
    }
}

bool wxMultiChoiceDialog::TransferDataFromWindow()
{
    m_selections.Empty();

#if wxUSE_CHECKLISTBOX
    wxCheckListBox* checkListBox = wxDynamicCast(m_listbox, wxCheckListBox);
    if (checkListBox)
    {
        size_t count = checkListBox->GetCount();
        for ( size_t n = 0; n < count; n++ )
        {
            if ( checkListBox->IsChecked(n) )
                m_selections.Add(n);
        }
        return true;
    }
#endif

    size_t count = m_listbox->GetCount();
    for ( size_t n = 0; n < count; n++ )
    {
        if ( m_listbox->IsSelected(n) )
            m_selections.Add(n);
    }

    return true;
}

#if wxUSE_CHECKLISTBOX

wxListBoxBase *wxMultiChoiceDialog::CreateList(int n, const wxString *choices, long styleLbox)
{
	wxSize size = wxDefaultSize;
	if (wxSystemSettings::GetScreenType() > wxSYS_SCREEN_PDA)
		size = wxSize(300, 200);

    return new wxCheckListBox( this, wxID_LISTBOX,
                               wxDefaultPosition, size,
                               n, choices,
                               styleLbox );
}

#endif // wxUSE_CHECKLISTBOX

#endif // wxUSE_CHOICEDLG
