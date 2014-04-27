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

#include "ps2/BiosTools.h"

#include <wx/stdpaths.h>

using namespace Dialogs;
using namespace pxSizerFlags;

// --------------------------------------------------------------------------------------
//  DocsFolderPickerPanel
// --------------------------------------------------------------------------------------
Panels::DocsFolderPickerPanel::DocsFolderPickerPanel( wxWindow* parent, bool isFirstTime )
	: BaseApplicableConfigPanel( parent, wxVERTICAL, _("Usermode Selection") )
{
	const wxString usermodeExplained( pxE( L"Please select your preferred default location for PCSX2 user-level documents below (includes memory cards, screenshots, settings, and savestates).  These folder locations can be overridden at any time using the Plugin/BIOS Selector panel."
	) );

	const wxString usermodeWarning( pxE( L"You can change the preferred default location for PCSX2 user-level documents here (includes memory cards, screenshots, settings, and savestates).  This option only affects Standard Paths which are set to use the installation default value."
	) );

	const RadioPanelItem UsermodeOptions[] = 
	{
		RadioPanelItem(
			_("User Documents (recommended)"),
			_("Location: ") + PathDefs::GetDocuments(DocsFolder_User).ToString()
		),

		RadioPanelItem(
			_("Custom folder:"),
			wxEmptyString,
			_("This setting may require administration privileges from your operating system, depending on how your system is configured.")
		)
	};

	m_radio_UserMode = new pxRadioPanel( this, UsermodeOptions );
	m_radio_UserMode->SetPaddingHoriz( m_radio_UserMode->GetPaddingVert() + 4 );
	m_radio_UserMode->Realize();
	if( pxStaticText* woot = m_radio_UserMode->GetSubText(0) ) woot->Unwrapped();		// wrapping sucks for path names!
	
	m_dirpicker_custom = new DirPickerPanel( this, FolderId_Documents, AddAppName(_("Select a document root for %s")) );

	*this	+= Heading( isFirstTime ? usermodeExplained : usermodeWarning );
	*this	+= m_radio_UserMode		| StdExpand();
	*this	+= m_dirpicker_custom	| pxExpand.Border( wxLEFT, StdPadding + m_radio_UserMode->GetIndentation() );
	*this	+= 4;

	Connect( wxEVT_COMMAND_RADIOBUTTON_SELECTED, wxCommandEventHandler(DocsFolderPickerPanel::OnRadioChanged) );
}

DocsModeType Panels::DocsFolderPickerPanel::GetDocsMode() const
{
	return (DocsModeType) m_radio_UserMode->GetSelection();
}

void Panels::DocsFolderPickerPanel::Apply()
{
	DocsFolderMode			= (DocsModeType) m_radio_UserMode->GetSelection();
	CustomDocumentsFolder	= m_dirpicker_custom->GetPath();
}

void Panels::DocsFolderPickerPanel::AppStatusEvent_OnSettingsApplied()
{
	if( m_radio_UserMode ) m_radio_UserMode->SetSelection( DocsFolderMode );
	if( m_dirpicker_custom ) m_dirpicker_custom->Enable( DocsFolderMode == DocsFolder_Custom );
}

void Panels::DocsFolderPickerPanel::OnRadioChanged( wxCommandEvent& evt )
{
	evt.Skip();

	if( !m_radio_UserMode ) return;

	if( m_dirpicker_custom )
		m_dirpicker_custom->Enable( m_radio_UserMode->GetSelection() == (int)DocsFolder_Custom );

	if( pxStaticText* woot = m_radio_UserMode->GetSubText(0) )
		woot->Enable( m_radio_UserMode->GetSelection() == (int)DocsFolder_User );
}

// --------------------------------------------------------------------------------------
//  LanguageSelectionPanel
// --------------------------------------------------------------------------------------
Panels::LanguageSelectionPanel::LanguageSelectionPanel( wxWindow* parent )
	: BaseApplicableConfigPanel( parent, wxHORIZONTAL )
{
	m_picker = NULL;
	i18n_EnumeratePackages( m_langs );

	int size = m_langs.size();
	ScopedArray<wxString> compiled( size );

	for( int i=0; i<size; ++i )
		compiled[i].Printf( L"%s", m_langs[i].englishName.c_str() );

	m_picker = new wxComboBox( this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize,
		size, compiled.GetPtr(), wxCB_READONLY | wxCB_SORT );

	wxButton* applyButton = new wxButton( this, pxID_RestartWizard, _("Apply") );
	applyButton->SetToolTip(_("Make this language my default right now!"));

	*this	+= 5;
	*this	+= m_picker | pxSizerFlags::StdSpace();
	*this	+= 5;
	*this	+= applyButton | pxSizerFlags::StdSpace();
	*this	+= 5;

	Connect( pxID_RestartWizard,	wxEVT_COMMAND_BUTTON_CLICKED,	wxCommandEventHandler( LanguageSelectionPanel::OnApplyLanguage_Clicked ) );

	m_picker->SetSelection( 0 );		// always default to System Default
}

void Panels::LanguageSelectionPanel::OnApplyLanguage_Clicked( wxCommandEvent& evt )
{
	evt.Skip();
	Apply();
}

void Panels::LanguageSelectionPanel::Apply()
{
	if( !m_picker ) return;

	// The combo box's order is sorted and may not match our m_langs order, so
	// we have to do a string comparison to find a match:

	wxString sel( m_picker->GetString( m_picker->GetSelection() ) );

	g_Conf->LanguageCode = L"default";	// use this if no matches found
	g_Conf->LanguageId = wxLANGUAGE_DEFAULT; 
	int size = m_langs.size();
	for( int i=0; i<size; ++i )
	{
		if( m_langs[i].englishName == sel )
		{
			if( i18n_SetLanguage( m_langs[i].wxLangId, m_langs[i].canonicalName ) )
			{
				g_Conf->LanguageCode = m_langs[i].canonicalName;
				g_Conf->LanguageId = m_langs[i].wxLangId;
			}
			break;
		}
	}

}

void Panels::LanguageSelectionPanel::AppStatusEvent_OnSettingsApplied()
{
	if (m_picker)
	{
		m_picker->SetSelection( 0 );		// always default to System Default

		if (g_Conf->LanguageCode.IsEmpty())
			g_Conf->LanguageCode = L"default";

		for (uint i=0; i<m_langs.size(); ++i)
		{
			if (0==m_langs[i].canonicalName.CmpNoCase(g_Conf->LanguageCode))
			{
				m_picker->SetSelection( i );
			}
		}
	}
}
