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
#include "MainFrame.h"

using namespace Panels;
using namespace pxSizerFlags;

static void CheckHacksOverrides()
{
	if( !wxGetApp().Overrides.HasCustomHacks() ) return;
	
	// The user has commandline overrides enabled, so the options they see here and/or apply won't match
	// the commandline overrides.  Let them know!

	wxDialogWithHelpers dialog( wxFindWindowByName( L"Dialog:" + Dialogs::SysConfigDialog::GetNameStatic() ), _("Config Overrides Warning") );
	
	dialog += dialog.Text( pxEt("!Panel:HasHacksOverrides",
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
	
	dialog += dialog.Text( pxEt("!Panel:HasPluginsOverrides",
		L"Warning!  You are running PCSX2 with command line options that override your configured plugin and/or folder settings.  "
		L"These command line options will not be reflected in the settings dialog, and will be disabled "
		L"when you apply settings changes here."
	));

	// [TODO] : List command line option overrides in action?

	pxIssueConfirmation( dialog, MsgButtons().OK(), L"Dialog.ComponentsConfig.Overrides" );
}

//Behavior when unchecking 'Presets' is to keep the GUI settings at the last preset (even if not yet applied).
//
//Alternative possible behavior when unchecking 'Presets' (currently not implemented) is to set the GUI to
//	the last applied settings. If such behavior is to be implemented, g_Conf->EnablePresets should be set to
//	false before it's applied to the GUI and then restored to it's original state such that the GUI reflects
//	g_Conf's settings as if it doesn't force presets. (if a settings which has presets enable is applied to the
//	GUI then most of the GUI is disabled).
void Dialogs::SysConfigDialog::UpdateGuiForPreset ( int presetIndex, bool presetsEnabled )
{
 	if( !m_listbook )
		return;

	//Console.WriteLn("Applying config to Gui: preset #%d, presets enabled: %s", presetIndex, presetsEnabled?"true":"false");

	AppConfig preset = *g_Conf;
	preset.IsOkApplyPreset( presetIndex );	//apply a preset to a copy of g_Conf.
	preset.EnablePresets = presetsEnabled;	//override IsOkApplyPreset (which always applies/enabled) to actual required state
	
	//update the config panels of SysConfigDialog to reflect the preset.
	size_t pages = m_labels.GetCount();
	for( size_t i=0; i<pages; ++i )
	{
		//NOTE: We should only apply the preset to panels of class BaseApplicableConfigPanel_SpecificConfig
		//      which supports it, and BaseApplicableConfigPanel implements IsSpecificConfig() as lame RTTI to detect it.
		//		However, the panels in general (m_listbook->GetPage(i)) are of type wxNotebookPage which doesn't
		//		support IsSpecificConfig(), so the panels (pages) that SysConfigDialog holds must be of class
		//		BaseApplicableConfigPanel or derived, and not of the parent class wxNotebookPage.
		if ( ((BaseApplicableConfigPanel*)(m_listbook->GetPage(i)))->IsSpecificConfig() )
		{
			((BaseApplicableConfigPanel_SpecificConfig*)(m_listbook->GetPage(i)))
				->ApplyConfigToGui( preset, AppConfig::APPLY_FLAG_FROM_PRESET | AppConfig::APPLY_FLAG_MANUALLY_PROPAGATE );
		}
	}

	//Main menus behavior regarding presets and changes/cancel/apply from SysConfigDialog:
	//1. As long as preset-related values were not changed at SysConfigDialog, menus behave normally.
	//2. After the first preset-related change at SysConfigDialog (this function) and before Apply/Ok/Cancel:
	//	- The menus reflect the temporary pending values, but these preset-controlled items are grayed out even if temporarily presets is unchecked.
	//3. When clicking Ok/Apply/Cancel at SysConfigDialog, the menus are re-alligned with g_Conf (including gray out or not as needed).
	//NOTE: Enabling the presets and disabling them wihout clicking Apply leaves the pending menu config at last preset values
	//		(consistent with SysConfigDialog behavior). But unlike SysConfigDialog, the menu items stay grayed out.
	//		Clicking cancel will revert all pending changes, but clicking apply will commit them, and this includes the menus.
	//		E.g.:
	//			1. Patches (menu) is disabled and presets (SysConfigDialog) is disabled.
	//			2. Opening config and checking presets without apply --> patches are visually enabled and grayed out (not yet applied to g_Conf)
	//			3. Unchecking presets, still without clicking apply  --> patches are visually still enabled (last preset values) and grayed out.
	//			4. Clicking Apply (presets still unchecked) --> patches will be enabled and not grayed out, presets are disabled.
	//			--> If clicking Cancel instead of Apply at 4., will revert everything to the state of 1 (preset disabled, patches disabled and not grayed out).
	
	bool origEnable=preset.EnablePresets;
	preset.EnablePresets=true;	// will cause preset-related items to be grayed out at the menus regardless of their value.
	if ( GetMainFramePtr() )
		GetMainFramePtr()->ApplyConfigToGui( preset, AppConfig::APPLY_FLAG_FROM_PRESET | AppConfig::APPLY_FLAG_MANUALLY_PROPAGATE );
	
	// Not really needed as 'preset' is local and dumped anyway. For the sake of future modifications of more GUI elements.
	preset.EnablePresets=origEnable;	
	
}

void Dialogs::SysConfigDialog::AddPresetsControl()
{
	m_slider_presets = new wxSlider( this, wxID_ANY, g_Conf->PresetIndex, 0, AppConfig::GetMaxPresetIndex(),
		wxDefaultPosition, wxDefaultSize, wxHORIZONTAL /*| wxSL_AUTOTICKS | wxSL_LABELS */);

	m_slider_presets->SetToolTip(
		pxEt( "!Notice:Tooltip:Presets:Slider",
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

	m_check_presets = new pxCheckBox( this, _("Preset:"), 0);
	m_check_presets->SetToolTip(
		pxEt( "!Notice:Tooltip:Presets:Checkbox",
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
	*m_extraButtonSizer += m_check_presets  | pxMiddle;
	*m_extraButtonSizer += m_slider_presets | pxMiddle;
	*m_extraButtonSizer += 5;
	*m_extraButtonSizer += m_msg_preset     | pxMiddle;

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

//Write the values SysConfigDialog holds (preset index and enabled) to g_Conf.
//Make the main menu system write the presets values it holds to g_Conf (preset may have affected the gui without changing g_Conf)
//The panels will write themselves to g_Conf on apply (AFTER this function) and will also trigger a global OnSettingsApplied.
void Dialogs::SysConfigDialog::Apply()
{
	//Console.WriteLn("Applying preset to to g_Conf: Preset index: %d, EnablePresets: %s", (int)m_slider_presets->GetValue(), m_check_presets->IsChecked()?"true":"false");
	g_Conf->EnablePresets	= m_check_presets->IsChecked();
	g_Conf->PresetIndex		= m_slider_presets->GetValue();
	
	if (GetMainFramePtr())
		GetMainFramePtr()->CommitPreset_noTrigger();
}

//Update the main menu system to reflect the original configuration on cancel.
//The config panels don't need this because they just reload themselves with g_Conf when re-opened next time.
//But the menu system has a mostly persistent state that reflects g_Conf (except for when presets are used).
void Dialogs::SysConfigDialog::Cancel()
{
	if (GetMainFramePtr())
		GetMainFramePtr()->ApplyConfigToGui( *g_Conf, AppConfig::APPLY_FLAG_FROM_PRESET | AppConfig::APPLY_FLAG_MANUALLY_PROPAGATE );
}

Dialogs::SysConfigDialog::SysConfigDialog(wxWindow* parent)
	: BaseConfigurationDialog( parent, AddAppName(_("Emulation Settings - %s")), 580 )
{
	ScopedBusyCursor busy( Cursor_ReallyBusy );

	CreateListbook( wxGetApp().GetImgList_Config() );
	const AppImageIds::ConfigIds& cfgid( wxGetApp().GetImgId().Config );

	//NOTE: all pages which are added to SysConfigDialog must be of class BaseApplicableConfigPanel or derived.
	//		see comment inside UpdateGuiForPreset implementation for more info.
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
