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
#include "MainFrame.h"
#include "GSFrame.h"

// This is necessary because this stupid wxWidgets thing has implicit debug errors
// in the FindItem call that asserts if the menu options are missing.  This is bad
// mojo for configurable/dynamic menus. >_<
void MainEmuFrame::EnableMenuItem( int id, bool enable )
{
	if( wxMenuItem* item = m_menubar.FindItem(id) )
		item->Enable( enable );
}

static void _SaveLoadStuff( bool enabled )
{
	sMainFrame.EnableMenuItem( MenuId_Sys_LoadStates, enabled );
	sMainFrame.EnableMenuItem( MenuId_Sys_SaveStates, enabled );
}

// Updates the enable/disable status of all System related controls: menus, toolbars,
// etc.  Typically called by SysEvtHandler whenever the message pump becomes idle.
void UI_UpdateSysControls()
{
	if( wxGetApp().PostMethodMyself( &UI_UpdateSysControls ) ) return;

	sApp.PostAction( CoreThreadStatusEvent( CoreThread_Indeterminate ) );

	_SaveLoadStuff( true );
}

void UI_DisableSysReset()
{
	if( wxGetApp().PostMethodMyself( UI_DisableSysReset ) ) return;
	sMainFrame.EnableMenuItem( MenuId_Sys_Restart, false );

	_SaveLoadStuff( false );
}

void UI_DisableSysShutdown()
{
	if( wxGetApp().PostMethodMyself( &UI_DisableSysShutdown ) ) return;

	sMainFrame.EnableMenuItem( MenuId_Sys_Restart, false );
	sMainFrame.EnableMenuItem( MenuId_Sys_Shutdown, false );
}

void UI_EnableSysShutdown()
{
	if( wxGetApp().PostMethodMyself( &UI_EnableSysShutdown ) ) return;

	sMainFrame.EnableMenuItem( MenuId_Sys_Restart, true );
	sMainFrame.EnableMenuItem( MenuId_Sys_Shutdown, true );
}


void UI_DisableSysActions()
{
	if( wxGetApp().PostMethodMyself( &UI_DisableSysActions ) ) return;

	sMainFrame.EnableMenuItem( MenuId_Sys_Restart, false );
	sMainFrame.EnableMenuItem( MenuId_Sys_Shutdown, false );
	
	_SaveLoadStuff( false );
}

void UI_EnableSysActions()
{
	if( wxGetApp().PostMethodMyself( &UI_EnableSysActions ) ) return;

	sMainFrame.EnableMenuItem( MenuId_Sys_Restart, true );
	sMainFrame.EnableMenuItem( MenuId_Sys_Shutdown, true );
	
	_SaveLoadStuff( true );
}

void UI_DisableStateActions()
{
	if( wxGetApp().PostMethodMyself( &UI_DisableStateActions ) ) return;
	_SaveLoadStuff( false );
}

void UI_EnableStateActions()
{
	if( wxGetApp().PostMethodMyself( &UI_EnableStateActions ) ) return;
	_SaveLoadStuff( true );
}


