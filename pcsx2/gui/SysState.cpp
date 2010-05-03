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
#include "App.h"

#include "System/SysThreads.h"
#include "SaveState.h"

#include "ZipTools/ThreadedZipTools.h"

// Used to hold the current state backup (fullcopy of PS2 memory and plugin states).
static VmStateBuffer state_buffer( L"Public Savestate Buffer" );

static const char SavestateIdentString[] = "PCSX2 Savestate";
static const uint SavestateIdentLen = sizeof(SavestateIdentString);

static void SaveStateFile_WriteHeader( IStreamWriter& thr )
{
	thr.Write( SavestateIdentString );
	thr.Write( g_SaveVersion );	
}

static void SaveStateFile_ReadHeader( IStreamReader& thr )
{
	char ident[SavestateIdentLen] = {0};

	thr.Read( ident );

	if( strcmp(SavestateIdentString, ident) )
		throw Exception::SaveStateLoadError( thr.GetStreamName(),
			wxsFormat( L"Unrecognized file signature while loading savestate: %s", ident ),
			_("File is not a valid PCSX2 savestate, or is from an older unsupported version of PCSX2.")
		);

	u32 savever;
	thr.Read( savever );
	
	if( (savever >> 16) != (g_SaveVersion >> 16) )
		throw Exception::SaveStateLoadError( thr.GetStreamName(),
			wxsFormat( L"Unrecognized file signature while loading savestate: %s", ident ),
			_("File is not a valid PCSX2 savestate, or is from an older unsupported version of PCSX2.")
		);
	
	if( savever > g_SaveVersion )
		throw Exception::SaveStateLoadError( thr.GetStreamName(),
			wxsFormat( L"Unrecognized file signature while loading savestate: %s", ident ),
			_("File is not a valid PCSX2 savestate, or is from an older unsupported version of PCSX2.")
		);
};

// --------------------------------------------------------------------------------------
//  gzipReader
// --------------------------------------------------------------------------------------
// Interface for reading data from a gzip stream.
//
class gzipReader : public IStreamReader
{
	DeclareNoncopyableObject(gzipReader);

protected:
	wxString		m_filename;
	gzFile			m_gzfp;

public:
	gzipReader( const wxString& filename )
		: m_filename( filename )
	{
		if(	NULL == (m_gzfp = gzopen( m_filename.ToUTF8(), "rb" )) )
			throw Exception::CannotCreateStream( m_filename, "Cannot open file for reading." );

		gzbuffer(m_gzfp, 0x100000); // 1mb buffer for zlib internal operations
	}
	
	virtual ~gzipReader() throw ()
	{
		if( m_gzfp ) gzclose( m_gzfp );
	}

	wxString GetStreamName() const { return m_filename; }

	void Read( void* dest, size_t size )
	{
		int result = gzread( m_gzfp, dest, size );
		if( result == -1)
			throw Exception::BadStream( m_filename, "Data read failed: Invalid or corrupted gzip archive." );

		if( (size_t)result < size )
			throw Exception::EndOfStream( m_filename );
	}
};

static bool IsSavingOrLoading = false;


// --------------------------------------------------------------------------------------
//  SysExecEvent_DownloadState
// --------------------------------------------------------------------------------------
// Pauses core emulation and downloads the savestate into the state_buffer.
//
class SysExecEvent_DownloadState : public SysExecEvent
{
protected:
	VmStateBuffer*	m_dest_buffer;

public:
	wxString GetEventName() const { return L"VM_Download"; }

	virtual ~SysExecEvent_DownloadState() throw() {}
	SysExecEvent_DownloadState* Clone() const { return new SysExecEvent_DownloadState( *this ); }
	SysExecEvent_DownloadState( VmStateBuffer* dest=&state_buffer )
	{
		m_dest_buffer = dest;
	}

	bool IsCriticalEvent() const { return true; }
	bool AllowCancelOnExit() const { return false; }
	
protected:
	void InvokeEvent()
	{
		ScopedCoreThreadPause paused_core;

		if( !SysHasValidState() )
			throw Exception::RuntimeError( L"Cannot complete state freeze request; the virtual machine state is reset.", _("You'll need to start a new virtual machine before you can save its state.") );

		memSavingState(m_dest_buffer).FreezeAll();

		UI_EnableStateActions();
		paused_core.AllowResume();
	}
};

// It's bad mojo to have savestates trying to read and write from the same file at the
// same time.  To prevent that we use this mutex lock, which is used by both the
// CompressThread and the UnzipFromDisk events.  (note that CompressThread locks the
// mutex during OnStartInThread, which ensures that the ZipToDisk event blocks; preventing
// the SysExecutor's Idle Event from re-enabing savestates and slots.)
//
static Mutex mtx_CompressToDisk;

// --------------------------------------------------------------------------------------
//  CompressThread_VmState
// --------------------------------------------------------------------------------------
class VmStateZipThread : public CompressThread_gzip
{
	typedef CompressThread_gzip _parent;

protected:
	ScopedLock		m_lock_Compress;

public:
	VmStateZipThread( const wxString& file, VmStateBuffer* srcdata )
		: _parent( file, srcdata, SaveStateFile_WriteHeader )
	{
		m_lock_Compress.Assign(mtx_CompressToDisk);
	}

	VmStateZipThread( const wxString& file, ScopedPtr<VmStateBuffer>& srcdata )
		: _parent( file, srcdata, SaveStateFile_WriteHeader )
	{
		m_lock_Compress.Assign(mtx_CompressToDisk);
	}

	virtual ~VmStateZipThread() throw()
	{
		
	}
	
protected:
	void OnStartInThread()
	{
		_parent::OnStartInThread();
		m_lock_Compress.Acquire();
	}

	void OnCleanupInThread()
	{
		m_lock_Compress.Release();
		_parent::OnCleanupInThread();
	}
};

// --------------------------------------------------------------------------------------
//  SysExecEvent_ZipToDisk
// --------------------------------------------------------------------------------------
class SysExecEvent_ZipToDisk : public SysExecEvent
{
protected:
	VmStateBuffer*		m_src_buffer;
	wxString			m_filename;

public:
	wxString GetEventName() const { return L"VM_ZipToDisk"; }

	virtual ~SysExecEvent_ZipToDisk() throw()
	{
		delete m_src_buffer;
	}

	SysExecEvent_ZipToDisk* Clone() const { return new SysExecEvent_ZipToDisk( *this ); }

	SysExecEvent_ZipToDisk( ScopedPtr<VmStateBuffer>& src, const wxString& filename )
		: m_filename( filename )
	{
		m_src_buffer = src.DetachPtr();
	}

	SysExecEvent_ZipToDisk( VmStateBuffer* src, const wxString& filename )
		: m_filename( filename )
	{
		m_src_buffer = src;
	}

	bool IsCriticalEvent() const { return true; }
	bool AllowCancelOnExit() const { return false; }

protected:
	void InvokeEvent()
	{
		 (new VmStateZipThread( m_filename, m_src_buffer ))->Start();
		 m_src_buffer = NULL;
	}
};

// --------------------------------------------------------------------------------------
//  SysExecEvent_UnzipFromDisk
// --------------------------------------------------------------------------------------
// Note: Unzipping always goes directly into the state_buffer, and is always a blocking
// action on the SysExecutor thread (the system cannot execute other commands while states
// are unzipping or uplading into the system).
//
class SysExecEvent_UnzipFromDisk : public SysExecEvent
{
protected:
	wxString		m_filename;

public:
	wxString GetEventName() const { return L"VM_UnzipFromDisk"; }
	
	virtual ~SysExecEvent_UnzipFromDisk() throw() {}
	SysExecEvent_UnzipFromDisk* Clone() const { return new SysExecEvent_UnzipFromDisk( *this ); }
	SysExecEvent_UnzipFromDisk( const wxString& filename )
		: m_filename( filename )
	{
	}

	wxString GetStreamName() const { return m_filename; }

protected:
	void InvokeEvent()
	{
		ScopedLock lock( mtx_CompressToDisk );

		gzipReader m_gzreader(m_filename );
		SaveStateFile_ReadHeader( m_gzreader );

		// We use direct Suspend/Resume control here, since it's desirable that emulation
		// *ALWAYS* start execution after the new savestate is loaded.

		GetCoreThread().Pause();

		// fixme: should start initially with the file size, and then grow from there.

		static const int BlockSize = 0x100000;
		state_buffer.MakeRoomFor( 0x800000 );		// start with an 8 meg buffer to avoid frequent reallocation.
		int curidx = 0;

		try {
			while(true) {
				state_buffer.MakeRoomFor( curidx+BlockSize );
				m_gzreader.Read( state_buffer.GetPtr(curidx), BlockSize );
				curidx += BlockSize;
				Threading::pxTestCancel();
			}
		}
		catch( Exception::EndOfStream& )
		{
			// This exception actually means success!  Any others we let get sent
			// to the main event handler/thread for handling.
		}

		// Optional shutdown of plugins when loading states?  I'm not implementing it yet because some
		// things, like the SPU2-recovery trick, rely on not resetting the plugins prior to loading
		// the new savestate data.
		
		//if( ShutdownOnStateLoad ) GetCoreThread().Cancel();
		GetCoreThread().RecoverState();
		GetCoreThread().Resume();	// force resume regardless of emulation state earlier.
	}
};

// =====================================================================================================
//  StateCopy Public Interface
// =====================================================================================================

VmStateBuffer& StateCopy_GetBuffer()
{
	return state_buffer;
}

bool StateCopy_IsValid()
{
	return !state_buffer.IsDisposed();
}

void StateCopy_FreezeToMem()
{
	GetSysExecutorThread().PostEvent( new SysExecEvent_DownloadState() );
}

void StateCopy_SaveToFile( const wxString& file )
{
	UI_DisableStateActions();

	ScopedPtr<VmStateBuffer> zipbuf(new VmStateBuffer( L"Zippable Savestate" ));
	GetSysExecutorThread().PostEvent(new SysExecEvent_DownloadState( zipbuf ));
	GetSysExecutorThread().PostEvent(new SysExecEvent_ZipToDisk( zipbuf, file ));
}

void StateCopy_LoadFromFile( const wxString& file )
{
	UI_DisableSysActions();
	GetSysExecutorThread().PostEvent(new SysExecEvent_UnzipFromDisk( file ));
}

// Saves recovery state info to the given saveslot, or saves the active emulation state
// (if one exists) and no recovery data was found.  This is needed because when a recovery
// state is made, the emulation state is usually reset so the only persisting state is
// the one in the memory save. :)
void StateCopy_SaveToSlot( uint num )
{
	const wxString file( SaveStateBase::GetFilename( num ) );

	Console.WriteLn( Color_StrongGreen, "Saving savestate to slot %d...", num );
	Console.Indent().WriteLn( Color_StrongGreen, L"filename: %s", file.c_str() );

	StateCopy_SaveToFile( file );
}

void StateCopy_LoadFromSlot( uint slot )
{
	wxString file( SaveStateBase::GetFilename( slot ) );

	if( !wxFileExists( file ) )
	{
		Console.Warning( "Savestate slot %d is empty.", slot );
		return;
	}

	Console.WriteLn( Color_StrongGreen, "Loading savestate from slot %d...", slot );
	Console.Indent().WriteLn( Color_StrongGreen, L"filename: %s", file.c_str() );

	StateCopy_LoadFromFile( file );
}

void StateCopy_Clear()
{
	state_buffer.Dispose();
}

