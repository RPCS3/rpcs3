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

namespace Panels
{
	// Helper class since the 'AddPage' template system needs a single-parameter constructor.
	class McdConfigPanel_Multitap2 : public McdConfigPanel_Multitap
	{
	public:
		McdConfigPanel_Multitap2( wxWindow* parent ) : McdConfigPanel_Multitap( parent, 1 ) {}
		virtual ~McdConfigPanel_Multitap2() throw() { }
	};
}

wxString GetMsg_McdNtfsCompress()
{
	return pxE( ".Dialog:Memorycards:NtfsCompress", 
		L"NTFS compression is built-in, fast, and completely reliable; and typically compresses MemoryCards "
		L"very well (this option is highly recommended)."
	);
}

Panels::McdConfigPanel_Toggles::McdConfigPanel_Toggles(wxWindow *parent)
	: _parent( parent )
{
	m_idealWidth -= 48;

	m_check_Ejection = new pxCheckBox( this,
		_("Auto-eject memorycards when loading savestates"),
		pxE( ".Dialog:Memorycards:EnableEjection",
			L"Avoids memorycard corruption by forcing games to re-index card contents after "
			L"loading from savestates.  May not be compatible with all games (Guitar Hero)."
		)
	);

	*this += m_check_Ejection		| pxExpand;
	
	#ifdef __WXMSW__
	m_check_CompressNTFS = new pxCheckBox( this,
		_("Enable NTFS Compression on all cards by default."),
		GetMsg_McdNtfsCompress()
	);
	
 	*this += m_check_CompressNTFS	| pxExpand;
	#endif
}

void Panels::McdConfigPanel_Toggles::Apply()
{
	g_Conf->McdEnableEjection	= m_check_Ejection->GetValue();
	
	#ifdef __WXMSW__
	g_Conf->McdCompressNTFS		= m_check_CompressNTFS->GetValue();
	#endif
}

void Panels::McdConfigPanel_Toggles::AppStatusEvent_OnSettingsApplied()
{
	m_check_Ejection		->SetValue( g_Conf->McdEnableEjection );
	#ifdef __WXMSW__
	m_check_CompressNTFS	->SetValue( g_Conf->McdCompressNTFS );
	#endif
}

Panels::McdConfigPanel_Standard::McdConfigPanel_Standard(wxWindow *parent) : _parent( parent )
{
	m_panel_cardinfo[0] = new MemoryCardInfoPanel( this, 0, 0 );
	m_panel_cardinfo[1] = new MemoryCardInfoPanel( this, 1, 0 );
	
	for( uint port=0; port<2; ++port )
	{
		wxStaticBoxSizer& portSizer( *new wxStaticBoxSizer( wxVERTICAL, this, wxsFormat(_("Port %u"), port+1) ) );
		portSizer += m_panel_cardinfo[port]		| pxExpand;

		*this += portSizer	| StdExpand();
	}
}

void Panels::McdConfigPanel_Standard::Apply()
{
}

void Panels::McdConfigPanel_Standard::AppStatusEvent_OnSettingsApplied()
{
}

Panels::McdConfigPanel_Multitap::McdConfigPanel_Multitap(wxWindow *parent, int port) : _parent( parent )
{
	m_port = port;

	m_check_Multitap = new pxCheckBox( this, wxsFormat(_("Enable Multitap on Port %u"), m_port+1) );
	m_check_Multitap->SetFont( wxFont( m_check_Multitap->GetFont().GetPointSize()+1, wxFONTFAMILY_MODERN, wxNORMAL, wxNORMAL, false, L"Lucida Console" ) );

	*this += m_check_Multitap;

	Connect( m_check_Multitap->GetId(), wxEVT_COMMAND_CHECKBOX_CLICKED, wxCommandEventHandler(McdConfigPanel_Multitap::OnMultitapChecked));
}

void Panels::McdConfigPanel_Multitap::OnMultitapChecked( wxCommandEvent& evt )
{

}

void Panels::McdConfigPanel_Multitap::Apply()
{

}

void Panels::McdConfigPanel_Multitap::AppStatusEvent_OnSettingsApplied()
{

}

using namespace Panels;
using namespace pxSizerFlags;

Dialogs::McdConfigDialog::McdConfigDialog( wxWindow* parent )
	: BaseConfigurationDialog( parent, _("MemoryCard Settings - PCSX2"), wxGetApp().GetImgList_Config(), 600 )
{
	SetName( GetNameStatic() );

	// [TODO] : Discover and use a good multitap port icon!  Possibility might be a
	//   simple 3x memorycards icon, in cascading form.
	//  (for now everything defaults to the redundant memorycard icon)

	const AppImageIds::ConfigIds& cfgid( wxGetApp().GetImgId().Config );

	AddPage<McdConfigPanel_Toggles>		( wxLt("Settings"),		cfgid.MemoryCard );
	AddPage<McdConfigPanel_Standard>	( wxLt("Slots 1/2"),	cfgid.MemoryCard );
	AddPage<McdConfigPanel_Multitap>	( wxLt("Multitap 1"),	cfgid.MemoryCard );
	AddPage<McdConfigPanel_Multitap2>	( wxLt("Multitap 2"),	cfgid.MemoryCard );

	MSW_ListView_SetIconSpacing( m_listbook, m_idealWidth );

	m_panel_mcdlist = new MemoryCardListPanel( this );

	*this += StdPadding;
	*this += new wxStaticLine( this )	| StdExpand();
	*this += StdPadding;
	*this += m_panel_mcdlist			| StdExpand();

	AddOkCancel();
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
