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
#include "MSWstuff.h"

#include "ConfigurationDialog.h"
#include "BaseConfigurationDialog.inl"
#include "ModalPopups.h"
#include "Panels/ConfigurationPanels.h"

//#include <wx/filepicker.h>

using namespace Panels;

Dialogs::SysConfigDialog::SysConfigDialog(wxWindow* parent)
	: BaseConfigurationDialog( parent, _("PS2 Settings - PCSX2"), wxGetApp().GetImgList_Config(), 600 )
{
	SetName( GetNameStatic() );

	const AppImageIds::ConfigIds& cfgid( wxGetApp().GetImgId().Config );

	AddPage<CpuPanelEE>			( wxLt("EE/IOP"),		cfgid.Cpu );
	AddPage<CpuPanelVU>			( wxLt("VUs"),			cfgid.Cpu );
	AddPage<VideoPanel>			( wxLt("GS"),			cfgid.Video );
	AddPage<SpeedHacksPanel>	( wxLt("Speedhacks"),	cfgid.Speedhacks );
	AddPage<GameFixesPanel>		( wxLt("Game Fixes"),	cfgid.Gamefixes );
	AddPage<PluginSelectorPanel>( wxLt("Plugins"),		cfgid.Plugins );

	MSW_ListView_SetIconSpacing( m_listbook, m_idealWidth );

	AddOkCancel();
}

Dialogs::AppConfigDialog::AppConfigDialog(wxWindow* parent)
	: BaseConfigurationDialog( parent, _("Application Settings - PCSX2"), wxGetApp().GetImgList_Config(), 600 )
{
	SetName( GetNameStatic() );

	const AppImageIds::ConfigIds& cfgid( wxGetApp().GetImgId().Config );

	AddPage<GSWindowSettingsPanel>	( wxLt("GS Window"),	cfgid.Paths );
	AddPage<StandardPathsPanel>		( wxLt("Folders"),		cfgid.Paths );

	MSW_ListView_SetIconSpacing( m_listbook, GetClientSize().GetWidth() );

	AddOkCancel();
}
