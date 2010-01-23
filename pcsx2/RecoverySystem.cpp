/*  PCSX2 - PS2 Emulator for PCs
 *  Copyright (C) 2002-2009  PCSX2 Dev Team
 *
 *  PCSX2 is free software: you can redistribute it and/or modify it under the terms
 *  of the GNU Lesser General Public License as published by the Free Software Found-
 *  ation, either version 3 of te License, or (at your option) any later version.
 *
 *  PCSX2 is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY;
 *  without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
 *  PURPOSE.  See the GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License along with PCSX2.
 *  If not, see <http://www.gnu.org/licenses/>.
 */

#include "PrecompiledHeader.h"

#include "zlib/zlib.h"

#include "App.h"
#include "HostGui.h"
#include "AppSaveStates.h"
#include "Utilities/EventSource.inl"

class _BaseStateThread;

template EventSource<IEventListener_SaveStateThread>;
static EventSource<IEventListener_SaveStateThread>		m_evtsrc_SaveState;

// Used to hold the current state backup (fullcopy of PS2 memory and plugin states).
static SafeArray<u8> state_buffer;

_BaseStateThread* current_state_thread = NULL;

// Simple lock boolean for the state buffer being in use by a thread.
static NonblockingMutex state_buffer_lock;

// This boolean tracks if a savestate is actively saving.  When a state is saving we
// typically delay program termination to allow th state time to finish it's work.
static bool state_is_saving = false;

// This boolean is to keep the system from resuming emulation until the current state has completely
// uploaded or downloaded itself.  It is only modified from the main thread, and should only be read
// from the main thread.
int sys_resume_lock = 0;

static FnType_OnThreadComplete* Callback_FreezeFinished = NULL;

static bool StateCopy_ForceClear()
{
	sys_resume_lock = 0;
	state_buffer_lock.Release();
	state_buffer.Dispose();
}

class _BaseStateThread : public PersistentThread,
	public virtual EventListener_AppStatus,
	public virtual IDeletableObject
{
	typedef PersistentThread _parent;

protected:
	bool	m_isStarted;

	// Holds the pause/suspend state of the emulator when the state load/stave chain of action is started,
	// so that the proper state can be restored automatically on completion.
	bool	m_resume_when_done;

public:
	virtual ~_BaseStateThread() throw()
	{
		if( !m_isStarted ) return;

		// Assertion fails because C++ changes the 'this' pointer to the base class since
		// derived classes have been deallocated at this point the destructor!

		//pxAssumeDev( current_state_thread == this, wxCharNull );
		current_state_thread = NULL;
		state_buffer_lock.Release();		// just in case;
	}
	
	virtual bool IsFreezing() const=0;

protected:
	_BaseStateThread( const char* name, FnType_OnThreadComplete* onFinished )
	{
		Callback_FreezeFinished = onFinished;
		m_name					= L"StateThread::" + fromUTF8(name);
		m_isStarted				= false;
		m_resume_when_done		= false;
	}

	void OnStart()
	{
		if( !state_buffer_lock.TryAcquire() )
			throw Exception::CancelEvent( m_name + L"request ignored: state copy buffer is already locked!" );

		current_state_thread = this;
		m_isStarted = true;
		_parent::OnStart();
	}

	void SendFinishEvent( int type )
	{
		wxGetApp().PostCommand( this, pxEvt_FreezeThreadFinished, type, m_resume_when_done );
	}

	void AppStatusEvent_OnExit()
	{
		Cancel();
		wxGetApp().DeleteObject( this );
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

	bool IsFreezing() const { return true; }

protected:
	void OnStart()
	{
		_parent::OnStart();
		++sys_resume_lock;
		m_resume_when_done = CoreThread.Pause();
	}

	void ExecuteTaskInThread()
	{
		memSavingState( state_buffer ).FreezeAll();
	}

	void OnCleanupInThread()
	{
		SendFinishEvent( SaveStateAction_CreateFinished );
		_parent::OnCleanupInThread();
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
	StateThread_ZipToDisk( FnType_OnThreadComplete* onFinished, bool resume_done, const wxString& file )
		: _BaseStateThread( "ZipToDisk", onFinished )
		, m_filename( file )
	{
		m_gzfp				= NULL;
		m_resume_when_done	= resume_done;
	}

	virtual ~StateThread_ZipToDisk() throw()
	{
		if( m_gzfp != NULL ) gzclose( m_gzfp );
	}
	
	bool IsFreezing() const { return true; }	

protected:
	void OnStart()
	{
		_parent::OnStart();
		m_gzfp = gzopen( m_filename.ToUTF8(), "wb" );
		if(	m_gzfp == NULL )
			throw Exception::CreateStream( m_filename, "Cannot create savestate file for writing." );
	}

	void ExecuteTaskInThread()
	{
		Yield( 3 );

		static const int BlockSize = 0x20000;
		int curidx = 0;
		do
		{
			int thisBlockSize = std::min( BlockSize, state_buffer.GetSizeInBytes() - curidx );
			if( gzwrite( m_gzfp, state_buffer.GetPtr(curidx), thisBlockSize ) < thisBlockSize )
				throw Exception::BadStream( m_filename );
			curidx += thisBlockSize;
			Yield( 1 );
		} while( curidx < state_buffer.GetSizeInBytes() );
		
		Console.WriteLn( "State saved to disk without error." );
	}
	
	void OnCleanupInThread()
	{
		SendFinishEvent( SaveStateAction_ZipToDiskFinished );
		_parent::OnCleanupInThread();
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
	StateThread_UnzipFromDisk( FnType_OnThreadComplete* onFinished, bool resume_done, const wxString& file )
		: _BaseStateThread( "UnzipFromDisk", onFinished )
		, m_filename( file )
	{
		m_gzfp				= NULL;
		m_finished			= false;
		m_resume_when_done	= resume_done;
	}

	virtual ~StateThread_UnzipFromDisk() throw()
	{
		if( m_gzfp != NULL ) gzclose( m_gzfp );
	}

	bool IsFreezing() const { return false; }

protected:
	void OnStart()
	{
		_parent::OnStart();

		m_gzfp = gzopen( m_filename.ToUTF8(), "rb" );
		if(	m_gzfp == NULL )
			throw Exception::CreateStream( m_filename, "Cannot open savestate file for reading." );
	}

	void ExecuteTaskInThread()
	{
		// fixme: should start initially with the file size, and then grow from there.

		static const int BlockSize = 0x100000;
		state_buffer.MakeRoomFor( 0x800000 );		// start with an 8 meg buffer to avoid frequent reallocation.

		int curidx = 0;
		do
		{
			state_buffer.MakeRoomFor( curidx+BlockSize );
			gzread( m_gzfp, state_buffer.GetPtr(curidx), BlockSize );
			curidx += BlockSize;
			TestCancel();
		} while( !gzeof(m_gzfp) );
		
		m_finished = true;
	}

	void OnCleanupInThread()
	{
		SendFinishEvent( SaveStateAction_UnzipFromDiskFinished );
		_parent::OnCleanupInThread();
	}
};


void Pcsx2App::OnFreezeThreadFinished( wxCommandEvent& evt )
{
	// clear the OnFreezeFinished to NULL now, in case of error.
	// (but only actually run it if no errors occur)
	FnType_OnThreadComplete* fn_tmp = Callback_FreezeFinished;
	Callback_FreezeFinished = NULL;

	{
		ScopedPtr<PersistentThread> thr( (PersistentThread*)evt.GetClientData() );
		if( !pxAssertDev( thr != NULL, "NULL thread handle on freeze finished?" ) ) return;

		current_state_thread = NULL;
		state_buffer_lock.Release();
		--sys_resume_lock;

		m_evtsrc_SaveState.Dispatch( (SaveStateActionType)evt.GetInt() );
		
		thr->RethrowException();
	}
	
	if( fn_tmp != NULL ) fn_tmp( evt );
}

static void OnFinished_Resume( const wxCommandEvent& evt )
{
	CoreThread.RecoverState();
	if( evt.GetExtraLong() ) CoreThread.Resume();
}

static wxString zip_dest_filename;

static void OnFinished_ZipToDisk( const wxCommandEvent& evt )
{
	if( !pxAssertDev( evt.GetInt() == SaveStateAction_CreateFinished, "Unexpected StateThreadAction value, aborting save." ) ) return;

	if( zip_dest_filename.IsEmpty() )
	{
		Console.Warning( "Cannot save state to disk: empty filename specified." );
		return;
	}

	// Phase 2: Record to disk!!
	(new StateThread_ZipToDisk( NULL, !!evt.GetExtraLong(), zip_dest_filename ))->Start();
	
	CoreThread.Resume();
}

class InvokeAction_WhenSaveComplete :
	public IEventListener_SaveStateThread,
	public IDeletableObject
{
protected:
	IActionInvocation*	m_action;

public:
	InvokeAction_WhenSaveComplete( IActionInvocation* action )
	{
		m_action = action;
	}

	virtual ~InvokeAction_WhenSaveComplete() throw() {}

	void SaveStateAction_OnZipToDiskFinished()
	{
		if( m_action )
		{
			m_action->InvokeAction();
			safe_delete( m_action );
		}
		wxGetApp().DeleteObject( this );
	}
};

class InvokeAction_WhenStateCopyComplete : public InvokeAction_WhenSaveComplete
{
public:
	InvokeAction_WhenStateCopyComplete( IActionInvocation* action )
		: InvokeAction_WhenSaveComplete( action )
	{
	}

	virtual ~InvokeAction_WhenStateCopyComplete() throw() {}

	void SaveStateAction_OnCreateFinished()
	{
		SaveStateAction_OnZipToDiskFinished();
	}
};

// =====================================================================================================
//  StateCopy Public Interface
// =====================================================================================================

bool StateCopy_InvokeOnSaveComplete( IActionInvocation* sst )
{
	AffinityAssert_AllowFromMain();

	if( current_state_thread == NULL || !current_state_thread->IsFreezing() )
	{
		delete sst;
		return false;
	}

	m_evtsrc_SaveState.Add( new InvokeAction_WhenSaveComplete( sst ) );
	return true;
}

bool StateCopy_InvokeOnCopyComplete( IActionInvocation* sst )
{
	AffinityAssert_AllowFromMain();

	if( current_state_thread == NULL || !current_state_thread->IsFreezing() )
	{
		delete sst;
		return false;
	}

	m_evtsrc_SaveState.Add( new InvokeAction_WhenStateCopyComplete( sst ) );
	return true;
}

void StateCopy_SaveToFile( const wxString& file )
{
	if( state_buffer_lock.IsLocked() ) return;
	zip_dest_filename = file;
	(new StateThread_Freeze( OnFinished_ZipToDisk ))->Start();
	Console.WriteLn( Color_StrongGreen, L"Saving savestate to file: %s", zip_dest_filename.c_str() );
}

void StateCopy_LoadFromFile( const wxString& file )
{
	if( state_buffer_lock.IsLocked() ) return;
	bool resume_when_done = CoreThread.Pause();
	(new StateThread_UnzipFromDisk( OnFinished_Resume, resume_when_done, file ))->Start();
}

// Saves recovery state info to the given saveslot, or saves the active emulation state
// (if one exists) and no recovery data was found.  This is needed because when a recovery
// state is made, the emulation state is usually reset so the only persisting state is
// the one in the memory save. :)
void StateCopy_SaveToSlot( uint num )
{
	if( state_buffer_lock.IsLocked() ) return;

	zip_dest_filename = SaveStateBase::GetFilename( num );
	(new StateThread_Freeze( OnFinished_ZipToDisk ))->Start();
	Console.WriteLn( Color_StrongGreen, "Saving savestate to slot %d...", num );
	Console.Indent().WriteLn( Color_StrongGreen, L"filename: %s", zip_dest_filename.c_str() );
}

void StateCopy_LoadFromSlot( uint slot )
{
	if( state_buffer_lock.IsLocked() ) return;
	wxString file( SaveStateBase::GetFilename( slot ) );

	if( !wxFileExists( file ) )
	{
		Console.Warning( "Savestate slot %d is empty.", slot );
		return;
	}

	Console.WriteLn( Color_StrongGreen, "Loading savestate from slot %d...", slot );
	Console.Indent().WriteLn( Color_StrongGreen, L"filename: %s", file.c_str() );

	bool resume_when_done = CoreThread.Pause();
	(new StateThread_UnzipFromDisk( OnFinished_Resume, resume_when_done, file ))->Start();
}

bool StateCopy_IsValid()
{
	return !state_buffer.IsDisposed();
}

const SafeArray<u8>* StateCopy_GetBuffer()
{
	if( state_buffer_lock.IsLocked() || state_buffer.IsDisposed() ) return NULL;
	return &state_buffer;
}

void StateCopy_FreezeToMem()
{
	if( state_buffer_lock.IsLocked() ) return;
	(new StateThread_Freeze( OnFinished_Resume ))->Start();
}

class Acquire_And_Block
{
protected:
	bool	m_DisposeWhenFinished;
	bool	m_Acquired;

public:
	Acquire_And_Block( bool dispose )
	{
		m_DisposeWhenFinished	= dispose;
		m_Acquired				= false;

		/*
		// If the state buffer is locked and we're being called from the main thread then we need
		// to cancel the current action.  This is needed because state_buffer_lock is only updated
		// from events handled on the main thread.

		if( wxThread::IsMain() )
			throw Exception::CancelEvent( "Blocking ThawFromMem canceled due to existing state buffer lock." );
		else*/

		while ( !state_buffer_lock.TryAcquire() )
		{
			pxAssume( current_state_thread != NULL );
			current_state_thread->Block();
			wxGetApp().ProcessPendingEvents();		// Trying this for now, may or may not work due to recursive pitfalls (see above)
		};

		m_Acquired = true;
	}
	
	virtual ~Acquire_And_Block() throw()
	{
		if( m_DisposeWhenFinished )
			state_buffer.Dispose();
		
		if( m_Acquired )
			state_buffer_lock.Release();
	}
};

void StateCopy_FreezeToMem_Blocking()
{
	Acquire_And_Block blocker( false );
	memSavingState( state_buffer ).FreezeAll();
}

// Copies the saved state into the active VM, and automatically free's the saved state data.
void StateCopy_ThawFromMem_Blocking()
{
	Acquire_And_Block blocker( true );
	memLoadingState( state_buffer ).FreezeAll();
}

void StateCopy_Clear()
{
	if( state_buffer_lock.IsLocked() ) return;
	state_buffer.Dispose();
}

bool StateCopy_IsBusy()
{
	return state_buffer_lock.IsLocked();
}
