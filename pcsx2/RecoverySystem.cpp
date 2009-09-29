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

static SafeArray<u8> state_buffer;

// Simple lock boolean for the state buffer in use by a thread.  This simple solution works because
// we are assured that state save/load actions will only be initiated from the main thread.
static bool state_buffer_lock = false;

// This boolean is to keep the system from resuming emulation until the current state has completely
// uploaded or downloaded itself.  It is only modified from the main thread, and should only be read
// form the main thread.
bool sys_resume_lock = false;

class StateThread_Freeze : public PersistentThread
{
	typedef PersistentThread _parent;

public:
	StateThread_Freeze( const wxString& file )
	{
		m_name = L"SaveState::CopyAndZip";

		DevAssert( wxThread::IsMain(), "StateThread creation is allowed from the Main thread only." );
		if( state_buffer_lock )
			throw Exception::RuntimeError( "Cannot save state; a previous save or load action is already in progress." );

		Start();
		sys_resume_lock = true;
	}
	
protected:
	sptr ExecuteTask()
	{
		memSavingState( state_buffer ).FreezeAll();
		return 0;
	}
	
	void DoThreadCleanup()
	{
		wxCommandEvent evt( pxEVT_FreezeFinished );
		evt.SetClientData( this );
		wxGetApp().AddPendingEvent( evt );

		_parent::DoThreadCleanup();
	}
};

class StateThread_ZipToDisk : public PersistentThread
{
	typedef PersistentThread _parent;

protected:
	gzFile m_gzfp;

public:
	StateThread_ZipToDisk( const wxString& file ) : m_gzfp( NULL )
	{
		m_name = L"SaveState::ZipToDisk";

		DevAssert( wxThread::IsMain(), "StateThread creation is allowed from the Main thread only." );
		if( state_buffer_lock )
			throw Exception::RuntimeError( "Cannot save state; a previous save or load action is already in progress." );

		m_gzfp = gzopen( file.ToUTF8().data(), "wb" );
		if(	m_gzfp == NULL )
			throw Exception::CreateStream( file, "Cannot create savestate file for writing." );

		try{ Start(); }
		catch(...)
		{
			gzclose( m_gzfp ); m_gzfp = NULL;
			throw;
		}
		sys_resume_lock = true;
	}

	~StateThread_ZipToDisk() throw()
	{
		sys_resume_lock = false;		// just in case;
		if( m_gzfp != NULL ) gzclose( m_gzfp );
	}

protected:
	sptr ExecuteTask()
	{
		Sleep( 2 );
		if( gzwrite( (gzFile)m_gzfp, state_buffer.GetPtr(), state_buffer.GetSizeInBytes() ) < state_buffer.GetSizeInBytes() )
			throw Exception::BadStream();

		return 0;
	}
	
	void DoThreadCleanup()
	{
		wxCommandEvent evt( pxEVT_FreezeFinished );
		evt.SetClientData( this );		// tells message to clean us up.
		wxGetApp().AddPendingEvent( evt );

		_parent::DoThreadCleanup();
	}
};


class StateThread_UnzipFromDisk : public PersistentThread
{
	typedef PersistentThread _parent;

protected:
	gzFile m_gzfp;

public:
	StateThread_UnzipFromDisk( const wxString& file ) : m_gzfp( NULL )
	{
		m_name = L"SaveState::UnzipFromDisk";

		DevAssert( wxThread::IsMain(), "StateThread creation is allowed from the Main thread only." );
		if( state_buffer_lock )
			throw Exception::RuntimeError( "Cannot save state; a previous save or load action is already in progress." );

		m_gzfp = gzopen( file.ToUTF8().data(), "wb" );
		if(	m_gzfp == NULL )
			throw Exception::CreateStream( file, "Cannot create savestate file for writing." );

		try{ Start(); }
		catch(...)
		{
			gzclose( m_gzfp ); m_gzfp = NULL;
			throw;
		}
		sys_resume_lock = true;
	}

	~StateThread_UnzipFromDisk() throw()
	{
		sys_resume_lock = false;		// just in case;
		if( m_gzfp != NULL ) gzclose( m_gzfp );
	}

protected:
	sptr ExecuteTask()
	{
		// fixme: should start initially with the file size, and then grow from there.

		static const int BlockSize = 327680;
		int curidx = 0;
		do
		{
			state_buffer.ExactAlloc( curidx+BlockSize );
			gzread( m_gzfp, state_buffer.GetPtr(curidx), BlockSize );
			curidx += BlockSize;
		} while( !gzeof(m_gzfp) );
	}

	void DoThreadCleanup()
	{
		wxCommandEvent evt( pxEVT_ThawFinished );
		evt.SetClientData( this );		// tells message to clean us up.
		wxGetApp().AddPendingEvent( evt );

		_parent::DoThreadCleanup();
	}
};

void Pcsx2App::OnFreezeFinished( wxCommandEvent& evt )
{
	state_buffer.Dispose();
	state_buffer_lock = false;

	SysClearExecutionCache();
	SysResume();
	
	if( PersistentThread* thread = (PersistentThread*)evt.GetClientData() )
	{
		delete thread;
	}
}

void Pcsx2App::OnThawFinished( wxCommandEvent& evt )
{
	state_buffer.Dispose();
	state_buffer_lock = false;

	SysClearExecutionCache();
	SysResume();

	if( PersistentThread* thread = (PersistentThread*)evt.GetClientData() )
	{
		delete thread;
	}
}

void StateCopy_SaveToFile( const wxString& file )
{
	if( state_buffer_lock ) return;
	// [TODO] Implement optional 7zip compression here?
}

// Saves recovery state info to the given saveslot, or saves the active emulation state
// (if one exists) and no recovery data was found.  This is needed because when a recovery
// state is made, the emulation state is usually reset so the only persisting state is
// the one in the memory save. :)
void StateCopy_SaveToSlot( uint num )
{
	if( state_buffer_lock ) return;
	StateCopy_SaveToFile( SaveStateBase::GetFilename( num ) );
}

bool StateCopy_IsValid()
{
	return !state_buffer.IsDisposed();
}

bool StateCopy_HasFullState()
{
	return false;
}

bool StateCopy_HasPartialState()
{
	return false;
}

void StateCopy_FreezeToMem()
{
	if( state_buffer_lock ) return;
}

void StateCopy_ThawFromMem()
{
	if( state_buffer_lock ) return;
}

void StateCopy_Clear()
{
	if( state_buffer_lock ) return;
	state_buffer.Dispose();
}



// Creates a full recovery of the entire emulation state (CPU and all plugins).
// If a current recovery state is already present, then nothing is done (the
// existing recovery state takes precedence since if it were out-of-date it'd be
// deleted!).
void MakeFull()
{
	//if( g_RecoveryState ) return;
	//if( !SysHasValidState() ) return;

	/*
	try
	{
		g_RecoveryState = new SafeArray<u8>( L"Memory Savestate Recovery" );
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
		g_RecoveryState = NULL;
	}*/
}

