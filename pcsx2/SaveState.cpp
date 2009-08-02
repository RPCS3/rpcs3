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

#include "IopCommon.h"
#include "SaveState.h"

#include "CDVD/IsoFSdrv.h"
#include "VUmicro.h"
#include "VU.h"
#include "iCore.h"
#include "sVU_zerorec.h"

#include "GS.h"
#include "COP0.h"
#include "Cache.h"

using namespace R5900;

extern void recResetEE();
extern void recResetIOP();

static void PreLoadPrep()
{
	SysClearExecutionCache();
}

static void PostLoadPrep()
{
	memzero_obj(pCache);
//	WriteCP0Status(cpuRegs.CP0.n.Status.val);
	for(int i=0; i<48; i++) MapTLB(i);
}

wxString SaveState::GetFilename( int slot )
{
	return (g_Conf->Folders.Savestates +
		wxsFormat( L"%8.8X.%3.3d", ElfCRC, slot )).GetFullPath();
}

SaveState::SaveState( const char* msg, const wxString& destination ) :
	m_version( g_SaveVersion )
,	m_tagspace( 128 )
{
	Console::WriteLn( "%s %s", params msg, destination.ToAscii().data() );
}

s32 CALLBACK gsSafeFreeze( int mode, freezeData *data )
{
	// have to call in the GS thread, otherwise weird stuff will start happening
	mtgsThread->SendPointerPacket( GS_RINGTYPE_FREEZE, mode, data );
	mtgsWaitGS();
	return 0;
}

void SaveState::FreezeTag( const char* src )
{
	const int length = strlen( src );
	m_tagspace.MakeRoomFor( length+1 );
	
	strcpy( m_tagspace.GetPtr(), src );
	FreezeMem( m_tagspace.GetPtr(), length );

	if( strcmp( m_tagspace.GetPtr(), src ) != 0 )
	{
		assert( 0 );
		throw Exception::BadSavedState(
			// Untranslated diagnostic msg (use default msg for translation)
			L"Savestate data corruption detected while reading tag: " + wxString::FromAscii(src)
		);
	}
}

void SaveState::FreezeAll()
{
	if( IsLoading() )
		PreLoadPrep();
		
	// Check the BIOS, and issue a warning if the bios for this state
	// doesn't match the bios currently being used (chances are it'll still
	// work fine, but some games are very picky).

	char descin[128];
	wxString descout;
	IsBIOS( g_Conf->FullpathToBios(), descout );
	memcpy_fast( descin, descout.ToAscii().data(), 128 );
	Freeze( descin );
	
	if( memcmp( descin, descout, 128 ) != 0 )
	{
		Console::Error(
			"\n\tWarning: BIOS Version Mismatch, savestate may be unstable!\n"
			"\t\tCurrent BIOS:   %s\n"
			"\t\tSavestate BIOS: %s\n",
			params descout.ToAscii().data(), descin
		);
	}

	// First Block - Memory Dumps
	// ---------------------------
	FreezeMem(PS2MEM_BASE, Ps2MemSize::Base);		// 32 MB main memory   
	FreezeMem(PS2MEM_SCRATCH, Ps2MemSize::Scratch);	// scratch pad 
	FreezeMem(PS2MEM_HW, Ps2MemSize::Hardware);		// hardware memory

	FreezeMem(psxM, Ps2MemSize::IopRam);		// 2 MB main memory
	FreezeMem(psxH, Ps2MemSize::IopHardware);	// hardware memory
	FreezeMem(psxS, 0x000100);					// iop's sif memory	

	// Second Block - Various CPU Registers and States
	// -----------------------------------------------
	FreezeTag( "cpuRegs" );
	Freeze(cpuRegs);   // cpu regs + COP0
	Freeze(psxRegs);   // iop regs
	Freeze(fpuRegs);
	Freeze(tlb);           // tlbs

	// Third Block - Cycle Timers and Events
	// -------------------------------------
	FreezeTag( "Cycles" );
	Freeze(EEsCycle);
	Freeze(EEoCycle);
	Freeze(g_nextBranchCycle);
	Freeze(g_psxNextBranchCycle);
	Freeze(s_iLastCOP0Cycle);
	Freeze(s_iLastPERFCycle);

	// Fourth Block - EE-related systems
	// ---------------------------------
	rcntFreeze();
	gsFreeze();
	vuMicroFreeze();
	vif0Freeze();
	vif1Freeze();
	sifFreeze();
	ipuFreeze();
	gifFreeze();
	sprFreeze();

	// Fifth Block - iop-related systems
	// ---------------------------------
	psxRcntFreeze();
	sioFreeze();
	sio2Freeze();
	cdrFreeze();
	cdvdFreeze();

	// Sixth Block - Plugins Galore!
	// -----------------------------
	FreezePlugin( "GS", gsSafeFreeze );
	
	g_plugins->Freeze( *this );

	if( IsLoading() )
		PostLoadPrep();
}

/////////////////////////////////////////////////////////////////////////////
// gzipped to/from disk state saves implementation

gzBaseStateInfo::gzBaseStateInfo( const char* msg, const wxString& filename ) :
  SaveState( msg, filename )
, m_filename( filename )
, m_file( NULL )
{
}

gzBaseStateInfo::~gzBaseStateInfo()
{
	if( m_file != NULL )
	{
		gzclose( m_file );
		m_file = NULL;
	}
}


gzSavingState::gzSavingState( const wxString& filename ) :
  gzBaseStateInfo( "Saving state to: ", filename )
{
	m_file = gzopen(filename.ToAscii().data(), "wb");
	if( m_file == NULL )
		throw Exception::FileNotFound();

	gzsetparams( m_file, Z_BEST_SPEED, Z_DEFAULT_STRATEGY );
	Freeze( m_version );
}


gzLoadingState::gzLoadingState( const wxString& filename ) :
  gzBaseStateInfo( "Loading state from: ", filename )
{
	m_file = gzopen(filename.ToAscii().data(), "rb");
	if( m_file == NULL )
		throw Exception::FileNotFound();

	gzread( m_file, &m_version, 4 );

	if( (m_version >> 16) != (g_SaveVersion >> 16) )
	{
		Console::Error(
			"Savestate load aborted:\n"
			"\tUnknown or invalid savestate identifier, either from a (very!) old version of\n"
			"\tPcsx2, or the file is corrupted"
		);
		throw Exception::UnsupportedStateVersion( m_version );
	}
	else if( m_version > g_SaveVersion )
	{
		Console::Error(
			"Savestate load aborted:\n"
			"\tThe savestate was created with a newer version of Pcsx2.  I don't know how to load it!" );
		throw Exception::UnsupportedStateVersion( m_version );
	}
}

gzLoadingState::~gzLoadingState() { }


void gzSavingState::FreezeMem( void* data, int size )
{
	gzwrite( m_file, data, size );
}

void gzLoadingState::FreezeMem( void* data, int size )
{
	if( gzread( m_file, data, size ) != size )
		throw Exception::BadSavedState( m_filename );
}

void gzSavingState::FreezePlugin( const char* name, s32 (CALLBACK *freezer)(int mode, freezeData *data) )
{
	freezeData fP = { 0, NULL };
	Console::WriteLn( "\tSaving %s", params name );

	FreezeTag( name );

	if (freezer(FREEZE_SIZE, &fP) == -1)
		throw Exception::FreezePluginFailure( name, "saving" );

	Freeze( fP.size );
	if( fP.size == 0 ) return;

	SafeArray<s8> buffer( fP.size );
	fP.data = buffer.GetPtr();

	if(freezer(FREEZE_SAVE, &fP) == -1)
		throw Exception::FreezePluginFailure( name, "saving" );

	FreezeMem( fP.data, fP.size );
}

void gzLoadingState::FreezePlugin( const char* name, s32 (CALLBACK *freezer)(int mode, freezeData *data) )
{
	freezeData fP = { 0, NULL };
	Console::WriteLn( "\tLoading %s", params name );

	FreezeTag( name );
	Freeze( fP.size );
	if( fP.size == 0 ) return;

	SafeArray<s8> buffer( fP.size );
	fP.data = buffer.GetPtr();

	FreezeMem( fP.data, fP.size );

	if(freezer(FREEZE_LOAD, &fP) == -1)
		throw Exception::FreezePluginFailure( name, "loading" );
}

//////////////////////////////////////////////////////////////////////////////////
// uncompressed to/from memory state saves implementation

memBaseStateInfo::memBaseStateInfo( SafeArray<u8>& memblock, const char* msg ) :
	SaveState( msg, L"Memory")
,	m_memory( memblock )
,	m_idx( 0 )
{
	// Always clear the MTGS thread state.
	mtgsWaitGS();
}

memSavingState::memSavingState( SafeArray<u8>& save_to ) : memBaseStateInfo( save_to, "Saving state to: " )
{
	save_to.ChunkSize = ReallocThreshold;
	save_to.MakeRoomFor( MemoryBaseAllocSize );
}

// Saving of state data to a memory buffer
void memSavingState::FreezeMem( void* data, int size )
{
	const int end = m_idx+size;
	m_memory.MakeRoomFor( end );

	u8* dest = (u8*)m_memory.GetPtr();
	const u8* src = (u8*)data;

	for( ; m_idx<end; ++m_idx, ++src )
		dest[m_idx] = *src;
}

memLoadingState::memLoadingState(SafeArray<u8>& load_from ) : 
	memBaseStateInfo( load_from, "Loading state from: " )
{
}

memLoadingState::~memLoadingState() { }

// Loading of state data from a memory buffer...
void memLoadingState::FreezeMem( void* data, int size )
{
	const int end = m_idx+size;
	const u8* src = (u8*)m_memory.GetPtr();
	u8* dest = (u8*)data;

	for( ; m_idx<end; ++m_idx, ++dest )
		*dest = src[m_idx];
}

void memSavingState::FreezePlugin( const char* name, s32 (CALLBACK *freezer)(int mode, freezeData *data) )
{
	freezeData fP = { 0, NULL };
	Console::WriteLn( "\tSaving %s", params name );

	if( freezer(FREEZE_SIZE, &fP) == -1 )
		throw Exception::FreezePluginFailure( name, "saving" );

	Freeze( fP.size );
	if( fP.size == 0 ) return;

	const int end = m_idx+fP.size;
	m_memory.MakeRoomFor( end );

	fP.data = ((s8*)m_memory.GetPtr()) + m_idx;
	if(freezer(FREEZE_SAVE, &fP) == -1)
		throw Exception::FreezePluginFailure( name, "saving" );

	m_idx += fP.size;
}

void memLoadingState::FreezePlugin( const char* name, s32 (CALLBACK *freezer)(int mode, freezeData *data) )
{
	freezeData fP;
	Console::WriteLn( "\tLoading %s", params name );

	Freeze( fP.size );
	if( fP.size == 0 ) return;

	if( ( fP.size + m_idx ) > m_memory.GetSizeInBytes() )
	{
		assert(0);
		throw Exception::BadSavedState( L"memory");
	}

	fP.data = ((s8*)m_memory.GetPtr()) + m_idx;
	if(freezer(FREEZE_LOAD, &fP) == -1)
		throw Exception::FreezePluginFailure( name, "loading" );

	m_idx += fP.size;
}
