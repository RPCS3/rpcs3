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

TESTRUNARGS g_TestRun;


//////////////////////////////////////////////////////////////////////////////////////////
// Save Slot Detection System

static int Slots[5] = { -1, -1, -1, -1, -1 };

bool States_isSlotUsed(int num)
{
	if (ElfCRC == 0) 
		return false;
	else
		return Path::isFile(SaveState::GetFilename( num ));
}

//////////////////////////////////////////////////////////////////////////////////////////
// Save state load-from-file (or slot) helpers.

// Internal use state loading function which does not trap exceptions.
// The calling function should trap ahnd handle exceptions as needed.
static void _loadStateOrExcept( const string& file )
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
void States_Load( const string& file )
{
	try
	{
		_loadStateOrExcept( file );
		HostGui::Notice( fmt_string( "*PCSX2*: Loaded State %s", file) );
	}
	catch( Exception::StateLoadError_Recoverable& ex)
	{
		Console::Notice( "Could not load savestate file: %s.\n\n%s", params file, ex.cMessage() );

		// At this point the cpu hasn't been reset, so we can return
		// control to the user safely... (that's why we use a console notice instead of a popup)

		return;
	}
	catch( Exception::BaseException& ex )
	{
		// The emulation state is ruined.  Might as well give them a popup and start the gui.

		string message( fmt_string(
			"Encountered an error while loading savestate from file: %s.\n", file ) );

		if( g_EmulationInProgress )
			message += "Since the savestate load was incomplete, the emulator must reset.\n";

		message += "\nError: " + ex.Message();

		Msgbox::Alert( message.c_str() );
		SysReset();
		return;
	}
	HostGui::BeginExecution();
}

void States_Load(int num)
{
	string file( SaveState::GetFilename( num ) );

	if( !Path::isFile( file ) )
	{
		Console::Notice( "Saveslot %d is empty.", params num );
		return;
	}

	try
	{
		_loadStateOrExcept( file );
		HostGui::Notice( fmt_string( "*PCSX2*: Loaded State %d", num ) );
	}
	catch( Exception::StateLoadError_Recoverable& ex)
	{
		Console::Notice( "Could not load savestate slot %d.\n\n%s", params num, ex.cMessage() );

		// At this point the cpu hasn't been reset, so we can return
		// control to the user safely... (that's why we use a console notice instead of a popup)

		return;
	}
	catch( Exception::BaseException& ex )
	{
		// The emulation state is ruined.  Might as well give them a popup and start the gui.

		string message( fmt_string(
			"Encountered an error while loading savestate from slot %d.\n", num ) );

		if( g_EmulationInProgress )
			message += "Since the savestate load was incomplete, the emulator has been reset.\n";

		message += "\nError: " + ex.Message();

		Msgbox::Alert( message.c_str() );
		SysEndExecution();
		return;
	}

	HostGui::BeginExecution();
}

//////////////////////////////////////////////////////////////////////////////////////////
// Save state save-to-file (or slot) helpers.

void States_Save( const string& file )
{
	try
	{
		StateRecovery::SaveToFile( file );
		HostGui::Notice( fmt_string( "State saved to file: %s", file ) );
	}
	catch( Exception::BaseException& ex )
	{
		Console::Error( (fmt_string(
			"An error occurred while trying to save to file %s\n", file ) +
			"Your emulation state has not been saved!\n\nError: " + ex.Message()).c_str()
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
		HostGui::Notice( fmt_string( "State saved to slot %d", num ) );
	}
	catch( Exception::BaseException& ex )
	{
		Console::Error( (fmt_string(
			"An error occurred while trying to save to slot %d\n", num ) +
			"Your emulation state has not been saved!\n\nError: " + ex.Message()).c_str()
		 );
	}
	HostGui::ResetMenuSlots();
}

//////////////////////////////////////////////////////////////////////////////////////////
//
void vSyncDebugStuff( uint frame )
{
#ifdef PCSX2_DEVBUILD
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
