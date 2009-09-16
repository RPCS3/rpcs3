/*  PCSX2 - PS2 Emulator for PCs
 *  Copyright (C) 2002-2009  PCSX2 Dev Team
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
#include "HostGui.h"

static wxScopedPtr<SafeArray<u8>> g_RecoveryState;

namespace StateRecovery {

	bool HasState()
	{
		return g_RecoveryState;
	}

	void Recover()
	{
		if( !g_RecoveryState ) return;

		Console::Status( "Resuming execution from full memory state..." );
		memLoadingState( *g_RecoveryState ).FreezeAll();

		StateRecovery::Clear();
		SysClearExecutionCache();
	}

	void SaveToFile( const wxString& file )
	{
		SafeArray<u8> buf;
		memSavingState( buf ).FreezeAll();
	}

	// Saves recovery state info to the given saveslot, or saves the active emulation state
	// (if one exists) and no recovery data was found.  This is needed because when a recovery
	// state is made, the emulation state is usually reset so the only persisting state is
	// the one in the memory save. :)
	void SaveToSlot( uint num )
	{
		SaveToFile( SaveStateBase::GetFilename( num ) );
	}
	
	// Creates a full recovery of the entire emulation state (CPU and all plugins).
	// If a current recovery state is already present, then nothing is done (the
	// existing recovery state takes precedence since if it were out-of-date it'd be
	// deleted!).
	void MakeFull()
	{
		if( g_RecoveryState ) return;
		if( !EmulationInProgress() ) return;
		
		try
		{
			g_RecoveryState.reset( new SafeArray<u8>( L"Memory Savestate Recovery" ) );
			memSavingState( *g_RecoveryState ).FreezeAll();
		}
		catch( Exception::RuntimeError& ex )
		{
			Msgbox::Alert( wxsFormat(	// fixme: needs proper translation
				L"PCSX2 encountered an error while trying to backup/suspend the PS2 VirtualMachine state. "
				L"You may resume emulation without losing any data, however the machine state will not be "
				L"able to recover if you make changes to your PCSX2 configuration.\n\n"
				L"Details: %s", ex.FormatDisplayMessage().c_str() )
			);
			g_RecoveryState.reset();
		}
	}

	// Clears and deallocates any recovery states.
	void Clear()
	{
		g_RecoveryState.reset();
	}
}
