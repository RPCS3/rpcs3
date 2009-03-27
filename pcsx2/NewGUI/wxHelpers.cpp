
#include "PrecompiledHeader.h"

#include <wx/wx.h>
#include "wxHelpers.h"

#include <wx/cshelp.h>

#if wxUSE_TOOLTIPS
#   include <wx/tooltip.h>
#endif

namespace wxHelpers
{
	wxSizerFlags stdCenteredFlags( wxSizerFlags().Align(wxALIGN_CENTER).DoubleBorder() );
	wxSizerFlags stdSpacingFlags( wxSizerFlags().Border( wxALL, 6 ) );
	wxSizerFlags stdButtonSizerFlags( wxSizerFlags().Align(wxALIGN_RIGHT).Border() );
	wxSizerFlags CheckboxFlags( wxSizerFlags().Border( wxALL, 6 ).Expand() );
	
	wxCheckBox& AddCheckBox( wxWindow* parent, wxBoxSizer& sizer, const wxString& label, wxWindowID id )
	{
		wxCheckBox* retval = new wxCheckBox( parent, id, label );
		sizer.Add( retval, CheckboxFlags );
		return *retval;
	}
}

CheckedStaticBox::CheckedStaticBox( wxWindow* parent, int orientation, const wxString& title=wxEmptyString ) :
	wxPanel( parent ),
	BoxSizer( *new wxStaticBoxSizer( orientation, this ) ),
	MainToggle( *new wxCheckBox( this, wxID_ANY, title, wxPoint( 8, 1 ) ) )
{
	MainToggle.SetSize( MainToggle.GetSize() + wxSize( 8, 0 ) );
	
	//Connect( 100, wxEVT_COMMAND_CHECKBOX_CLICKED, wxCommandEventHandler( CheckedStaticBox::OnMainToggleEvent ), (wxObject*)this );
}

BEGIN_EVENT_TABLE(CheckedStaticBox, wxPanel)
	EVT_CHECKBOX(wxID_ANY, MainToggle_Click)
END_EVENT_TABLE()

void CheckedStaticBox::MainToggle_Click( wxCommandEvent& evt )
{
	SetValue( evt.IsChecked() );
}

// Sets the main checkbox status, and enables/disables all child controls
// bound to the StaticBox accordingly.
void CheckedStaticBox::SetValue( bool val )
{
	wxWindowList& list = GetChildren();
	
	for( wxWindowList::iterator iter = list.begin(); iter != list.end(); ++iter)
	{
		wxWindow *current = *iter;
		if( current != &MainToggle )
			current->Enable( val );
	}
	//MainToggle.Enable();
	MainToggle.SetValue( val );
}

bool CheckedStaticBox::GetValue() const
{
	return MainToggle.GetValue();
}

wxCheckBox& wxDialogWithHelpers::AddCheckBox( wxBoxSizer& sizer, const wxString& label, wxWindowID id )
{
	return wxHelpers::AddCheckBox( this, sizer, label, id );
}

wxDialogWithHelpers::wxDialogWithHelpers( wxWindow* parent, int id,  const wxString& title, bool hasContextHelp, const wxPoint& pos, const wxSize& size ) :
	wxDialog( parent, id, title, pos, size ),
	m_hasContextHelp( hasContextHelp )
{
	if( hasContextHelp )
		wxHelpProvider::Set( new wxSimpleHelpProvider() );
}

void wxDialogWithHelpers::AddOkCancel( wxBoxSizer &sizer )
{
	wxBoxSizer* buttonSizer = &sizer;
	if( m_hasContextHelp )
	{
		// Add the context-sensitive help button on the caption for the platforms
		// which support it (currently MSW only)
		SetExtraStyle( wxDIALOG_EX_CONTEXTHELP );

#ifndef __WXMSW__
		// create a sizer to hold the help and ok/cancel buttons.
		buttonSizer = new wxBoxSizer( wxHORIZONTAL );
		buttonSizer->Add( new wxContextHelpButton(this), stdButtonSizerFlags.Align( wxALIGN_LEFT ) );
#endif
	}
	buttonSizer->Add( CreateStdDialogButtonSizer( wxOK | wxCANCEL ), wxHelpers::stdButtonSizerFlags );
}

