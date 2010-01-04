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
#include "App.h"
#include "ConfigurationPanels.h"

#include "Dialogs/ConfigurationDialog.h"

#include "ps2/BiosTools.h"

#include <wx/stdpaths.h>
#include <wx/bookctrl.h>

using namespace Dialogs;

// -----------------------------------------------------------------------
// This method should be called by the parent dalog box of a configuration
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

	bool		oldAdminMode		= UseAdminMode;
	wxDirName	oldSettingsFolder	= SettingsFolder;
	bool		oldUseDefSet		= UseDefaultSettingsFolder;

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
		UseAdminMode = oldAdminMode;
		SettingsFolder = oldSettingsFolder;
		UseDefaultSettingsFolder = oldUseDefSet;
		*g_Conf = confcopy;

		if( ex.IsVerbose )
		{
			wxMessageBox( ex.FormatDisplayMessage(), _("Cannot apply settings...") );

			if( ex.GetPanel() != NULL )
				ex.GetPanel()->SetFocusToMe();
		}

		retval = false;
	}
	catch( ... )
	{
		UseAdminMode = oldAdminMode;
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
	, m_Listener_SettingsApplied( wxGetApp().Source_SettingsApplied(), EventListener<int>( this, OnSettingsApplied ) )
{
	Init();
}

BaseApplicableConfigPanel::BaseApplicableConfigPanel( wxWindow* parent, wxOrientation orient, const wxString& staticLabel )
	: wxPanelWithHelpers( parent, orient, staticLabel )
	, m_Listener_SettingsApplied( wxGetApp().Source_SettingsApplied(), EventListener<int>( this, OnSettingsApplied ) )
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
	if( IApplyState* iapp = FindApplyStateManager() )
	{
		ApplyStateStruct& applyState( iapp->GetApplyState() );
		m_OwnerPage = applyState.CurOwnerPage;
		m_OwnerBook = applyState.ParentBook;
		applyState.PanelList.push_back( this );
	}
}

void __evt_fastcall BaseApplicableConfigPanel::OnSettingsApplied( void* obj, int& ini )
{
	if( obj == NULL ) return;
	((BaseApplicableConfigPanel*)obj)->OnSettingsChanged();
}


// -----------------------------------------------------------------------
Panels::UsermodeSelectionPanel::UsermodeSelectionPanel( wxWindow* parent, bool isFirstTime )
	: BaseApplicableConfigPanel( parent, wxVERTICAL, _("Usermode Selection") )
{
	const wxString usermodeExplained( pxE( ".Panels:Usermode:Explained",
		L"Please select your preferred default location for PCSX2 user-level documents below "
		L"(includes memory cards, screenshots, settings, and savestates).  "
		L"These folder locations can be overridden at any time using the Core Settings panel."
	) );

	const wxString usermodeWarning( pxE( ".Panels:Usermode:Warning",
		L"You can change the preferred default location for PCSX2 user-level documents here "
		L"(includes memory cards, screenshots, settings, and savestates).  "
		L"This option only affects Standard Paths which are set to use the installation default value."
	) );

	const RadioPanelItem UsermodeOptions[] = 
	{
		RadioPanelItem(
			_("User Documents (recommended)"),
			_("Location: ") + wxStandardPaths::Get().GetDocumentsDir()
		),

		RadioPanelItem(
			_("Current working folder (intended for developer use only)"),
			_("Location: ") + wxGetCwd(),
			_("This setting requires administration privileges from your operating system.")
		),
	};
	
	m_radio_UserMode = new pxRadioPanel( this, UsermodeOptions );
	m_radio_UserMode->SetPaddingHoriz( m_radio_UserMode->GetPaddingHoriz() + 4 );
	m_radio_UserMode->Realize();

	*this	+= Text( (isFirstTime ? usermodeExplained : usermodeWarning) );
	*this	+= m_radio_UserMode | pxSizerFlags::StdExpand();
	*this	+= 4;

	OnSettingsChanged();
}

void Panels::UsermodeSelectionPanel::Apply()
{
	UseAdminMode = (m_radio_UserMode->GetSelection() == 1);
}

void Panels::UsermodeSelectionPanel::OnSettingsChanged()
{
	m_radio_UserMode->SetSelection( (int)UseAdminMode );
}

// -----------------------------------------------------------------------
Panels::LanguageSelectionPanel::LanguageSelectionPanel( wxWindow* parent )
	: BaseApplicableConfigPanel( parent, wxHORIZONTAL )
	, m_langs()
{
	m_picker = NULL;
	i18n_EnumeratePackages( m_langs );

	int size = m_langs.size();
	int cursel = 0;
	ScopedArray<wxString> compiled( size ); //, L"Compiled Language Names" );
	wxString configLangName( wxLocale::GetLanguageName( wxLANGUAGE_DEFAULT ) );

	for( int i=0; i<size; ++i )
	{
		compiled[i].Printf( L"%s", m_langs[i].englishName.c_str() ); //, xltNames[i].c_str() );
		if( m_langs[i].englishName == configLangName )
			cursel = i;
	}

	m_picker = new wxComboBox( this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize,
		size, compiled.GetPtr(), wxCB_READONLY | wxCB_SORT );

	*this	+= Text(_("Select a language: ")) | pxMiddle;
	*this	+= 5;
	*this	+= m_picker | pxSizerFlags::StdSpace();

	m_picker->SetSelection( cursel );
	//OnSettingsChanged();
}

void Panels::LanguageSelectionPanel::Apply()
{
	// The combo box's order is sorted and may not match our m_langs order, so
	// we have to do a string comparison to find a match:

	wxString sel( m_picker->GetString( m_picker->GetSelection() ) );

	g_Conf->LanguageId = wxLANGUAGE_DEFAULT;	// use this if no matches found
	int size = m_langs.size();
	for( int i=0; i<size; ++i )
	{
		if( m_langs[i].englishName == sel )
		{
			g_Conf->LanguageId = m_langs[i].wxLangId;
			break;
		}
	}
}

void Panels::LanguageSelectionPanel::OnSettingsChanged()
{
	m_picker->SetSelection( g_Conf->LanguageId );
}
