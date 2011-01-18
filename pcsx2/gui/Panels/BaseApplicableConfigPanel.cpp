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
#include "App.h"

#include "ConfigurationPanels.h"
#include "Dialogs/ConfigurationDialog.h"

#include <wx/bookctrl.h>

using namespace Dialogs;

// -----------------------------------------------------------------------
// This method should be called by the parent dialog box of a configuration
// on dialog destruction.  It asserts if the ApplyList hasn't been cleaned up
// and then cleans it up forcefully.
//
void ApplyStateStruct::DoCleanup() throw()
{
	pxAssertMsg( PanelList.size() != 0, L"PanelList list hasn't been cleaned up." );
	PanelList.clear();
	ParentBook = NULL;
}

void ApplyStateStruct::StartBook( wxBookCtrlBase* book )
{
	pxAssertDev( ParentBook == NULL, "An ApplicableConfig session is already in progress." );
	ParentBook = book;
}

void ApplyStateStruct::StartWizard()
{
	pxAssertDev( ParentBook == NULL, "An ApplicableConfig session is already in progress." );
}

// -----------------------------------------------------------------------
//
// Parameters:
//  pageid - identifier of the page to apply settings for.  All other pages will be
//     skipped.  If pageid is negative (-1) then all pages are applied.
//
// Returns false if one of the panels fails input validation (in which case dialogs
// should not be closed, etc).
//
bool ApplyStateStruct::ApplyPage( int pageid )
{
	bool retval = true;

	// Save these settings so we can restore them if the Apply fails.

	DocsModeType	oldDocsMode			= DocsFolderMode;
	wxDirName		oldSettingsFolder	= SettingsFolder;
	bool			oldUseDefSet		= UseDefaultSettingsFolder;

	AppConfig confcopy( *g_Conf );

	try
	{
		PanelApplyList_t::iterator yay = PanelList.begin();
		while( yay != PanelList.end() )
		{
			//DbgCon.Status( L"Writing settings for: " + (*yay)->GetLabel() );
			if( (pageid < 0) || (*yay)->IsOnPage( pageid ) )
				(*yay)->Apply();
			yay++;
		}

		// If an exception is thrown above, this code below won't get run.
		// (conveniently skipping any option application! :D)

		// Note: apply first, then save -- in case the apply fails.

		AppApplySettings( &confcopy );
	}
	catch( Exception::CannotApplySettings& ex )
	{
		DocsFolderMode = oldDocsMode;
		SettingsFolder = oldSettingsFolder;
		UseDefaultSettingsFolder = oldUseDefSet;
		*g_Conf = confcopy;

		if( ex.IsVerbose )
		{
			Msgbox::Alert( ex.FormatDisplayMessage(), _("Cannot apply settings...") );

			if( ex.GetPanel() != NULL )
				ex.GetPanel()->SetFocusToMe();
		}

		retval = false;
	}
	catch( ... )
	{
		DocsFolderMode = oldDocsMode;
		SettingsFolder = oldSettingsFolder;
		UseDefaultSettingsFolder = oldUseDefSet;
		*g_Conf = confcopy;

		throw;
	}

	return retval;
}

// Returns false if one of the panels fails input validation (in which case dialogs
// should not be closed, etc).
bool ApplyStateStruct::ApplyAll()
{
	return ApplyPage( -1 );
}

// --------------------------------------------------------------------------------------
//  BaseApplicableConfigPanel Implementations
// --------------------------------------------------------------------------------------
IApplyState* BaseApplicableConfigPanel::FindApplyStateManager() const
{
	wxWindow* millrun = this->GetParent();
	while( millrun != NULL )
	{
		if( BaseApplicableDialog* dialog = wxDynamicCast( millrun, BaseApplicableDialog ) )
			return (IApplyState*)dialog;

		if( ApplicableWizardPage* wizpage = wxDynamicCast( millrun, ApplicableWizardPage ) )
			return (IApplyState*)wizpage;

		millrun = millrun->GetParent();
	}
	return NULL;
}

BaseApplicableConfigPanel::~BaseApplicableConfigPanel() throw()
{
	if( IApplyState* iapp = FindApplyStateManager() )
		iapp->GetApplyState().PanelList.remove( this );
}

BaseApplicableConfigPanel::BaseApplicableConfigPanel( wxWindow* parent, wxOrientation orient )
	: wxPanelWithHelpers( parent, orient )
	, m_AppStatusHelper( this )
{
	Init();
}

BaseApplicableConfigPanel::BaseApplicableConfigPanel( wxWindow* parent, wxOrientation orient, const wxString& staticLabel )
	: wxPanelWithHelpers( parent, orient, staticLabel )
	, m_AppStatusHelper( this )
{
	Init();
}

void BaseApplicableConfigPanel::SetFocusToMe()
{
	if( (m_OwnerBook == NULL) || (m_OwnerPage == wxID_NONE) ) return;
	m_OwnerBook->SetSelection( m_OwnerPage );
}

void BaseApplicableConfigPanel::Init()
{
	// We need to bind to an event that occurs *after* all window and child
	// window mess has been created.  Unfortunately the WindowCreate event handler
	// is immediate, and depends on the platform for how it "works", and thus
	// useless.  Solution: Create our own! :)

	//Connect( wxEVT_CREATE,	wxWindowCreateEventHandler	(BaseApplicableConfigPanel::OnCreateWindow) );
	Connect( pxEvt_ApplySettings,	wxCommandEventHandler	(BaseApplicableConfigPanel::OnSettingsApplied) );

	if( IApplyState* iapp = FindApplyStateManager() )
	{
		ApplyStateStruct& applyState( iapp->GetApplyState() );
		m_OwnerPage = applyState.CurOwnerPage;
		m_OwnerBook = applyState.ParentBook;
		applyState.PanelList.push_back( this );
	}

	wxCommandEvent applyEvent( pxEvt_ApplySettings );
	applyEvent.SetId( GetId() );
	AddPendingEvent( applyEvent );
}

void BaseApplicableConfigPanel::OnSettingsApplied( wxCommandEvent& evt )
{
	evt.Skip();
	if( evt.GetId() == GetId() ) AppStatusEvent_OnSettingsApplied();
}

void BaseApplicableConfigPanel::AppStatusEvent_OnSettingsApplied() {}

BaseApplicableConfigPanel_SpecificConfig::BaseApplicableConfigPanel_SpecificConfig(wxWindow* parent, wxOrientation orient)
	: BaseApplicableConfigPanel( parent, orient )
{
}

BaseApplicableConfigPanel_SpecificConfig::BaseApplicableConfigPanel_SpecificConfig(wxWindow* parent, wxOrientation orient, const wxString& staticLabel )
	: BaseApplicableConfigPanel( parent, orient, staticLabel )
{
}

