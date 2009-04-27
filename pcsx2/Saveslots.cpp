/*  Pcsx2 - Pc Ps2 Emulator
 *  Copyright (C) 2002-2009  Pcsx2 Team
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA
 */

#include "PrecompiledHeader.h"
#include "Common.h"
#include "HostGui.h"

#include "GS.h"

StartupParams g_Startup;

//////////////////////////////////////////////////////////////////////////////////////////
// Save Slot Detection System

static int Slots[5] = { -1, -1, -1, -1, -1 };

bool States_isSlotUsed(int num)
{
	if (ElfCRC == 0) 
		return false;
	else
		return wxFileExists( SaveState::GetFilename( num ) );
}

//////////////////////////////////////////////////////////////////////////////////////////
// Save state load-from-file (or slot) helpers.

// Internal use state loading function which does not trap exceptions.
// The calling function should trap and handle exceptions as needed.
static void _loadStateOrExcept( const wxString& file )
{
	gzLoadingState joe( file );		// this'll throw an StateLoadError_Recoverable.

	// Make sure the cpu and plugins are ready to be state-ified!
	cpuReset();
	OpenPlugins( NULL );

	joe.FreezeAll();

	if( GSsetGameCRC != NULL )
		GSsetGameCRC(ElfCRC, g_ZeroGSOptions);
}

// returns true if the new state was loaded, or false if nothing happened.
void States_Load( const wxString& file )
{
	try
	{
		_loadStateOrExcept( file );
		HostGui::Notice( wxsFormat( _("Loaded State %s"), file.c_str() ) );
	}
	catch( Exception::StateLoadError_Recoverable& ex)
	{
		Console::Notice( ex.LogMessage() );

		// At this point the cpu hasn't been reset, so we can return
		// control to the user safely... (that's why we use a console notice instead of a popup)

		return;
	}
	catch( Exception::BaseException& ex )
	{
		// The emulation state is ruined.  Might as well give them a popup and start the gui.
		// Translation Tip: Since the savestate load was incomplete, the emulator has been reset.

		Msgbox::Alert(
			wxsFormat( _("Error loading savestate from file: %s"), file.c_str() ) +
			L"\n\n" + _("Error details:") + ex.DisplayMessage()
		);
		SysReset();
		return;
	}
	HostGui::BeginExecution();
}

void States_Load(int num)
{
	wxString file( SaveState::GetFilename( num ) );

	if( !Path::IsFile( file ) )
	{
		Console::Notice( "Saveslot %d is empty.", params num );
		return;
	}

	try
	{
		_loadStateOrExcept( file );
		HostGui::Notice( wxsFormat( _("Loaded State %d"), num ) );
	}
	catch( Exception::StateLoadError_Recoverable& ex)
	{
		Console::Notice( wxsFormat( L"Could not load savestate slot %d.\n\n%s", num, ex.LogMessage() ) );

		// At this point the cpu hasn't been reset, so we can return
		// control to the user safely... (that's why we use a console notice instead of a popup)

		return;
	}
	catch( Exception::BaseException& ex )
	{
		// The emulation state is ruined.  Might as well give them a popup and start the gui.
		// Translation Tip: Since the savestate load was incomplete, the emulator has been reset.

		Msgbox::Alert(
			wxsFormat( _("Error loading savestate from slot %d"), file.c_str() ) +
			L"\n\n" + _("Error details:") + ex.DisplayMessage()
		);

		SysEndExecution();
		return;
	}

	HostGui::BeginExecution();
}

//////////////////////////////////////////////////////////////////////////////////////////
// Save state save-to-file (or slot) helpers.

void States_Save( const wxString& file )
{
	try
	{
		StateRecovery::SaveToFile( file );
		HostGui::Notice( wxsFormat( _("State saved to file: %s"), file.c_str() ) );
	}
	catch( Exception::BaseException& ex )
	{
		// TODO: Implement a "pause the action and issue a popup" thing here.
		// *OR* some kind of GS overlay... [for now we use the console]

		// Translation Tip: "Your emulation state has not been saved!"

		/*Msgbox::Alert(
			wxsFormat( _("Error saving state to file: %s"), file.c_str() ) +
			L"\n\n" + _("Error details:") + ex.DisplayMessage()
		);*/

		Console::Error( wxsFormat(
			L"An error occurred while trying to save to file %s\n", file.c_str() ) +
			L"Your emulation state has not been saved!\n\nError: " + ex.LogMessage()
		);
	}

	// Filename could be a valid slot, so still need to update
	HostGui::ResetMenuSlots();
}

void States_Save(int num)
{
	try
	{
		StateRecovery::SaveToSlot( num );
		HostGui::Notice( wxsFormat( _("State saved to slot %d"), num ) );
	}
	catch( Exception::BaseException& ex )
	{
		// TODO: Implement a "pause the action and issue a popup" thing here.
		// *OR* some kind of GS overlay... [for now we use the console]

		Console::Error( wxsFormat(
			L"An error occurred while trying to save to slot %d\n", num ) +
			L"Your emulation state has not been saved!\n\nError: " + ex.LogMessage()
		);
	}
	HostGui::ResetMenuSlots();
}

//////////////////////////////////////////////////////////////////////////////////////////
//
void vSyncDebugStuff( uint frame )
{
#ifdef OLD_TESTBUILD_STUFF
	if( g_TestRun.enabled && g_TestRun.frame > 0 ) {
		if( frame > g_TestRun.frame ) {
			// take a snapshot
			if( g_TestRun.pimagename != NULL && GSmakeSnapshot2 != NULL ) {
				if( g_TestRun.snapdone ) {
					g_TestRun.curimage++;
					g_TestRun.snapdone = 0;
					g_TestRun.frame += 20;
					if( g_TestRun.curimage >= g_TestRun.numimages ) {
						// exit
						SysEndExecution();
					}
				}
				else {
					// query for the image
					GSmakeSnapshot2(g_TestRun.pimagename, &g_TestRun.snapdone, g_TestRun.jpgcapture);
				}
			}
			else {
				// exit
				SysEndExecution();
			}
		}
	}

	GSVSYNC();

	if( g_SaveGSStream == 1 ) {
		freezeData fP;

		g_SaveGSStream = 2;
		g_fGSSave->gsFreeze();

		if (GSfreeze(FREEZE_SIZE, &fP) == -1) {
			safe_delete( g_fGSSave );
			g_SaveGSStream = 0;
		}
		else {
			fP.data = (s8*)malloc(fP.size);
			if (fP.data == NULL) {
				safe_delete( g_fGSSave );
				g_SaveGSStream = 0;
			}
			else {
				if (GSfreeze(FREEZE_SAVE, &fP) == -1) {
					safe_delete( g_fGSSave );
					g_SaveGSStream = 0;
				}
				else {
					g_fGSSave->Freeze( fP.size );
					if (fP.size) {
						g_fGSSave->FreezeMem( fP.data, fP.size );
						free(fP.data);
					}
				}
			}
		}
	}
	else if( g_SaveGSStream == 2 ) {

		if( --g_nLeftGSFrames <= 0 ) {
			safe_delete( g_fGSSave );
			g_SaveGSStream = 0;
			Console::WriteLn("Done saving GS stream");
		}
	}
#endif
}
