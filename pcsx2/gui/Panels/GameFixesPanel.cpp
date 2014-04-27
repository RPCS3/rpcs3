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

using namespace pxSizerFlags;

Panels::GameFixesPanel::GameFixesPanel( wxWindow* parent )
	: BaseApplicableConfigPanel_SpecificConfig( parent )
{
	wxStaticBoxSizer& groupSizer = *new wxStaticBoxSizer( wxVERTICAL, this, _("Gamefixes") );

	// NOTE: Order of checkboxes must match the order of the bits in the GamefixOptions structure!
	// NOTE2: Don't make this static, because translations can change at run-time :)
	// NOTE3: This panel is way to big, header text was reduced as a temporary solution.
	//		  However if you want add more gamefix, a final solution must be found (noteboox/scrolling...)

	struct CheckTextMess
	{
		wxString label, tooltip;
	};

	const CheckTextMess check_text[GamefixId_COUNT] =
	{
		{
			_("VU Add Hack - Fixes Tri-Ace games boot crash."),
			_("Games that need this hack to boot:\n * Star Ocean 3\n * Radiata Stories\n * Valkyrie Profile 2")
		},
		{
			_("VU Clip Flag Hack - For Persona games (SuperVU recompiler only!)"),
			wxEmptyString
		},
		{
			_("FPU Compare Hack - For Digimon Rumble Arena 2."),
			wxEmptyString
		},
		{
			_("FPU Multiply Hack - For Tales of Destiny."),
			wxEmptyString
		},
		{
			_("FPU Negative Div Hack - For Gundam games."),
			wxEmptyString
		},
		{
			_("VU XGkick Hack - For Erementar Gerad."),
			wxEmptyString
		},
		{
			_("FFX videos fix - Fixes bad graphics overlay in FFX videos."),
			wxEmptyString
		},
		{
			_("EE timing hack - Multi purpose hack. Try if all else fails."),
			pxEt( L"Known to affect following games:\n * Digital Devil Saga (Fixes FMV and crashes)\n * SSX (Fixes bad graphics and crashes)\n * Resident Evil: Dead Aim (Causes garbled textures)"
			)
		},
		{
			_("Skip MPEG hack - Skips videos/FMVs in games to avoid game hanging/freezes."),
			wxEmptyString
		},
		{
			_("OPH Flag hack - Try if your game freezes showing the same frame."),
			pxEt( L"Known to affect following games:\n * Bleach Blade Battler\n * Growlanser II and III\n * Wizardry"
			)
		},
		{
			_("Ignore DMAC writes when it is busy."),
			pxEt( L"Known to affect following games:\n * Mana Khemia 1 (Going \"off campus\")\n"
			)
		},
		{
			_("Simulate VIF1 FIFO read ahead. Fixes slow loading games."),
			pxEt( L"Known to affect following games:\n * Test Drive Unlimited\n * Transformers"
			)
		},
		{
			_("Delay VIF1 Stalls (VIF1 FIFO) - For SOCOM 2 HUD."),
			wxEmptyString
		},
		{
			_("Ignore Bus Direction on Path3 Transfer - Used for Hotwheels"),
			wxEmptyString
		},
		{
			_("Switch to GSdx software rendering when a FMV plays"),
			wxEmptyString
		}
	};

	for( int i=0; i<GamefixId_COUNT; ++i )
	{
		groupSizer += (m_checkbox[i] = new pxCheckBox( this, check_text[i].label ));
		m_checkbox[i]->SetToolTip( check_text[i].tooltip );
	}

	m_check_Enable = new pxCheckBox( this, _("Enable manual game fixes [Not recommended]"),
		pxE( L"It's better to enable 'Automatic game fixes' at the main menu instead, and leave this page empty. ('Automatic' means: selectively use specific tested fixes for specific games)"
		)
	);

	m_check_Enable->SetToolTip(_("Gamefixes can work around wrong emulation in some titles. \nThey may also cause compatibility or performance issues.\n\nThe safest way to make sure that all game fixes are completely disabled.")).SetSubPadding( 1 );
	m_check_Enable->SetValue( g_Conf->EnableGameFixes );

	*this	+= m_check_Enable	| StdExpand();
	*this	+= groupSizer		| pxCenter;


	Connect( m_check_Enable->GetId(), wxEVT_COMMAND_CHECKBOX_CLICKED, wxCommandEventHandler( GameFixesPanel::OnEnable_Toggled ) );

	EnableStuff();
}

void Panels::GameFixesPanel::Apply()
{
	g_Conf->EnableGameFixes = m_check_Enable->GetValue();

	Pcsx2Config::GamefixOptions& opts( g_Conf->EmuOptions.Gamefixes );
	for (GamefixId i=GamefixId_FIRST; i < pxEnumEnd; ++i)
		opts.Set((GamefixId)i, m_checkbox[i]->GetValue());

	// make sure the user's command line specifications are disabled (if present).
	wxGetApp().Overrides.ApplyCustomGamefixes = false;
}

void Panels::GameFixesPanel::EnableStuff( AppConfig* configToUse )
{
	if( !configToUse ) configToUse = g_Conf;
	for (GamefixId i=GamefixId_FIRST; i < pxEnumEnd; ++i)
		m_checkbox[i]->Enable(m_check_Enable->GetValue() && !configToUse->EnablePresets);
}

void Panels::GameFixesPanel::OnEnable_Toggled( wxCommandEvent& evt )
{
	AppConfig tmp=*g_Conf;
	tmp.EnablePresets=false; //if clicked, button was enabled, so not using a preset --> let EnableStuff work

	EnableStuff( &tmp );
	evt.Skip();
}

void Panels::GameFixesPanel::AppStatusEvent_OnSettingsApplied()
{
	ApplyConfigToGui( *g_Conf );
}

void Panels::GameFixesPanel::ApplyConfigToGui( AppConfig& configToApply, int flags )
{
	const Pcsx2Config::GamefixOptions& opts( configToApply.EmuOptions.Gamefixes );
	for (GamefixId i=GamefixId_FIRST; i < pxEnumEnd; ++i)
		m_checkbox[i]->SetValue( opts.Get((GamefixId)i) );//apply the use/don't-use fix values
	
	m_check_Enable->SetValue( configToApply.EnableGameFixes );//main gamefixes checkbox
	EnableStuff( &configToApply );// enable/disable the all the fixes controls according to the main one
	
	this->Enable(!configToApply.EnablePresets);
}
