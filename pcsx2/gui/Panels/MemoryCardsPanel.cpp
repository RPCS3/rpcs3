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
#include "ConfigurationPanels.h"
#include "Dialogs/ConfigurationDialog.h"

#include <wx/filepicker.h>
#include <wx/ffile.h>

using namespace pxSizerFlags;

wxFilePickerCtrl* CreateMemoryCardFilePicker( wxWindow* parent, uint portidx, uint slotidx, const wxString& filename=wxEmptyString )
{
	return new wxFilePickerCtrl( parent, wxID_ANY, filename,
		wxsFormat(_("Select memorycard for Port %u / Slot %u"), portidx+1, slotidx+1),	// picker window title
		L"*.ps2",	// default wildcard
		wxDefaultPosition, wxDefaultSize,
		wxFLP_DEFAULT_STYLE & ~wxFLP_FILE_MUST_EXIST
	);

}

// --------------------------------------------------------------------------------------
//  SingleCardPanel Implementations
// --------------------------------------------------------------------------------------
Panels::MemoryCardsPanel::SingleCardPanel::SingleCardPanel( wxWindow* parent, uint portidx, uint slotidx )
	: BaseApplicableConfigPanel( parent, wxVERTICAL /*, wxsFormat(_("Port %u / Slot %u"), portidx+1, slotidx+1)*/ )
{
	m_port = portidx;
	m_slot = slotidx;

	m_filepicker = CreateMemoryCardFilePicker( this, portidx, slotidx );

	m_check_Disable		= new wxCheckBox	( this, wxID_ANY, _("Disable this card") );
	m_button_Recreate	= new wxButton		( this, wxID_ANY, _("Re-create") );
	m_label_Status		= new wxStaticText	( this, wxID_ANY, _("Status: ") );
	
	pxSetToolTip( m_check_Disable, pxE( ".Tooltip:MemoryCard:Enable",
		L"When disabled, the card slot is reported as being empty to the emulator."
	) );
	
	pxSetToolTip( m_button_Recreate, pxE( ".Tooltip:MemoryCard:Recreate",
		L"Deletes the existing memory card and creates a new one.  All existing card contents will be lost."
	) );

	wxFlexGridSizer& s_status( *new wxFlexGridSizer( 4 ) );
	s_status.AddGrowableCol( 1 );

	s_status += StdPadding;
	s_status += m_label_Status		| pxMiddle;
	s_status += m_button_Recreate;
	s_status += StdPadding;

	*this += m_check_Disable	| pxBorder( wxLEFT, StdPadding );
	*this += m_filepicker		| StdExpand();
	*this += s_status			| pxExpand;

	Connect( m_filepicker->GetId(),			wxEVT_COMMAND_FILEPICKER_CHANGED,	wxCommandEventHandler(SingleCardPanel::OnFileChanged) );
	Connect( m_button_Recreate->GetId(),	wxEVT_COMMAND_BUTTON_CLICKED,		wxCommandEventHandler(SingleCardPanel::OnRecreate_Clicked) );

	AppStatusEvent_OnSettingsApplied();
}

void Panels::MemoryCardsPanel::SingleCardPanel::OnFileChanged( wxCommandEvent& evt )
{
	if( UpdateStatusLine(m_filepicker->GetPath()) ) evt.Skip();
}

void Panels::MemoryCardsPanel::SingleCardPanel::OnRecreate_Clicked( wxCommandEvent& evt )
{
	if( !wxFileName( m_filepicker->GetPath() ).IsOk() ) return;
	Dialogs::CreateMemoryCardDialog( this, m_port, m_slot, m_filepicker->GetPath() ).ShowModal();
}

void Panels::MemoryCardsPanel::SingleCardPanel::Apply()
{
	AppConfig::McdOptions& mcd( g_Conf->Mcd[m_port][m_slot] );

	//mcd.Enabled		= m_check_Disable->GetValue();
	//mcd.Filename	= m_filepicker->GetPath();
}

void Panels::MemoryCardsPanel::SingleCardPanel::AppStatusEvent_OnSettingsApplied()
{
	const AppConfig::McdOptions& mcd( g_Conf->Mcd[m_port][m_slot] );
	const wxString mcdFilename( g_Conf->FullpathToMcd( m_port, m_slot ) );

	m_filepicker->SetPath( mcdFilename );
	UpdateStatusLine( mcdFilename );
	m_check_Disable->SetValue( !mcd.Enabled );
}

bool Panels::MemoryCardsPanel::SingleCardPanel::UpdateStatusLine( const wxFileName& mcdfilename )
{
	if( !mcdfilename.IsOk() )
	{
		m_label_Status->SetLabel(_("Invalid filename or path."));
		m_button_Recreate->SetLabel(_("Create"));
		m_button_Recreate->Disable();

		return false;
	}
	
	m_button_Recreate->Enable();
	
	if( !mcdfilename.FileExists() )
	{
		m_label_Status->SetLabel(_("Status: File does not exist."));
		m_button_Recreate->SetLabel(_("Create"));
		
		return false;
	}
	else
	{
		m_button_Recreate->SetLabel(_("Re-create"));

		// TODO: Add formatted/unformatted check here.
		
		wxFFile mcdFile( mcdfilename.GetFullPath() );
		if( !mcdFile.IsOpened() )
		{
			m_label_Status->SetLabel(_("Status: Permission denied."));
		}
		else
		{
			const uint size = (uint)(mcdFile.Length() / (1024 * 528 * 2));
			m_label_Status->SetLabel( wxsFormat(_("Status: %u MB"), size) );
		}

		return true;
	}
	
}

// --------------------------------------------------------------------------------------
//  MemoryCardsPanel Implementations
// --------------------------------------------------------------------------------------
Panels::MemoryCardsPanel::MemoryCardsPanel( wxWindow* parent )
	: BaseApplicableConfigPanel( parent )
{
	wxPanelWithHelpers* columns[2];

	m_idealWidth -= 48;
	m_check_Ejection = new pxCheckBox( this,
		_("Auto-eject memorycards when loading savestates"),
		pxE( ".Dialog:Memorycards:EnableEjection",
			L"Avoids memorycard corruption by forcing games to re-index card contents after "
			L"loading from savestates.  May not be compatible with all games (Guitar Hero)."
		)
	);

	m_idealWidth += 48;

	for( uint port=0; port<2; ++port )
	{
		columns[port] = new wxPanelWithHelpers( this, wxVERTICAL );
		columns[port]->SetIdealWidth( (columns[port]->GetIdealWidth()-12) / 2 );

		m_check_Multitap[port] = new pxCheckBox( columns[port], wxsFormat(_("Enable Multitap on Port %u"), port+1) );
		m_check_Multitap[port]->SetClientData( (void*) port );

		for( uint slot=0; slot<4; ++slot )
		{
			m_CardPanel[port][slot] = new SingleCardPanel( columns[port], port, slot );
		}

		Connect( m_check_Multitap[port]->GetId(), wxEVT_COMMAND_CHECKBOX_CLICKED, wxCommandEventHandler(MemoryCardsPanel::OnMultitapChecked));
	}

	// ------------------------------------
	//       Sizer / Layout Section
	// ------------------------------------

	wxFlexGridSizer* s_table = new wxFlexGridSizer( 2 );
	s_table->AddGrowableCol( 0 );
	s_table->AddGrowableCol( 1 );

	for( uint port=0; port<2; ++port )
	{
		wxStaticBoxSizer& portSizer( *new wxStaticBoxSizer( wxVERTICAL, columns[port], wxsFormat(_("Port %u"), port+1) ) );

		*columns[port] += portSizer | SubGroup();

		for( uint slot=0; slot<4; ++slot )
		{
			portSizer += m_CardPanel[port][slot] | pxExpand;
			
			if( slot == 0 )
			{
				portSizer += new wxStaticLine( columns[port] )	| pxExpand.Border( wxTOP, StdPadding );
				portSizer += m_check_Multitap[port]				| pxCenter.Border( wxBOTTOM, StdPadding );
			}
		}

		*s_table += columns[port] | StdExpand();
	}
	

	wxBoxSizer* s_checks = new wxBoxSizer( wxVERTICAL );
	*s_checks += m_check_Ejection;

	*this += s_table	| pxExpand;
	*this += s_checks	| StdExpand();

	AppStatusEvent_OnSettingsApplied();
	Disable();		// it's all broken right now, so disable it
}

void Panels::MemoryCardsPanel::OnMultitapChecked( wxCommandEvent& evt )
{
	for( int port=0; port<2; ++port )
	{
		if( m_check_Multitap[port]->GetId() != evt.GetId() ) continue;

		for( uint slot=1; slot<4; ++slot )
		{
			m_CardPanel[port][slot]->Enable( m_check_Multitap[port]->IsChecked() && g_Conf->Mcd[port][slot].Enabled );
		}
	}
}

void Panels::MemoryCardsPanel::Apply()
{
}

void Panels::MemoryCardsPanel::AppStatusEvent_OnSettingsApplied()
{
	const Pcsx2Config& emuconf( g_Conf->EmuOptions );

	for( uint port=0; port<2; ++port )
	for( uint slot=0; slot<4; ++slot )
	{
		m_CardPanel[port][slot]->Enable( g_Conf->Mcd[port][slot].Enabled && ((slot == 0) || emuconf.MultitapEnabled(port)) );
	}
	
	
}
