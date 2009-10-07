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

// Simple lock boolean for the state buffer being in use by a thread.
static NonblockingMutex state_buffer_lock;

// This boolean is to keep the system from resuming emulation until the current state has completely
// uploaded or downloaded itself.  It is only modified from the main thread, and should only be read
// form the main thread.
bool sys_resume_lock = false;

static FnType_OnThreadComplete* Callback_FreezeFinished = NULL;

enum
{
	StateThreadAction_None = 0,
	StateThreadAction_Create,
	StateThreadAction_Restore,
	StateThreadAction_ZipToDisk,
	StateThreadAction_UnzipFromDisk,
};

class _BaseStateThread : public PersistentThread
{
	typedef PersistentThread _parent;

public:
	virtual ~_BaseStateThread() throw()
	{
		state_buffer_lock.Release();		// just in case;
	}

protected:
	_BaseStateThread( const char* name, FnType_OnThreadComplete* onFinished )
	{
		Callback_FreezeFinished = onFinished;
		m_name = L"StateThread::" + fromUTF8(name);
	}

	void OnStart()
	{
		if( !state_buffer_lock.TryLock() )
			throw Exception::CancelEvent( m_name + L"request ignored: state copy buffer is already locked!" );
	}

	void SendFinishEvent( int type )
	{
		wxCommandEvent evt( pxEVT_FreezeThreadFinished );
		evt.SetClientData( this );
		evt.SetInt( type );
		wxGetApp().AddPendingEvent( evt );
	}

};

// --------------------------------------------------------------------------------------
//  StateThread_Freeze
// --------------------------------------------------------------------------------------
class StateThread_Freeze : public _BaseStateThread
{
	typedef _BaseStateThread _parent;
	
public:
	StateThread_Freeze( FnType_OnThreadComplete* onFinished ) : _BaseStateThread( "Freeze", onFinished )
	{
		if( !SysHasValidState() )
			throw Exception::RuntimeError( L"Cannot complete state freeze request; the virtual machine state is reset.", _("You'll need to start a new virtual machine before you can save its state.") );
	}

protected:
	void OnStart()
	{
		_parent::OnStart();
		sys_resume_lock = true;
		sCoreThread.Pause();
	}

	void ExecuteTask()
	{
		memSavingState( state_buffer ).FreezeAll();
	}

	void OnThreadCleanup()
	{
		SendFinishEvent( StateThreadAction_Create );
		_parent::OnThreadCleanup();
	}
};

// --------------------------------------------------------------------------------------
//   StateThread_Thaw
// --------------------------------------------------------------------------------------
class StateThread_Thaw : public _BaseStateThread
{
	typedef _BaseStateThread _parent;

public:
	StateThread_Thaw( FnType_OnThreadComplete* onFinished ) : _BaseStateThread( "Thaw", onFinished ) { }
	
protected:
	void OnStart()
	{
		_parent::OnStart();

		if( state_buffer.IsDisposed() )
		{
			state_buffer_lock.Release();
			throw Exception::RuntimeError( "ThawState request made, but no valid state exists!" );
		}

		sys_resume_lock = true;
		sCoreThread.Pause();
	}

	void ExecuteTask()
	{
		memLoadingState( state_buffer ).FreezeAll();
	}
	
	void OnThreadCleanup()
	{
		SendFinishEvent( StateThreadAction_Restore );
		_parent::OnThreadCleanup();
	}
};

// --------------------------------------------------------------------------------------
//   StateThread_ZipToDisk
// --------------------------------------------------------------------------------------
class StateThread_ZipToDisk : public _BaseStateThread
{
	typedef _BaseStateThread _parent;

protected:
	const wxString	m_filename;
	gzFile			m_gzfp;

public:
	StateThread_ZipToDisk( FnType_OnThreadComplete* onFinished, const wxString& file ) :
		_BaseStateThread( "ZipToDisk", onFinished )
	,	m_filename( file )
	,	m_gzfp( NULL )
	{
	}

	~StateThread_ZipToDisk() throw()
	{
		if( m_gzfp != NULL ) gzclose( m_gzfp );
	}

protected:
	void OnStart()
	{
		_parent::OnStart();
		m_gzfp = gzopen( toUTF8(m_filename), "wb" );
		if(	m_gzfp == NULL )
			throw Exception::CreateStream( m_filename, "Cannot create savestate file for writing." );
	}

	void ExecuteTask()
	{
		Yield( 2 );
		if( gzwrite( (gzFile)m_gzfp, state_buffer.GetPtr(), state_buffer.GetSizeInBytes() ) < state_buffer.GetSizeInBytes() )
			throw Exception::BadStream();
	}
	
	void OnThreadCleanup()
	{
		SendFinishEvent( StateThreadAction_ZipToDisk );
		_parent::OnThreadCleanup();
	}
};


// --------------------------------------------------------------------------------------
//   StateThread_UnzipFromDisk
// --------------------------------------------------------------------------------------
class StateThread_UnzipFromDisk : public _BaseStateThread
{
	typedef _BaseStateThread _parent;

protected:
	const wxString	m_filename;
	gzFile			m_gzfp;

	// set true only once the whole file has finished loading.  IF the thread is canceled or
	// an error occurs, this will remain false.
	bool			m_finished;
	
public:
	StateThread_UnzipFromDisk( FnType_OnThreadComplete* onFinished, const wxString& file ) :
		_BaseStateThread( "UnzipFromDisk", onFinished )
	,	m_filename( file )
	,	m_gzfp( NULL )
	,	m_finished( false )
	{
	}

	~StateThread_UnzipFromDisk() throw()
	{
		if( m_gzfp != NULL ) gzclose( m_gzfp );
	}

protected:
	void OnStart()
	{
		_parent::OnStart();

		m_gzfp = gzopen( toUTF8(m_filename), "rb" );
		if(	m_gzfp == NULL )
			throw Exception::CreateStream( m_filename, "Cannot open savestate file for reading." );
	}

	void ExecuteTask()
	{
		// fixme: should start initially with the file size, and then grow from there.

		static const int BlockSize = 327680;
		int curidx = 0;
		do
		{
			state_buffer.ExactAlloc( curidx+BlockSize );
			gzread( m_gzfp, state_buffer.GetPtr(curidx), BlockSize );
			curidx += BlockSize;
			TestCancel();
		} while( !gzeof(m_gzfp) );
		
		m_finished = true;
	}

	void OnThreadCleanup()
	{
		SendFinishEvent( StateThreadAction_UnzipFromDisk );
		_parent::OnThreadCleanup();
	}
};

void Pcsx2App::OnFreezeThreadFinished( wxCommandEvent& evt )
{
	// clear the OnFreezeFinsihed to NULL now, in case of error.
	// (but only actually run it if no errors occur)
	FnType_OnThreadComplete* fn_tmp = Callback_FreezeFinished;
	Callback_FreezeFinished = NULL;

	{
		ScopedPtr<PersistentThread> thr( (PersistentThread*)evt.GetClientData() );
		if( !pxAssertDev( thr != NULL, "NULL thread handle on freeze finished?" ) ) return;
		state_buffer_lock.Release();
		sys_resume_lock = false;
		thr->RethrowException();
	}
	
	if( fn_tmp != NULL ) fn_tmp( evt );

	//m_evtsrc_FreezeThreadFinished.Dispatch( evt );
}

void OnFinished_Resume( const wxCommandEvent& evt )
{
	if( evt.GetInt() == StateThreadAction_Restore )
	{
		// Successfully restored state, so remove the copy.  Don't remove it sooner
		// because the thread may have failed with some exception/error.

		state_buffer.Dispose();
		SysClearExecutionCache();
	}

	sCoreThread.Resume();
}

static wxString zip_dest_filename;

void OnFinished_ZipToDisk( const wxCommandEvent& evt )
{
	if( !pxAssertDev( evt.GetInt() == StateThreadAction_Create, "Unexpected StateThreadAction value, aborting save." ) ) return;

	if( zip_dest_filename.IsEmpty() )
	{
		Console.Notice( "Cannot save state to disk: empty filename specified." );
		return;
	}
		
	// Phase 2: Record to disk!!
	(new StateThread_ZipToDisk( OnFinished_Resume, zip_dest_filename ))->Start();
}

void OnFinished_Restore( const wxCommandEvent& evt )
{
	if( !pxAssertDev( evt.GetInt() == StateThreadAction_UnzipFromDisk, "Unexpected StateThreadAction value, aborting restore." ) ) return;

	// Phase 2: Restore over existing VM state!!
	(new StateThread_Thaw( OnFinished_Resume ))->Start();
}


// =====================================================================================================
//  StateCopy Public Interface
// =====================================================================================================

void StateCopy_SaveToFile( const wxString& file )
{
	if( state_buffer_lock.IsLocked() ) return;
	zip_dest_filename = file;
	(new StateThread_Freeze( OnFinished_ZipToDisk ))->Start();
	Console.Status( wxsFormat( L"Saving savestate to file: %s", zip_dest_filename.c_str() ) );
}

void StateCopy_LoadFromFile( const wxString& file )
{
	if( state_buffer_lock.IsLocked() ) return;
	sCoreThread.Pause();
	(new StateThread_UnzipFromDisk( OnFinished_Restore, file ))->Start();
}

// Saves recovery state info to the given saveslot, or saves the active emulation state
// (if one exists) and no recovery data was found.  This is needed because when a recovery
// state is made, the emulation state is usually reset so the only persisting state is
// the one in the memory save. :)
void StateCopy_SaveToSlot( uint num )
{
	zip_dest_filename = SaveStateBase::GetFilename( num );
	(new StateThread_Freeze( OnFinished_ZipToDisk ))->Start();
	Console.Status( "Saving savestate to slot %d...", num );
	Console.Status( wxsFormat(L"\tfilename: %s", zip_dest_filename.c_str()) );
}

void StateCopy_LoadFromSlot( uint slot )
{
	if( state_buffer_lock.IsLocked() ) return;
	wxString file( SaveStateBase::GetFilename( slot ) );

	if( !wxFileExists( file ) )
	{
		Console.Notice( "Savestate slot %d is empty.", slot );
		return;
	}

	Console.Status( "Loading savestate from slot %d...", slot );
	Console.Status( wxsFormat(L"\tfilename: %s", file.c_str()) );

	sCoreThread.Pause();
	(new StateThread_UnzipFromDisk( OnFinished_Restore, file ))->Start();
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
	if( state_buffer_lock.IsLocked() ) return;
	(new StateThread_Freeze( OnFinished_Restore ))->Start();
}

void StateCopy_ThawFromMem()
{
	if( state_buffer_lock.IsLocked() ) return;
	new StateThread_Thaw( OnFinished_Restore );
}

void StateCopy_Clear()
{
	if( state_buffer_lock.IsLocked() ) return;
	state_buffer.Dispose();
}

