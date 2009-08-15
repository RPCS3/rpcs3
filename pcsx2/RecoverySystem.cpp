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


//////////////////////////////////////////////////////////////////////////////////////////
// RecoverySystem.cpp -- houses code for recovering from on-the-fly changes to the emu
// configuration, and for saving/restoring the GS state (for more seamless exiting of
// fullscreen GS operation).
//
// The following handful of local classes are implemented att he bottom of this file.

static SafeArray<u8>* g_RecoveryState = NULL;
static SafeArray<u8>* g_gsRecoveryState = NULL;

// This class type creates a memory savestate using the existing Recovery information
// (if present) to generate the savestate material.  If no recovery data is present,
// the current emulation state is used instead.
class RecoveryMemSavingState : public memSavingState, Sealed
{
public:
	virtual ~RecoveryMemSavingState() { }
	RecoveryMemSavingState();

	void gsFreeze();
	void FreezePlugin( const char* name, s32 (CALLBACK* freezer)(int mode, freezeData *data) );
};

// This class type creates an on-disk (zipped) savestate using the existing Recovery
// information (if present) to generate the savestate material.  If no recovery data is
// present, the current emulation state is used instead.
class RecoveryZipSavingState : public gzSavingState, Sealed
{
public:
	virtual ~RecoveryZipSavingState() { }
	RecoveryZipSavingState( const wxString& filename );

	void gsFreeze();
	void FreezePlugin( const char* name, s32 (CALLBACK* freezer)(int mode, freezeData *data) );
};

// Special helper class used to save *just* the GS-relevant state information.
class JustGsSavingState : public memSavingState, Sealed
{
public:
	virtual ~JustGsSavingState() { }
	JustGsSavingState();

	// This special override saves the gs info to m_idx+4, and then goes back and
	// writes in the length of data saved.
	void gsFreeze();
};

namespace StateRecovery {

	bool HasState()
	{
		return g_RecoveryState != NULL || g_gsRecoveryState != NULL;
	}

	void Recover()
	{
		// Just in case they weren't initialized earlier (no harm in calling this multiple times)
		OpenPlugins();

		if( g_RecoveryState != NULL )
		{
			Console::Status( "Resuming execution from full memory state..." );
			memLoadingState( *g_RecoveryState ).FreezeAll();
		}
		else if( g_gsRecoveryState != NULL )
		{
			s32 dummylen;

			Console::Status( "Resuming execution from gsState..." );
			memLoadingState eddie( *g_gsRecoveryState );
			eddie.FreezePlugin( "GS", gsSafeFreeze );
			eddie.Freeze( dummylen );		// reads the length value recorded earlier.
			eddie.gsFreeze();
		}

		StateRecovery::Clear();

		// this needs to be called for every new game!
		// (note: sometimes launching games through bios will give a crc of 0)

		if( GSsetGameCRC != NULL )
			GSsetGameCRC(ElfCRC, g_ZeroGSOptions);
	}

	// Saves recovery state info to the given filename, or saves the active emulation state
	// (if one exists) and no recovery data was found.  This is needed because when a recovery
	// state is made, the emulation state is usually reset so the only persisting state is
	// the one in the memory save. :)
	void SaveToFile( const wxString& file )
	{
		if( g_RecoveryState != NULL )
		{
			// State is already saved into memory, and the emulator (and in-progress flag)
			// have likely been cleared out.  So save from the Recovery buffer instead of
			// doing a "standard" save:

			gzFile fileptr = gzopen( file.ToAscii().data(), "wb" );
			if( fileptr == NULL )
			{
				Msgbox::Alert( wxsFormat( _("Error while trying to save to file: %s"), file.c_str() ) );
				return;
			}
			gzwrite( fileptr, &g_SaveVersion, sizeof( u32 ) );
			gzwrite( fileptr, g_RecoveryState->GetPtr(), g_RecoveryState->GetSizeInBytes() );
			gzclose( fileptr );
		}
		else if( g_gsRecoveryState != NULL )
		{
			RecoveryZipSavingState( file ).FreezeAll();
		}
		else
		{
			if( !g_EmulationInProgress )
			{
				Msgbox::Alert( _("No emulation state to save") ); // translate: You need to start a game first before you can save it's state
				return;
			}

			States_Save( file );
		}
	}

	// Saves recovery state info to the given saveslot, or saves the active emulation state
	// (if one exists) and no recovery data was found.  This is needed because when a recovery
	// state is made, the emulation state is usually reset so the only persisting state is
	// the one in the memory save. :)
	void SaveToSlot( uint num )
	{
		SaveToFile( SaveState::GetFilename( num ) );
	}
	
	// This method will override any existing recovery states, so call it with caution, if you
	// think that there could be existing important state info in the recovery buffers (but
	// really there shouldn't be, unless you're calling this function when it's not intended
	// to be called).
	void MakeGsOnly()
	{
		StateRecovery::Clear();
		
		if( !g_EmulationInProgress ) return;

		g_gsRecoveryState = new SafeArray<u8>();
		JustGsSavingState eddie;
		eddie.FreezePlugin( "GS", gsSafeFreeze ) ;
		eddie.gsFreeze();
	}

	// Creates a full recovery of the entire emulation state (CPU and all plugins).
	// If a current recovery state is already present, then nothing is done (the
	// existing recovery state takes precedence).
	void MakeFull()
	{
		if( g_RecoveryState != NULL ) return;

		try
		{
			g_RecoveryState = new SafeArray<u8>( L"Memory Savestate Recovery" );
			RecoveryMemSavingState().FreezeAll();
			safe_delete( g_gsRecoveryState );
			g_EmulationInProgress = false;
		}
		catch( Exception::RuntimeError& ex )
		{
			Msgbox::Alert( wxsFormat(	// fixme: this error needs proper translation stuffs.
				L"Pcsx2 gamestate recovery failed. Some options may have been reverted to protect your game's state.\n"
				L"Error: %s", ex.DisplayMessage().c_str() )
			);
			safe_delete( g_RecoveryState );
		}
	}

	// Clears and deallocates any recovery states.
	void Clear()
	{
		safe_delete( g_RecoveryState );
		safe_delete( g_gsRecoveryState );
	}
}

RecoveryMemSavingState::RecoveryMemSavingState() : memSavingState( *g_RecoveryState )
{
}

void RecoveryMemSavingState::gsFreeze()
{
	if( g_gsRecoveryState != NULL )
	{
		// just copy the data from src to dst.
		// the normal savestate doesn't expect a length prefix for internal structures,
		// so don't copy that part.
		const u32 pluginlen = *((u32*)g_gsRecoveryState->GetPtr());
		const u32 gslen = *((u32*)g_gsRecoveryState->GetPtr(pluginlen+4));
		memcpy( m_memory.GetPtr(m_idx), g_gsRecoveryState->GetPtr(pluginlen+8), gslen );
		m_idx += gslen;
	}
	else
		memSavingState::gsFreeze();
}

void RecoveryMemSavingState::FreezePlugin( const char* name, s32 (CALLBACK* freezer)(int mode, freezeData *data) )
{
	if( (freezer == gsSafeFreeze) && (g_gsRecoveryState != NULL) )
	{
		// Gs data is already in memory, so just copy from src to dest:
		// length of the GS data is stored as the first u32, so use that to run the copy:
		const u32 len = *((u32*)g_gsRecoveryState->GetPtr());
		memcpy( m_memory.GetPtr(m_idx), g_gsRecoveryState->GetPtr(), len+4 );
		m_idx += len+4;
	}
	else
		memSavingState::FreezePlugin( name, freezer );
}

RecoveryZipSavingState::RecoveryZipSavingState( const wxString& filename ) : gzSavingState( filename )
{
}

void RecoveryZipSavingState::gsFreeze()
{
	if( g_gsRecoveryState != NULL )
	{
		// read data from the gsRecoveryState allocation instead of the GS, since the gs
		// info was invalidated when the plugin was shut down.

		// the normal savestate doesn't expect a length prefix for internal structures,
		// so don't copy that part.

		u32& pluginlen = *((u32*)g_gsRecoveryState->GetPtr(0));
		u32& gslen = *((u32*)g_gsRecoveryState->GetPtr(pluginlen+4));
		gzwrite( m_file, g_gsRecoveryState->GetPtr(pluginlen+4), gslen );
	}
	else
		gzSavingState::gsFreeze();
}

void RecoveryZipSavingState::FreezePlugin( const char* name, s32 (CALLBACK* freezer)(int mode, freezeData *data) )
{
	if( (freezer == gsSafeFreeze) && (g_gsRecoveryState != NULL) )
	{
		// Gs data is already in memory, so just copy from there into the gzip file.
		// length of the GS data is stored as the first u32, so use that to run the copy:
		u32& len = *((u32*)g_gsRecoveryState->GetPtr());
		gzwrite( m_file, g_gsRecoveryState->GetPtr(), len+4 );
	}
	else
		gzSavingState::FreezePlugin( name, freezer );
}

JustGsSavingState::JustGsSavingState() : memSavingState( *g_gsRecoveryState )
{
}

// This special override saves the gs info to m_idx+4, and then goes back and
// writes in the length of data saved.
void JustGsSavingState::gsFreeze()
{
	int oldmidx = m_idx;
	m_idx += 4;
	memSavingState::gsFreeze();
	if( IsSaving() )
	{
		s32& len = *((s32*)m_memory.GetPtr( oldmidx ));
		len = (m_idx - oldmidx)-4;
	}
}
