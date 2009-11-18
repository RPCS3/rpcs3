/*  PCSX2 - PS2 Emulator for PCs
 *  Copyright (C) 2002-2009  PCSX2 Dev Team
 *
 *  PCSX2 is free software: you can redistribute it and/or modify it under the terms
 *  of the GNU Lesser General Public License as published by the Free Software Found-
 *  ation, either version 3 of the License, or (at your option) any later version.
 *
 *  PCSX2 is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY;
 *  without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
 *  PURPOSE.  See the GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License along with PCSX2.
 *  If not, see <http://www.gnu.org/licenses/>.
 */

#include "PrecompiledHeader.h"
#include "System.h"
#include "App.h"

#include "ModalPopups.h"
#include "Utilities/StringHelpers.h"

using namespace wxHelpers;

bool ConfButtons::Allows( wxWindowID id ) const
{
	switch( id )
	{
		case wxID_OK:		return HasOK();
		case wxID_CANCEL:	return HasCancel();
		case wxID_APPLY:	return HasApply();
		case wxID_YES:		return HasYes();
		case wxID_NO:		return HasNo();
		case wxID_YESTOALL:	return HasYes() && AllowsToAll();
		case wxID_NOTOALL:	return HasNo() && AllowsToAll();

		case wxID_ABORT:	return HasAbort();
		case wxID_RETRY:	return HasRetry();
		
		// [TODO] : maybe add in an Ignore All?
		case wxID_IGNORE:	return HasIgnore();
	}
	return false;
}

static wxString ResultToString( int result )
{
	switch( result )
	{
		case wxID_OK:		return L"ok";
		case wxID_CANCEL:	return L"cancel";
		case wxID_APPLY:	return L"apply";
		case wxID_YES:		return L"yes";
		case wxID_NO:		return L"no";
		case wxID_YESTOALL:	return L"yestoall";
		case wxID_NOTOALL:	return L"notoall";

		case wxID_ABORT:	return L"abort";
		case wxID_RETRY:	return L"retry";

			// [TODO] : maybe add in an Ignore All?
		case wxID_IGNORE:	return L"ignore";
	}
	
	return wxEmptyString;
}

static wxWindowID ParseThatResult( const wxString& src, const ConfButtons& validTypes )
{
	if( !pxAssert( !src.IsEmpty() ) ) return wxID_ANY;

	static const wxWindowID retvals[] =
	{
		wxID_OK,		wxID_CANCEL,	wxID_APPLY,
		wxID_YES,		wxID_NO,
		wxID_YESTOALL,	wxID_NOTOALL,
		
		wxID_ABORT,		wxID_RETRY,		wxID_IGNORE,
	};

	for( int i=0; i<ArraySize( retvals ); ++i )
	{
		if( (validTypes.Allows( retvals[i] )) && (src == ResultToString(retvals[i])) )
			return retvals[i];
	}
	
	return wxID_ANY;
}

wxWindowID Dialogs::IssueConfirmation( wxWindow* parent, const wxString& disablerKey, const ConfButtons& type, const wxString& title, const wxString& msg )
{
	wxConfigBase* cfg = GetAppConfig();

	if( cfg != NULL ) 
	{
		cfg->SetPath( L"/PopupDisablers" );
		bool recdef = cfg->IsRecordingDefaults();
		cfg->SetRecordDefaults( false );
		
		wxString result = cfg->Read( disablerKey, L"enabled" );

		cfg->SetRecordDefaults( recdef );
		cfg->SetPath( L"/" );

		result.LowerCase();
		wxArrayString split;
		SplitString( split, result, L"," );

		// if only one param (no comma!?) then assume the entry is invalid and force a 
		// re-display of the dialog.

		if( split.Count() > 1 )
		{
			result = split[0];
			if( result == L"disabled" || result == L"off" || result == L"no" )
			{
				int result = ParseThatResult( split[1], type );
				if( result != wxID_ANY ) return result;
			}
		}
	}

	Dialogs::ExtensibleConfirmation confirmDlg( parent, type, title, msg );

	if( cfg == NULL ) return confirmDlg.ShowModal();

	// Add an option that allows the user to disable this popup from showing again.
	// (and if the config hasn't been initialized yet, then assume the dialog as non-disablable)

	pxCheckBox&	DisablerCtrl( *new pxCheckBox(&confirmDlg, _("Do not show this dialog again.")) );
	confirmDlg.GetExtensibleSizer().Add( &DisablerCtrl, wxSizerFlags().Centre() );

	if( type != ConfButtons().OK() )
		pxSetToolTip(&DisablerCtrl, _("Disables this popup and whatever response you select here will be automatically used from now on."));
	else
		pxSetToolTip(&DisablerCtrl, _("The popup will not be shown again.  This setting can be undone from the settings panels."));

	confirmDlg.Fit();

	int modalResult = confirmDlg.ShowModal();

	wxString cfgResult = ResultToString( modalResult );
	if( DisablerCtrl.IsChecked() && !cfgResult.IsEmpty() )
	{
		cfg->SetPath( L"/PopupDisablers" );
		cfg->Write( disablerKey, L"disabled," + cfgResult );
		cfg->SetPath( L"/" );
	}
	return modalResult;
}

Dialogs::ExtensibleConfirmation::ExtensibleConfirmation( wxWindow* parent, const ConfButtons& type, const wxString& title, const wxString& msg )
	: wxDialogWithHelpers( parent, wxID_ANY, title, false )
	, m_ExtensibleSizer( *new wxBoxSizer( wxVERTICAL ) )
	, m_ButtonSizer( *new wxBoxSizer( wxHORIZONTAL ) )
{
	m_idealWidth = 500;

	wxBoxSizer& mainsizer( *new wxBoxSizer(wxVERTICAL) );

	// Add the message padded some (StdCenter gives us a 5 pt padding).  Helps emphasize it a bit.
	wxBoxSizer& msgPadSizer( *new wxBoxSizer(wxVERTICAL) );
	
	msgPadSizer.Add( new pxStaticHeading( this, msg ) );
	mainsizer.Add( &msgPadSizer, pxSizerFlags::StdCenter() );

	mainsizer.Add( &m_ExtensibleSizer, wxSizerFlags().Centre() );

	// Populate the Button Sizer.
	// We prefer this over the built-in wxWidgets ButtonSizer stuff used for other types of
	// dialogs because we offer more button types, and we don't want the MSW default behavior
	// of right-justified buttons.

	if( type.HasCustom() )
		AddCustomButton( pxID_CUSTOM, type.GetCustomLabel() );
	
	// Order of wxID_RESET and custom button have been picked fairly arbitrarily, since there's
	// no standard governing those.

	#ifdef __WXGTK__
	// GTK+ / Linux inverts OK/CANCEL order -- cancel / no first, OK / Yes later. >_<
	if( type.HasCancel() )
		AddActionButton( wxID_CANCEL );

	if( type.HasNo() )
	{
		AddActionButton( wxID_NO );
		if( type.AllowsToAll() ) AddActionButton( wxID_NOTOALL );
	}
	if( type.HasOK() || type.HasYes() )			// Extra space between Affirm and Cancel Actions
		m_ButtonSizer.Add(0, 0, 1, wxEXPAND, 0);
	#endif

	if( type.HasOK() )
		AddActionButton( wxID_OK );

	if( type.HasYes() )
	{
		AddActionButton( wxID_YES );
		if( type.AllowsToAll() )
			AddActionButton( wxID_YESTOALL );
	}

	if( type.HasReset() )
		AddCustomButton( wxID_RESET, _("Reset") );

	#ifndef __WXGTK__
	if( type.HasNo() || type.HasCancel() )		// Extra space between Affirm and Cancel Actions
		m_ButtonSizer.Add(0, 0, 1, wxEXPAND, 0);

	if( type.HasNo() )
	{
		AddActionButton( wxID_NO );
		if( type.AllowsToAll() )
			AddActionButton( wxID_NOTOALL );
	}

	if( type.HasCancel() )
		AddActionButton( wxID_CANCEL );
	#endif
	
	mainsizer.Add( &m_ButtonSizer, pxSizerFlags::StdCenter() );

	SetSizerAndFit( &mainsizer, true );
	CenterOnScreen();
}

void Dialogs::ExtensibleConfirmation::OnActionButtonClicked( wxCommandEvent& evt )
{
	EndModal( evt.GetId() );
}

void Dialogs::ExtensibleConfirmation::AddCustomButton( wxWindowID id, const wxString& label )
{
	m_ButtonSizer.Add( new wxButton( this, id, label ), pxSizerFlags::StdButton() )->SetProportion( 6 );
	Connect( id, wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( ExtensibleConfirmation::OnActionButtonClicked ) );
}

void Dialogs::ExtensibleConfirmation::AddActionButton( wxWindowID id )
{
	m_ButtonSizer.Add( new wxButton( this, id ), pxSizerFlags::StdButton() )->SetProportion( 6 );
	Connect( id, wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( ExtensibleConfirmation::OnActionButtonClicked ) );
}

