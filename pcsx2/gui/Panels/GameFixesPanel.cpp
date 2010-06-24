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
	: BaseApplicableConfigPanel( parent )
{
	wxStaticBoxSizer& groupSizer = *new wxStaticBoxSizer( wxVERTICAL, this, _("Gamefixes") );

	// NOTE: Order of checkboxes must match the order of the bits in the GamefixOptions structure!
	// NOTE2: Don't make this static, because translations can change at run-time :)

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
			pxE( ".Tooltip:Gamefixes:EE Timing Hack",
				L"Known to affect following games:\n"
				L" * Digital Devil Saga (Fixes FMV and crashes)\n"
				L" * SSX (Fixes bad graphics and crashes)\n"
				L" * Resident Evil: Dead Aim (Causes garbled textures)"
			)
		},
		{
			_("Skip MPEG hack - Skips videos/FMVs in games to avoid game hanging/freezes."),
			wxEmptyString
		}
	};

	for( int i=0; i<GamefixId_COUNT; ++i )
	{
		groupSizer += (m_checkbox[i] = new pxCheckBox( this, check_text[i].label ));
		m_checkbox[i]->SetToolTip( check_text[i].tooltip );
	}

	m_check_Enable = new pxCheckBox( this, _("Enable game fixes"),
		pxE( ".Panel:Gamefixes:Compat Warning",
			L"Gamefixes can fix wrong emulation in some games. However "
            L"it can cause compatibility or performance issues in other games.  You "
			L"will need to turn off fixes manually when changing games."
		)
	);

	m_check_Enable->SetToolTip(_("The safest way to make sure that all game fixes are completely disabled.")).SetSubPadding( 1 );
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

void Panels::GameFixesPanel::EnableStuff()
{
    for (GamefixId i=GamefixId_FIRST; i < pxEnumEnd; ++i)
    	m_checkbox[i]->Enable(m_check_Enable->GetValue());
}

void Panels::GameFixesPanel::OnEnable_Toggled( wxCommandEvent& evt )
{
	EnableStuff();
	evt.Skip();
}

void Panels::GameFixesPanel::AppStatusEvent_OnSettingsApplied()
{
	const Pcsx2Config::GamefixOptions& opts( g_Conf->EmuOptions.Gamefixes );
	for (GamefixId i=GamefixId_FIRST; i < pxEnumEnd; ++i)
		m_checkbox[i]->SetValue( opts.Get((GamefixId)i) );
}
