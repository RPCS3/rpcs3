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

Dialogs::SysConfigDialog::SysConfigDialog(wxWindow* parent)
	: BaseConfigurationDialog( parent, AddAppName(_("Emulation Settings - %s")), 580 )
{
	ScopedBusyCursor busy( Cursor_ReallyBusy );

	CreateListbook( wxGetApp().GetImgList_Config() );
	const AppImageIds::ConfigIds& cfgid( wxGetApp().GetImgId().Config );

	AddPage<CpuPanelEE>				( wxLt("EE/IOP"),		cfgid.Cpu );
	AddPage<CpuPanelVU>				( wxLt("VUs"),			cfgid.Cpu );
	AddPage<VideoPanel>				( wxLt("GS"),			cfgid.Cpu );
	AddPage<GSWindowSettingsPanel>	( wxLt("GS Window"),	cfgid.Video );
	AddPage<SpeedHacksPanel>		( wxLt("Speedhacks"),	cfgid.Speedhacks );
	AddPage<GameFixesPanel>			( wxLt("Game Fixes"),	cfgid.Gamefixes );
	AddPage<GameDatabasePanel>		( wxLt("Game Database"),cfgid.Plugins );

	AddListbook();
	AddOkCancel();
}

Dialogs::ComponentsConfigDialog::ComponentsConfigDialog(wxWindow* parent)
	: BaseConfigurationDialog( parent, AddAppName(_("Components Selectors - %s")),  600 )
{
	ScopedBusyCursor busy( Cursor_ReallyBusy );

	CreateListbook( wxGetApp().GetImgList_Config() );
	const AppImageIds::ConfigIds& cfgid( wxGetApp().GetImgId().Config );

	AddPage<PluginSelectorPanel>	( wxLt("Plugins"),		cfgid.Plugins );
	AddPage<BiosSelectorPanel>		( wxLt("BIOS"),			cfgid.Cpu );
	AddPage<StandardPathsPanel>		( wxLt("Folders"),		cfgid.Paths );

	AddListbook();
	AddOkCancel();
}
