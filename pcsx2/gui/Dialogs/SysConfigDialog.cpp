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

//Current behavior when unchecking 'Presets' is to keep the GUI settings at the last preset (even if not applied).
//
//Alternative GUI behavior is that when 'Preset' is unchecked,
//  the GUI settings return to the last applied settings.
//  This allows the user to keep tweaking his "personal' settings and toggling 'Presets' for comparison,
//  or start tweaking from a specific preset by clicking Apply before unchecking 'Presets'
//  However, I think it's more confusing. Uncomment the next line to use the alternative behavior.
//#define PRESETS_USE_APPLIED_CONFIG_ON_UNCHECK
void Dialogs::SysConfigDialog::UpdateGuiForPreset ( int presetIndex, bool presetsEnabled )
{
	AppConfig preset = *g_Conf;
	preset.IsOkApplyPreset(presetIndex);
	preset.EnablePresets=presetsEnabled;//override IsOkApplyPreset to actual required state

 	if( m_listbook ){
		//Console.WriteLn("Applying config to Gui: preset #%d, presets enabled: %s", presetIndex, presetsEnabled?"true":"false");
 		size_t pages = m_labels.GetCount();
		for( size_t i=0; i<pages; ++i ){
			bool origPresetsEnabled = g_Conf->EnablePresets;
			if( !presetsEnabled )
				g_Conf->EnablePresets = false; // unly used when PRESETS_USE_APPLIED_CONFIG_WHEN_UNCHECKED is NOT defined

 			(
 				(BaseApplicableConfigPanel_SpecificConfig*)(m_listbook->GetPage(i))

#ifdef PRESETS_USE_APPLIED_CONFIG_ON_UNCHECK
			)->ApplyConfigToGui( presetsEnabled?preset:*g_Conf, true );
			//Console.WriteLn("SysConfigDialog::UpdateGuiForPreset: Using object: %s", presetsEnabled?"preset":"*g_Conf");
#else
			)->ApplyConfigToGui( preset, true );
			//Console.WriteLn("SysConfigDialog::UpdateGuiForPreset: Using object: %s", "preset");
#endif
			g_Conf->EnablePresets = origPresetsEnabled;
		}

	}

}

void Dialogs::SysConfigDialog::AddPresetsControl()
{
	m_slider_presets = new wxSlider( this, wxID_ANY, g_Conf->PresetIndex, 0, AppConfig::GeMaxPresetIndex(),
		wxDefaultPosition, wxDefaultSize, wxHORIZONTAL /*| wxSL_AUTOTICKS | wxSL_LABELS */);

	m_slider_presets->SetToolTip(
		pxE( "!Notice:Tooltip:Presets:Slider",
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
		pxE( "!Notice:Tooltip:Presets:Checkbox",
				L"The Presets apply speed hacks, some recompiler options and some game fixes known to boost speed.\n"
				L"Known important game fixes ('Patches') will be applied automatically.\n\n"
//This creates nested macros = not working. Un/comment manually if needed.
//#ifdef PRESETS_USE_APPLIED_CONFIG_ON_UNCHECK
//				L"--> Uncheck to modify settings manually."
//              L"If you want to manually modify with a preset as a base, apply this preset, then uncheck."
//#else
				L"--> Uncheck to modify settings manually (with current preset as base)"
//#endif
			)
	);
	m_check_presets->SetValue(!!g_Conf->EnablePresets);
	//Console.WriteLn("--> SysConfigDialog::AddPresetsControl: EnablePresets: %s", g_Conf->EnablePresets?"true":"false");

	wxString l; wxColor c(wxColour( L"Red" ));
	AppConfig::isOkGetPresetTextAndColor(g_Conf->PresetIndex, l, c);
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

	Connect( m_slider_presets->GetId(),	wxEVT_SCROLL_THUMBTRACK,		wxScrollEventHandler( Dialogs::SysConfigDialog::Preset_Scroll ) );
	Connect( m_slider_presets->GetId(),	wxEVT_SCROLL_CHANGED,			wxScrollEventHandler( Dialogs::SysConfigDialog::Preset_Scroll ) );
	Connect( m_check_presets->GetId(), 	wxEVT_COMMAND_CHECKBOX_CLICKED,	wxCommandEventHandler( Dialogs::SysConfigDialog::Presets_Toggled ) );
}



void Dialogs::SysConfigDialog::Presets_Toggled(wxCommandEvent &event)
{
	m_slider_presets->Enable( m_check_presets->IsChecked() );
	m_msg_preset->Enable( m_check_presets->IsChecked() );
	UpdateGuiForPreset( m_slider_presets->GetValue(), m_check_presets->IsChecked() );

	event.Skip();
}


void Dialogs::SysConfigDialog::Preset_Scroll(wxScrollEvent &event)
{	
	wxString pl;
	wxColor c;
	AppConfig::isOkGetPresetTextAndColor(m_slider_presets->GetValue(), pl, c);
	m_msg_preset->SetLabel(pl);
	m_msg_preset->SetForegroundColour( c );

	UpdateGuiForPreset( m_slider_presets->GetValue(), m_check_presets->IsChecked() );
	event.Skip();
}

void Dialogs::SysConfigDialog::Apply()
{
	//Console.WriteLn("Applying preset to to g_Conf: Preset index: %d, EnablePresets: %s", (int)m_slider_presets->GetValue(), m_check_presets->IsChecked()?"true":"false");
	g_Conf->EnablePresets	= m_check_presets->IsChecked();
	g_Conf->PresetIndex		= m_slider_presets->GetValue();
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
	AddPage<GSWindowSettingsPanel>	( pxL("GS Window"),		cfgid.Video );
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
	: BaseConfigurationDialog( parent, AddAppName(_("Appearance/Themes - %s")), 400 )
{
	ScopedBusyCursor busy( Cursor_ReallyBusy );

	CreateListbook( wxGetApp().GetImgList_Config() );
	const AppImageIds::ConfigIds& cfgid( wxGetApp().GetImgId().Config );

	AddPage<AppearanceThemesPanel>	( pxL("Appearance"),	cfgid.Appearance );
	AddPage<StandardPathsPanel>		( pxL("Folders"),		cfgid.Paths );

	AddListbook();
	AddOkCancel();

	//*this += new Panels::LanguageSelectionPanel( this ) | pxCenter;
	//wxDialogWithHelpers::AddOkCancel( NULL, false );
}

// ------------------------------------------------------------------------
Panels::AppearanceThemesPanel::AppearanceThemesPanel( wxWindow* parent )
	: BaseApplicableConfigPanel( parent )
{

}

AppearanceThemesPanel::~AppearanceThemesPanel() throw()
{

}

void AppearanceThemesPanel::Apply()
{

}

void AppearanceThemesPanel::AppStatusEvent_OnSettingsApplied()
{

}
