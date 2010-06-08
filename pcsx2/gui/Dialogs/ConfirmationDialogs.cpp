/*  PCSX2 - PS2 Emulator for PCs
 *  Copyright (C) 2002-2010  PCSX2 Dev Team
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

using namespace pxSizerFlags;

bool MsgButtons::Allows( wxWindowID id ) const
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
		
		case wxID_RESET:	return HasReset();
		case wxID_CLOSE:	return HasClose();
	}

	if (id <= wxID_LOWEST)
		return HasCustom();

	return false;
}

static wxString ResultToString( int result, const MsgButtons& buttons )
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

		case wxID_RESET:	return L"reset";
		case wxID_CLOSE:	return L"close";
	}

	if (result <= wxID_LOWEST)
		return buttons.GetCustomLabel();

	return wxEmptyString;
}

static wxWindowID ParseThatResult( const wxString& src, const MsgButtons& validTypes )
{
	if( !pxAssert( !src.IsEmpty() ) ) return wxID_ANY;

	static const wxWindowID retvals[] =
	{
		wxID_OK,		wxID_CANCEL,	wxID_APPLY,
		wxID_YES,		wxID_NO,
		wxID_YESTOALL,	wxID_NOTOALL,

		wxID_ABORT,		wxID_RETRY,		wxID_IGNORE,

		wxID_RESET,
		wxID_ANY,
	};

	for( int i=0; i<ArraySize( retvals ); ++i )
	{
		if( (validTypes.Allows( retvals[i] )) && (src == ResultToString(retvals[i], validTypes)) )
			return retvals[i];
	}

	return wxID_NONE;
}

static bool pxTrySetFocus( wxWindow& parent, wxWindowID id )
{
	if( wxWindow* found = parent.FindWindowById( id ) )
	{
		found->SetFocus();
		return true;
	}

	return false;
}

static bool pxTrySetFocus( wxWindow* parent, wxWindowID id )
{
	if( parent == NULL ) return false;
	pxTrySetFocus( *parent, id );
}

void MsgButtons::SetBestFocus( wxWindow& dialog ) const
{
	if( HasOK()			&& pxTrySetFocus( dialog, wxID_OK ) ) return;
	if( HasNo()			&& pxTrySetFocus( dialog, wxID_NO ) ) return;
	if( HasClose()		&& pxTrySetFocus( dialog, wxID_CLOSE ) ) return;
	if( HasRetry()		&& pxTrySetFocus( dialog, wxID_RETRY ) ) return;

	// Other confirmational types of buttons must be explicitly focused by the user or
	// by an implementing dialog.  We won't do it here implicitly because accidental
	// "on focus" typed keys could invoke really unwanted actions.

	// (typically close/ok/retry/etc. aren't so bad that accidental clicking does terrible things)
}

void MsgButtons::SetBestFocus( wxWindow* dialog ) const
{
	if( dialog == NULL ) return;
	SetBestFocus( *dialog );
}


wxWindowID pxIssueConfirmation( wxDialogWithHelpers& confirmDlg, const MsgButtons& buttons )
{
	if( confirmDlg.GetMinWidth() <= 0 ) confirmDlg.SetMinWidth( 400 );

	confirmDlg += new ModalButtonPanel( &confirmDlg, buttons ) | pxCenter.Border( wxTOP, 8 );
	buttons.SetBestFocus( confirmDlg );
	return confirmDlg.ShowModal();
}


wxWindowID pxIssueConfirmation( wxDialogWithHelpers& confirmDlg, const MsgButtons& buttons, const wxString& disablerKey )
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
				int result = ParseThatResult( split[1], buttons );
				if( result != wxID_ANY ) return result;
			}
		}
	}

	pxCheckBox*	DisablerCtrl = NULL;
	if( cfg != NULL )
	{
		// Add an option that allows the user to disable this popup from showing again.
		// (and if the config hasn't been initialized yet, then assume the dialog as non-disablable)

		DisablerCtrl = new pxCheckBox(&confirmDlg, _("Do not show this dialog again."));

		confirmDlg += 8;
		confirmDlg += DisablerCtrl | wxSF.Centre();

		if( buttons != MsgButtons().OK() )
			pxSetToolTip(DisablerCtrl, _("Disables this popup and whatever response you select here will be automatically used from now on."));
		else
			pxSetToolTip(DisablerCtrl, _("The popup will not be shown again.  This setting can be undone from the settings panels."));

	}

	int modalResult = pxIssueConfirmation( confirmDlg, buttons );

	if( cfg != NULL )
	{
		wxString cfgResult = ResultToString( modalResult, buttons );
		if( DisablerCtrl->IsChecked() && !cfgResult.IsEmpty() )
		{
			cfg->SetPath( L"/PopupDisablers" );
			cfg->Write( disablerKey, L"disabled," + cfgResult );
			cfg->SetPath( L"/" );
			cfg->Flush();
		}
	}
	return modalResult;
}

ModalButtonPanel::ModalButtonPanel( wxWindow* parent, const MsgButtons& buttons )
	: wxPanelWithHelpers( parent, wxHORIZONTAL )
{
	// Populate the Button Sizer.
	// We prefer this over the built-in wxWidgets ButtonSizer stuff used for other types of
	// dialogs because we offer more button types, and we don't want the MSW default behavior
	// of right-justified buttons.

	if( buttons.HasCustom() )
		AddCustomButton( pxID_CUSTOM, buttons.GetCustomLabel() );

	// Order of wxID_RESET and custom button have been picked fairly arbitrarily, since there's
	// no standard governing those.

#ifdef __WXGTK__
	// GTK+ / Linux inverts OK/CANCEL order -- cancel / no first, OK / Yes later. >_<
	if( buttons.HasCancel() )
		AddActionButton( wxID_CANCEL );

	if( buttons.HasNo() )
	{
		AddActionButton( wxID_NO );
		if( buttons.AllowsToAll() ) AddActionButton( wxID_NOTOALL );
	}

	if( buttons.HasIgnore() )
		AddCustomButton( wxID_IGNORE, _("Ignore") );

	if( buttons.HasOK() || buttons.HasYes() )			// Extra space between Affirm and Cancel Actions
		GetSizer()->Add(0, 0, 1, wxEXPAND, 0);
#endif

	if( buttons.HasOK() )
		AddActionButton( wxID_OK );

	if( buttons.HasYes() )
	{
		AddActionButton( wxID_YES );
		if( buttons.AllowsToAll() )
			AddActionButton( wxID_YESTOALL );
	}

#ifdef __WXGTK__
	if( buttons.HasRetry() )
		AddActionButton( wxID_RETRY );

	if( buttons.HasAbort() )
		AddActionButton( wxID_ABORT );
#else
	if( buttons.HasAbort() )
		AddActionButton( wxID_ABORT );

	if( buttons.HasRetry() )
		AddActionButton( wxID_RETRY );
#endif

	if( buttons.HasReset() )
		AddCustomButton( wxID_RESET, _("Reset") );

	if( buttons.HasClose() )
		AddActionButton( wxID_CLOSE );

#ifndef __WXGTK__
	if( buttons.HasNo() )
	{
		AddActionButton( wxID_NO );
		if( buttons.AllowsToAll() )
			AddActionButton( wxID_NOTOALL );
	}

	if( buttons.HasIgnore() )
		AddCustomButton( wxID_IGNORE, _("Ignore") );

	if( buttons.HasCancel() )
		AddActionButton( wxID_CANCEL );
#endif
}

void ModalButtonPanel::OnActionButtonClicked( wxCommandEvent& evt )
{
	evt.Skip();
	wxWindow* toplevel = wxGetTopLevelParent( this );
	if( wxDialog* dialog = wxDynamicCast( toplevel, wxDialog ) )
		dialog->EndModal( evt.GetId() );
}

void ModalButtonPanel::AddCustomButton( wxWindowID id, const wxString& label )
{
	*this += new wxButton( this, id, label ) | StdButton().Proportion(6);
	Connect( id, wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( ModalButtonPanel::OnActionButtonClicked ) );
}

// This is for buttons that are defined internally by wxWidgets, such as wxID_CANCEL, wxID_ABORT, etc.
// wxWidgets will assign the labels and stuff for us. :D
void ModalButtonPanel::AddActionButton( wxWindowID id )
{
	*this += new wxButton( this, id ) | StdButton().Proportion(6);
	Connect( id, wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( ModalButtonPanel::OnActionButtonClicked ) );
}

