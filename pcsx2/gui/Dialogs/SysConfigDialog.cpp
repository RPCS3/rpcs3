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

#include "ConfigurationDialog.h"
#include "BaseConfigurationDialog.inl"
#include "ModalPopups.h"
#include "Panels/ConfigurationPanels.h"

using namespace Panels;
using namespace pxSizerFlags;

static void CheckHacksOverrides()
{
	if( !wxGetApp().Overrides.HasCustomHacks() ) return;
	
	// The user has commandline overrides enabled, so the options they see here and/or apply won't match
	// the commandline overrides.  Let them know!

	wxDialogWithHelpers dialog( wxFindWindowByName( L"Dialog:" + Dialogs::SysConfigDialog::GetNameStatic() ), _("Config Overrides Warning") );
	
	dialog += dialog.Text( pxE("!Panel:HasHacksOverrides",
		L"Warning!  You are running PCSX2 with command line options that override your configured settings.  "
		L"These command line options will not be reflected in the Settings dialog, and will be disabled "
		L"if you apply any changes here."
	));

	// [TODO] : List command line option overrides in action?

	pxIssueConfirmation( dialog, MsgButtons().OK(), L"Dialog.SysConfig.Overrides" );
}

static void CheckPluginsOverrides()
{
	if( !wxGetApp().Overrides.HasPluginsOverride() ) return;
	
	// The user has commandline overrides enabled, so the options they see here and/or apply won't match
	// the commandline overrides.  Let them know!

	wxDialogWithHelpers dialog( NULL, _("Components Overrides Warning") );
	
	dialog += dialog.Text( pxE("!Panel:HasPluginsOverrides",
		L"Warning!  You are running PCSX2 with command line options that override your configured plugin and/or folder settings.  "
		L"These command line options will not be reflected in the settings dialog, and will be disabled "
		L"when you apply settings changes here."
	));

	// [TODO] : List command line option overrides in action?

	pxIssueConfirmation( dialog, MsgButtons().OK(), L"Dialog.ComponentsConfig.Overrides" );
}

bool isOkGetPresetTextAndColor(int n, wxString& label, wxColor& c){
	switch(n){
		case 0: label=pxE("!Panel:", L"1 - Safest");		c=wxColor(L"Forest GREEN"); break;
		case 1: label=pxE("!Panel:", L"2 - Safe (faster)");	c=wxColor(L"Dark Green"); break;
		case 2: label=pxE("!Panel:", L"3 - Balanced");			c=wxColor(L"Blue");break;
		case 3: label=pxE("!Panel:", L"4 - Aggressive");		c=wxColor(L"Purple"); break;
		case 4: label=pxE("!Panel:", L"5 - Aggressive plus");	c=wxColor(L"Orange"); break;
		case 5: label=pxE("!Panel:", L"6 - Mostly Harmful");	c=wxColor(L"Red");break;
		default: return false;
	}
	return true;
}


void Dialogs::SysConfigDialog::AddPresetsControl()
{
	m_slider_presets = new wxSlider( this, wxID_ANY, g_Conf->PresetIndex, 0, AppConfig::GeMaxPresetIndex(),
		wxDefaultPosition, wxDefaultSize, wxHORIZONTAL /*| wxSL_AUTOTICKS | wxSL_LABELS */);

	m_slider_presets->SetToolTip(
		pxE( "!Notice:Tooltip",
				L"The Presets apply speed hacks, some recompiler options and some game fixes known to boost speed.\n"
				L"Known important game fixes ('Patches') will be applied automatically.\n\n"
				L"Presets info:\n"
				L"1 -     The most accurate emulation but also the slowest.\n"
				L"3 --> Tries to balance speed with compatibility.\n"
				L"4 -     Some more aggressive hacks.\n"
				L"6 -     Too many hacks which will probably slow down most games.\n"
			)
	);
	m_slider_presets->Enable(g_Conf->EnablePresets);

	m_check_presets = new pxCheckBox( this, pxE("!Panel:", L"Preset:"), 0);
	m_check_presets->SetToolTip(
		pxE( "!Notice:Tooltip",
				L"The Presets apply speed hacks, some recompiler options and some game fixes known to boost speed.\n"
				L"Known important game fixes ('Patches') will be applied automatically.\n\n"
				L"--> Uncheck to modify settings manually (with current preset as base)"
			)
	);
	m_check_presets->SetValue(g_Conf->EnablePresets);

	wxString l; wxColor c(wxColour( L"Red" ));
	isOkGetPresetTextAndColor(g_Conf->PresetIndex, l, c);
	m_msg_preset = new pxStaticText(this, l, wxALIGN_LEFT);
	m_msg_preset->Enable(g_Conf->EnablePresets);
	m_msg_preset->SetForegroundColour( c );
	m_msg_preset->Bold();
	
	//I'm unable to do without the next 2 rows.. what am I missing?
	m_msg_preset->SetMinWidth(150);
	m_msg_preset->Unwrapped();


	*m_extraButtonSizer += 20;
	*m_extraButtonSizer += *m_check_presets  | pxMiddle;
	*m_extraButtonSizer += *m_slider_presets | pxMiddle;
	*m_extraButtonSizer += 5;
	*m_extraButtonSizer += *m_msg_preset	 | pxMiddle;

	Connect( m_slider_presets->GetId(),	wxEVT_SCROLL_THUMBTRACK, wxScrollEventHandler( Dialogs::SysConfigDialog::Preset_Scroll ) );
	Connect( m_slider_presets->GetId(),	wxEVT_SCROLL_CHANGED,    wxScrollEventHandler( Dialogs::SysConfigDialog::Preset_Scroll ) );
	Connect( m_check_presets->GetId(), wxEVT_COMMAND_CHECKBOX_CLICKED, wxCommandEventHandler( Dialogs::SysConfigDialog::Presets_Toggled ) );
}

void Dialogs::SysConfigDialog::Presets_Toggled(wxCommandEvent &event)
{
	g_Conf->EnablePresets = m_check_presets->IsChecked();
	m_slider_presets->Enable(g_Conf->EnablePresets);
	m_msg_preset->Enable(g_Conf->EnablePresets);

	if (g_Conf->EnablePresets)
		g_Conf->IsOkApplyPreset(g_Conf->PresetIndex);

	sApp.DispatchEvent( AppStatus_SettingsApplied );
	event.Skip();
}


void Dialogs::SysConfigDialog::Preset_Scroll(wxScrollEvent &event)
{	
	if (m_slider_presets->GetValue() == g_Conf->PresetIndex)
		return;

	wxString pl;
	wxColor c;
	isOkGetPresetTextAndColor(m_slider_presets->GetValue(), pl, c);
	m_msg_preset->SetLabel(pl);
	m_msg_preset->SetForegroundColour( c );

	g_Conf->IsOkApplyPreset(m_slider_presets->GetValue());

	sApp.DispatchEvent( AppStatus_SettingsApplied );
	event.Skip();
}

Dialogs::SysConfigDialog::SysConfigDialog(wxWindow* parent)
	: BaseConfigurationDialog( parent, AddAppName(_("Emulation Settings - %s")), 580 )
{
	ScopedBusyCursor busy( Cursor_ReallyBusy );

	CreateListbook( wxGetApp().GetImgList_Config() );
	const AppImageIds::ConfigIds& cfgid( wxGetApp().GetImgId().Config );

	AddPage<CpuPanelEE>				( pxL("EE/IOP"),		cfgid.Cpu );
	AddPage<CpuPanelVU>				( pxL("VUs"),			cfgid.Cpu );
	AddPage<VideoPanel>				( pxL("GS"),			cfgid.Cpu );
	AddPage<GSWindowSettingsPanel>	( pxL("GS Window"),	cfgid.Video );
	AddPage<SpeedHacksPanel>		( pxL("Speedhacks"),	cfgid.Speedhacks );
	AddPage<GameFixesPanel>			( pxL("Game Fixes"),	cfgid.Gamefixes );

	AddListbook();
	AddOkCancel();
	AddPresetsControl();

	if( wxGetApp().Overrides.HasCustomHacks() )
		wxGetApp().PostMethod( CheckHacksOverrides );
}

Dialogs::ComponentsConfigDialog::ComponentsConfigDialog(wxWindow* parent)
	: BaseConfigurationDialog( parent, AddAppName(_("Components Selectors - %s")),  600 )
{
	ScopedBusyCursor busy( Cursor_ReallyBusy );

	CreateListbook( wxGetApp().GetImgList_Config() );
	const AppImageIds::ConfigIds& cfgid( wxGetApp().GetImgId().Config );

	AddPage<PluginSelectorPanel>	( pxL("Plugins"),		cfgid.Plugins );
	AddPage<BiosSelectorPanel>		( pxL("BIOS"),			cfgid.Cpu );

	AddListbook();
	AddOkCancel();

	if( wxGetApp().Overrides.HasPluginsOverride() )
		wxGetApp().PostMethod( CheckPluginsOverrides );
}

Dialogs::InterfaceConfigDialog::InterfaceConfigDialog(wxWindow *parent)
	: BaseConfigurationDialog( parent, AddAppName(_("Language Selector - %s")), 400 )
{
	ScopedBusyCursor busy( Cursor_ReallyBusy );

	CreateListbook( wxGetApp().GetImgList_Config() );
	const AppImageIds::ConfigIds& cfgid( wxGetApp().GetImgId().Config );

	AddPage<StandardPathsPanel>		( pxL("Appearance"),	cfgid.Appearance );
	AddPage<StandardPathsPanel>		( pxL("Folders"),		cfgid.Paths );

	AddListbook();
	AddOkCancel();

	//*this += new Panels::LanguageSelectionPanel( this ) | pxCenter;
	//wxDialogWithHelpers::AddOkCancel( NULL, false );
}