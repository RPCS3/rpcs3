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

#include "ConfigurationDialog.h"
#include "BaseConfigurationDialog.inl"
#include "ModalPopups.h"
#include "MSWstuff.h"

#include "Panels/ConfigurationPanels.h"
#include "Panels/MemoryCardPanels.h"

using namespace pxSizerFlags;

wxString GetMsg_McdNtfsCompress()
{
	return pxE( "!Panel:Mcd:NtfsCompress", 
		L"NTFS compression is built-in, fast, and completely reliable; and typically compresses memory cards "
		L"very well (this option is highly recommended)."
	);
}

Panels::McdConfigPanel_Toggles::McdConfigPanel_Toggles(wxWindow *parent)
	: _parent( parent )
{
	m_check_Ejection = new pxCheckBox( this,
		_("Auto-eject memory cards when loading savestates"),
		pxE( "!Panel:Mcd:EnableEjection",
			L"Avoids memory card corruption by forcing games to re-index card contents after "
			L"loading from savestates.  May not be compatible with all games (Guitar Hero)."
		)
	);

	m_check_SavestateBackup = new pxCheckBox( this, pxsFmt(_("Backup existing Savestate when creating a new one")) );

	for( uint i=0; i<2; ++i )
	{
		m_check_Multitap[i] = new pxCheckBox( this, pxsFmt(_("Enable Multitap on Port %u"), i+1) );
		m_check_Multitap[i]->SetClientData( (void*)i );
		m_check_Multitap[i]->SetName(pxsFmt( L"CheckBox::Multitap%u", i ));
	}

	// ------------------------------
	//   Sizers and Layout Section
	// ------------------------------

	for( uint i=0; i<2; ++i )
		*this += m_check_Multitap[i];	
		
	*this += 4;

	*this += m_check_SavestateBackup;

	*this += 4;

	*this += m_check_Ejection;	
}

void Panels::McdConfigPanel_Toggles::Apply()
{
	g_Conf->EmuOptions.MultitapPort0_Enabled	= m_check_Multitap[0]->GetValue();
	g_Conf->EmuOptions.MultitapPort1_Enabled	= m_check_Multitap[1]->GetValue();

	g_Conf->EmuOptions.BackupSavestate			= m_check_SavestateBackup->GetValue();
	g_Conf->EmuOptions.McdEnableEjection		= m_check_Ejection->GetValue();
}

void Panels::McdConfigPanel_Toggles::AppStatusEvent_OnSettingsApplied()
{
	m_check_Multitap[0]		->SetValue( g_Conf->EmuOptions.MultitapPort0_Enabled );
	m_check_Multitap[1]		->SetValue( g_Conf->EmuOptions.MultitapPort1_Enabled );

	m_check_SavestateBackup ->SetValue( g_Conf->EmuOptions.BackupSavestate );
	m_check_Ejection		->SetValue( g_Conf->EmuOptions.McdEnableEjection );
}


using namespace Panels;
using namespace pxSizerFlags;

Dialogs::McdConfigDialog::McdConfigDialog( wxWindow* parent )
	: BaseConfigurationDialog( parent, _("MemoryCard Manager"), 600 )
{
	m_panel_mcdlist	= new MemoryCardListPanel_Simple( this );
	
	// [TODO] : Plan here is to add an advanced tab which gives the user the ability
	// to configure the names of each memory card slot.

	//AddPage<McdConfigPanel_Toggles>		( wxLt("Settings"),		cfgid.MemoryCard );
	//AddPage<McdConfigPanel_Standard>	( wxLt("Slots 1/2"),	cfgid.MemoryCard );

	*this	+= Heading(_("Drag items over other items in the list to swap or copy memory cards."))	| StdExpand();
	*this	+= StdPadding;

	*this	+= m_panel_mcdlist			| StdExpand();
	//*this	+= StdPadding;
	*this	+= new wxStaticLine( this )	| StdExpand();
	*this	+= StdPadding;
	*this	+= new McdConfigPanel_Toggles( this )	| StdExpand();

	for( uint i=0; i<2; ++i )
	{
		if( pxCheckBox* check = (pxCheckBox*)FindWindow(pxsFmt( L"CheckBox::Multitap%u", i )) )
			Connect( check->GetId(), wxEVT_COMMAND_CHECKBOX_CLICKED, wxCommandEventHandler(McdConfigDialog::OnMultitapClicked) );
	}

	AddOkCancel();
}

void Dialogs::McdConfigDialog::OnMultitapClicked( wxCommandEvent& evt )
{
	evt.Skip();
	if( !m_panel_mcdlist ) return;

	if( pxCheckBox* box = (pxCheckBox*)evt.GetEventObject() )
		m_panel_mcdlist->SetMultitapEnabled( (int)box->GetClientData(), box->IsChecked() );
}

bool Dialogs::McdConfigDialog::Show( bool show )
{
	if( show && m_panel_mcdlist )
		m_panel_mcdlist->OnShown();

	return _parent::Show( show );
}

int Dialogs::McdConfigDialog::ShowModal()
{
	if( m_panel_mcdlist )
		m_panel_mcdlist->OnShown();

	return _parent::ShowModal();
}
