/*  PCSX2 - PS2 Emulator for PCs
 *  Copyright (C) 2002-2010  PCSX2 Dev Team
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
#include "MemoryTypes.h"
#include "App.h"

#include "System/SysThreads.h"
#include "SaveState.h"
#include "VUmicro.h"

#include "ZipTools/ThreadedZipTools.h"

#include <wx/wfstream.h>

// Used to hold the current state backup (fullcopy of PS2 memory and plugin states).
//static VmStateBuffer state_buffer( L"Public Savestate Buffer" );

static const wxChar* EntryFilename_StateVersion			= L"PCSX2 Savestate Version.id";
static const wxChar* EntryFilename_Screenshot			= L"Screenshot.jpg";
static const wxChar* EntryFilename_InternalStructures	= L"PCSX2 Internal Structures.bin";


// --------------------------------------------------------------------------------------
//  SavestateEntry_* (EmotionMemory, IopMemory, etc)
// --------------------------------------------------------------------------------------
// Implementation Rationale:
//  The address locations of PS2 virtual memory components is fully dynamic, so we need to
//  resolve the pointers at the time they are requested (eeMem, iopMem, etc).  Thusly, we
//  cannot use static struct member initializers -- we need virtual functions that compute
//  and resolve the addresses on-demand instead... --air

class SavestateEntry_EmotionMemory : public BaseArchiveEntry
{
public:
	wxString GetFilename() const		{ return L"eeMemory.bin"; }
	u8* GetDataPtr() const				{ return eeMem->Main; }
	uint GetDataSize() const			{ return sizeof(eeMem->Main); }
};

class SavestateEntry_IopMemory : public BaseArchiveEntry
{
public:
	wxString GetFilename() const		{ return L"iopMemory.bin"; }
	u8* GetDataPtr() const				{ return iopMem->Main; }
	uint GetDataSize() const			{ return sizeof(iopMem->Main); }
};

class SavestateEntry_HwRegs : public BaseArchiveEntry
{
public:
	wxString GetFilename() const		{ return L"eeHwRegs.bin"; }
	u8* GetDataPtr() const				{ return eeHw; }
	uint GetDataSize() const			{ return sizeof(eeHw); }
};

class SavestateEntry_IopHwRegs : public BaseArchiveEntry
{
public:
	wxString GetFilename() const		{ return L"iopHwRegs.bin"; }
	u8* GetDataPtr() const				{ return iopHw; }
	uint GetDataSize() const			{ return sizeof(iopHw); }
};

class SavestateEntry_Scratchpad : public BaseArchiveEntry
{
public:
	wxString GetFilename() const		{ return L"Scratchpad.bin"; }
	u8* GetDataPtr() const				{ return eeMem->Scratch; }
	uint GetDataSize() const			{ return sizeof(eeMem->Scratch); }
};

class SavestateEntry_VU0mem : public BaseArchiveEntry
{
public:
	wxString GetFilename() const		{ return L"vu0Memory.bin"; }
	u8* GetDataPtr() const				{ return vuRegs[0].Mem; }
	uint GetDataSize() const			{ return VU0_MEMSIZE; }
};

class SavestateEntry_VU1mem : public BaseArchiveEntry
{
public:
	wxString GetFilename() const		{ return L"vu1Memory.bin"; }
	u8* GetDataPtr() const				{ return vuRegs[1].Mem; }
	uint GetDataSize() const			{ return VU1_MEMSIZE; }
};

class SavestateEntry_VU0prog : public BaseArchiveEntry
{
public:
	wxString GetFilename() const		{ return L"vu0Programs.bin"; }
	u8* GetDataPtr() const				{ return vuRegs[0].Micro; }
	uint GetDataSize() const			{ return VU0_PROGSIZE; }
};

class SavestateEntry_VU1prog : public BaseArchiveEntry
{
public:
	wxString GetFilename() const		{ return L"vu1Programs.bin"; }
	u8* GetDataPtr() const				{ return vuRegs[1].Micro; }
	uint GetDataSize() const			{ return VU1_PROGSIZE; }
};

// [TODO] : Add other components as files to the savestate gzip?
//  * VU0/VU1 memory banks?  VU0prog, VU1prog, VU0data, VU1data.
//  * GS register data?
//  * Individual plugins?
// (cpuRegs, iopRegs, VPU/GIF/DMAC structures should all remain as part of a larger unified
//  block, since they're all PCSX2-dependent and having separate files in the archie for them
//  would not be useful).
//

static const BaseArchiveEntry* const SavestateEntries[] = 
{
	new SavestateEntry_EmotionMemory,
	new SavestateEntry_IopMemory,
	new SavestateEntry_HwRegs,
	new SavestateEntry_IopHwRegs,
	new SavestateEntry_Scratchpad,
	new SavestateEntry_VU0mem,
	new SavestateEntry_VU1mem,
	new SavestateEntry_VU0prog,
	new SavestateEntry_VU1prog,
};

static const uint NumSavestateEntries = ArraySize(SavestateEntries);

// It's bad mojo to have savestates trying to read and write from the same file at the
// same time.  To prevent that we use this mutex lock, which is used by both the
// CompressThread and the UnzipFromDisk events.  (note that CompressThread locks the
// mutex during OnStartInThread, which ensures that the ZipToDisk event blocks; preventing
// the SysExecutor's Idle Event from re-enabing savestates and slots.)
//
static Mutex mtx_CompressToDisk;

static void CheckVersion( pxInputStream& thr )
{
	u32 savever;
	thr.Read( savever );
	
	// Major version mismatch.  Means we can't load this savestate at all.  Support for it
	// was removed entirely.
	if( savever > g_SaveVersion )
		throw Exception::SaveStateLoadError( thr.GetStreamName() )
			.SetDiagMsg(pxsFmt( L"Savestate uses an unsupported or unknown savestate version.\n(PCSX2 ver=%x, state ver=%x)", g_SaveVersion, savever ))
			.SetUserMsg(_("Cannot load this savestate.  The state is from an incompatible edition of PCSX2 that is either newer than this version, or is no longer supported."));

	// check for a "minor" version incompatibility; which happens if the savestate being loaded is a newer version
	// than the emulator recognizes.  99% chance that trying to load it will just corrupt emulation or crash.
	if( (savever >> 16) != (g_SaveVersion >> 16) )
		throw Exception::SaveStateLoadError( thr.GetStreamName() )
			.SetDiagMsg(pxsFmt( L"Savestate uses an unknown (future?!) savestate version.\n(PCSX2 ver=%x, state ver=%x)", g_SaveVersion, savever ))
			.SetUserMsg(_("Cannot load this savestate. The state is an unsupported version, likely created by a newer edition of PCSX2."));
};

// --------------------------------------------------------------------------------------
//  SysExecEvent_DownloadState
// --------------------------------------------------------------------------------------
// Pauses core emulation and downloads the savestate into the state_buffer.
//
class SysExecEvent_DownloadState : public SysExecEvent
{
protected:
	ArchiveEntryList*	m_dest_list;

public:
	wxString GetEventName() const { return L"VM_Download"; }

	virtual ~SysExecEvent_DownloadState() throw() {}
	SysExecEvent_DownloadState* Clone() const { return new SysExecEvent_DownloadState( *this ); }
	SysExecEvent_DownloadState( ArchiveEntryList* dest_list=NULL )
	{
		m_dest_list = dest_list;
	}

	bool IsCriticalEvent() const { return true; }
	bool AllowCancelOnExit() const { return false; }
	
protected:
	void InvokeEvent()
	{
		ScopedCoreThreadPause paused_core;

		if( !SysHasValidState() )
			throw Exception::RuntimeError()
				.SetDiagMsg(L"SysExecEvent_DownloadState: Cannot freeze/download an invalid VM state!")
				.SetUserMsg(L"There is no active virtual machine state to download or save." );

		memSavingState saveme( m_dest_list->GetBuffer() );
		ArchiveEntry internals( EntryFilename_InternalStructures );
		internals.SetDataIndex( saveme.GetCurrentPos() );

		saveme.FreezeAll();

		internals.SetDataSize( saveme.GetCurrentPos() - internals.GetDataIndex() );
		m_dest_list->Add( internals );
		
		for (uint i=0; i<NumSavestateEntries; ++i)
		{
			m_dest_list->Add( ArchiveEntry( SavestateEntries[i]->GetFilename() )
				.SetDataIndex( saveme.GetCurrentPos() )
				.SetDataSize( SavestateEntries[i]->GetDataSize() )
			);

			saveme.FreezeMem( SavestateEntries[i]->GetDataPtr(), SavestateEntries[i]->GetDataSize() );
		}

		UI_EnableStateActions();
		paused_core.AllowResume();
	}
};


// --------------------------------------------------------------------------------------
//  CompressThread_VmState
// --------------------------------------------------------------------------------------
class VmStateCompressThread : public BaseCompressThread
{
	typedef BaseCompressThread _parent;

protected:
	ScopedLock		m_lock_Compress;

public:
	VmStateCompressThread()
	{
		m_lock_Compress.Assign(mtx_CompressToDisk);
	}

	virtual ~VmStateCompressThread() throw()
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
	ArchiveEntryList*	m_src_list;
	wxString			m_filename;

public:
	wxString GetEventName() const { return L"VM_ZipToDisk"; }

	virtual ~SysExecEvent_ZipToDisk() throw()
	{
	}

	SysExecEvent_ZipToDisk* Clone() const { return new SysExecEvent_ZipToDisk( *this ); }

	SysExecEvent_ZipToDisk( ScopedPtr<ArchiveEntryList>& srclist, const wxString& filename )
		: m_filename( filename )
	{
		m_src_list = srclist.DetachPtr();
	}

	SysExecEvent_ZipToDisk( ArchiveEntryList* srclist, const wxString& filename )
		: m_filename( filename )
	{
		m_src_list = srclist;
	}

	bool IsCriticalEvent() const { return true; }
	bool AllowCancelOnExit() const { return false; }

protected:
	void InvokeEvent()
	{
		// Provisionals for scoped cleanup, in case of exception:
		ScopedPtr<ArchiveEntryList> elist( m_src_list );

		wxString tempfile( m_filename + L".tmp" );

		wxFFileOutputStream* woot = new wxFFileOutputStream(tempfile);
		if (!woot->IsOk())
			throw Exception::CannotCreateStream(tempfile);

		// Write the version and screenshot:
		ScopedPtr<pxOutputStream> out( new pxOutputStream(tempfile, new wxZipOutputStream(woot)) );
		wxZipOutputStream* gzfp = (wxZipOutputStream*)out->GetWxStreamBase();

		{
			wxZipEntry* vent = new wxZipEntry(EntryFilename_StateVersion);
			vent->SetMethod( wxZIP_METHOD_STORE );
			gzfp->PutNextEntry( vent );
			out->Write(g_SaveVersion);
			gzfp->CloseEntry();
		}

		ScopedPtr<wxImage> m_screenshot;
		
		if (m_screenshot)
		{
			wxZipEntry* vent = new wxZipEntry(EntryFilename_Screenshot);
			vent->SetMethod( wxZIP_METHOD_STORE );
			gzfp->PutNextEntry( vent );
			m_screenshot->SaveFile( *gzfp, wxBITMAP_TYPE_JPEG );
			gzfp->CloseEntry();
		}


		//m_gzfp->PutNextEntry(EntryFilename_Screenshot);
		//m_gzfp->Write();
		//m_gzfp->CloseEntry();

		(*new VmStateCompressThread())
			.SetSource(m_src_list)
			.SetOutStream(out)
			.SetFinishedPath(m_filename)
			.Start();

		// No errors?  Release cleanup handlers:			
		elist.DetachPtr();
	}
	
	void CleanupEvent()
	{
	}
};

// --------------------------------------------------------------------------------------
//  SysExecEvent_UnzipFromDisk
// --------------------------------------------------------------------------------------
// Note: Unzipping always goes directly into the SysCoreThread's static VM state, and is
// always a blocking action on the SysExecutor thread (the system cannot execute other
// commands while states are unzipping or uploading into the system).
//
class SysExecEvent_UnzipFromDisk : public SysExecEvent
{
protected:
	wxString	m_filename;
	
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

		// Ugh.  Exception handling made crappy because wxWidgets classes don't support scoped pointers yet.

		ScopedPtr<wxFFileInputStream> woot( new wxFFileInputStream(m_filename) );
		if (!woot->IsOk())
			throw Exception::CannotCreateStream( m_filename ).SetDiagMsg(L"Cannot open file for reading.");

		ScopedPtr<pxInputStream> reader( new pxInputStream(m_filename, new wxZipInputStream(woot)) );
		woot.DetachPtr();

		if (!reader->IsOk())
		{
			throw Exception::SaveStateLoadError( m_filename )
				.SetDiagMsg( L"Savestate file is not a valid gzip archive." )
				.SetUserMsg(_("This savestate cannot be loaded because it is not a valid gzip archive.  It may have been created by an older unsupported version of PCSX2, or it may be corrupted."));
		}

		wxZipInputStream* gzreader = (wxZipInputStream*)reader->GetWxStreamBase();

		// look for version and screenshot information in the zip stream:

		bool foundVersion = false;
		//bool foundScreenshot = false;
		//bool foundEntry[NumSavestateEntries] = false;

		ScopedPtr<wxZipEntry> foundInternal;
		ScopedPtr<wxZipEntry> foundEntry[NumSavestateEntries];

		while(true)
		{
			Threading::pxTestCancel();

			ScopedPtr<wxZipEntry> entry( gzreader->GetNextEntry() );
			if (!entry) break;

			if (entry->GetName().CmpNoCase(EntryFilename_StateVersion) == 0)
			{
				DevCon.WriteLn(L" ... found '%s'", EntryFilename_StateVersion);
				foundVersion = true;
				CheckVersion(*reader);
				continue;
			}

			if (entry->GetName().CmpNoCase(EntryFilename_InternalStructures) == 0)
			{
				DevCon.WriteLn(L" ... found '%s'", EntryFilename_InternalStructures);
				foundInternal = entry.DetachPtr();
				continue;
			}

			// No point in finding screenshots when loading states -- the screenshots are
			// only useful for the UI savestate browser.
			/*if (entry->GetName().CmpNoCase(EntryFilename_Screenshot) == 0)
			{
				foundScreenshot = true;
			}*/

			for (uint i=0; i<NumSavestateEntries; ++i)
			{
				if (entry->GetName().CmpNoCase(SavestateEntries[i]->GetFilename()) == 0)
				{
					DevCon.WriteLn( Color_Green, L" ... found '%s'", SavestateEntries[i]->GetFilename() );
					foundEntry[i] = entry.DetachPtr();
					break;
				}
			}
		}

		if (!foundVersion || !foundInternal)
		{
			throw Exception::SaveStateLoadError( m_filename )
				.SetDiagMsg( pxsFmt(L"Savestate file does not contain '%s'",
					!foundVersion ? EntryFilename_StateVersion : EntryFilename_InternalStructures) )
				.SetUserMsg(_("This file is not a valid PCSX2 savestate.  See the logfile for details."));
		}

		// Log any parts and pieces that are missing, and then generate an exception.
		bool throwIt = false;
		for (uint i=0; i<NumSavestateEntries; ++i)
		{
			if (foundEntry[i]) continue;
			throwIt = true;
			Console.WriteLn( Color_Red, " ... not found '%s'!", SavestateEntries[i]->GetFilename() );
		}

		if (throwIt)
			throw Exception::SaveStateLoadError( m_filename )
				.SetDiagMsg( L"Savestate cannot be loaded: some required components were not found or are incomplete." )
				.SetUserMsg(_("This savestate cannot be loaded due to missing critical components.  See the log file for details."));

		// We use direct Suspend/Resume control here, since it's desirable that emulation
		// *ALWAYS* start execution after the new savestate is loaded.

		GetCoreThread().Pause();
		SysClearExecutionCache();

		for (uint i=0; i<NumSavestateEntries; ++i)
		{
			Threading::pxTestCancel();

			gzreader->OpenEntry( *foundEntry[i] );
			const uint entrySize		= foundEntry[i]->GetSize();
			const uint expectedSize		= SavestateEntries[i]->GetDataSize();

			if (entrySize < expectedSize)
			{
				Console.WriteLn( Color_Yellow, " '%s' is incomplete (expected 0x%x bytes, loading only 0x%x bytes)",
					SavestateEntries[i]->GetFilename(), expectedSize, entrySize );
			}

			uint copylen = std::min(entrySize, expectedSize);
			reader->Read( SavestateEntries[i]->GetDataPtr(), copylen );
		}		

		// Load all the internal data

		gzreader->OpenEntry( *foundInternal );

		VmStateBuffer buffer( foundInternal->GetSize(), L"StateBuffer_UnzipFromDisk" );		// start with an 8 meg buffer to avoid frequent reallocation.
		reader->Read( buffer.GetPtr(), foundInternal->GetSize() );

		memLoadingState( buffer ).FreezeAll();
		GetCoreThread().Resume();	// force resume regardless of emulation state earlier.
	}
};

// =====================================================================================================
//  StateCopy Public Interface
// =====================================================================================================

void StateCopy_SaveToFile( const wxString& file )
{
	UI_DisableStateActions();

	ScopedPtr<ArchiveEntryList>	ziplist	(new ArchiveEntryList( new VmStateBuffer( L"Zippable Savestate" ) ));
	
	GetSysExecutorThread().PostEvent(new SysExecEvent_DownloadState	( ziplist ));
	GetSysExecutorThread().PostEvent(new SysExecEvent_ZipToDisk		( ziplist, file ));
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
