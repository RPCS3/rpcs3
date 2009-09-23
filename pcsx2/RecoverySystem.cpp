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
#include "zlib/zlib.h"

using namespace Threading;

static wxScopedPtr< SafeArray<u8> > g_RecoveryState;

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
	
	SafeArray<u8> gzSavingBuffer;

	class gzThreadClass : public PersistentThread
	{
		typedef PersistentThread _parent;

	protected:
		gzFile m_file;

	public:
		gzThreadClass( const wxString& file ) :
			m_file( gzopen( file.ToUTF8().data(), "wb" ) )
		{
			if( m_file == NULL )
				throw Exception::CreateStream( file, "Cannot create savestate file for writing." );
				
			Start();
		}
		
		virtual void DoThreadCleanup()
		{
			gzSavingBuffer.Dispose();
			if( m_file != NULL )
			{
				gzclose( m_file );
				m_file = NULL;
			}
			
			_parent::DoThreadCleanup();
		}
		
		virtual ~gzThreadClass() throw()
		{
			// fixme: something a little more graceful than Block, perhaps?
			Block();
		}
		
	protected:
		int ExecuteTask()
		{
			if( (m_file == NULL) || (gzSavingBuffer.GetSizeInBytes() == 0) ) return 0 ;
			SetName( "Savestate::gzipper" );
			gzwrite( m_file, gzSavingBuffer.GetPtr(), gzSavingBuffer.GetSizeInBytes() );
			
			return 0;
		}
	};

	wxScopedPtr<gzThreadClass> gzThread;

	void SaveToFile( const wxString& file )
	{
		SysSuspend( false );
		gzThread.reset( NULL );		// blocks on any existing gzipping business.

		memSavingState( gzSavingBuffer ).FreezeAll();

		// start that encoding thread:
		gzThread.reset( new gzThreadClass( file ) );
		
		SysResume();
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
