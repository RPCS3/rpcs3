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
#include "PsxCommon.h"
#include "SaveState.h"

#include "CDVDisodrv.h"
#include "VUmicro.h"
#include "VU.h"
#include "iCore.h"
#include "iVUzerorec.h"

#include "GS.h"
#include "COP0.h"
#include "Cache.h"


using namespace R5900;

extern int g_psxWriteOk;
extern void recResetEE();
extern void recResetIOP();

static void PreLoadPrep()
{
	SysResetExecutionState();
}

static void PostLoadPrep()
{
	memzero_obj(pCache);
//	WriteCP0Status(cpuRegs.CP0.n.Status.val);
	for(int i=0; i<48; i++) MapTLB(i);
}

void SaveState::GetFilename( string& dest, int slot )
{
	string elfcrcText;
	ssprintf( elfcrcText, "%8.8X.%3.3d", ElfCRC, slot );
	Path::Combine( dest, SSTATES_DIR, elfcrcText );
}

string SaveState::GetFilename( int slot )
{
	string elfcrcText, dest;
	GetFilename( dest, slot );
	return dest;
}


SaveState::SaveState( const char* msg, const string& destination ) : m_version( g_SaveVersion )
{
	Console::WriteLn( "%s %hs", params msg, &destination );
}

s32 CALLBACK gsSafeFreeze( int mode, freezeData *data )
{
	if( mtgsThread != NULL )
	{
		if( mode == 2 )
			return GSfreeze( 2, data );

		// have to call in thread, otherwise weird stuff will start happening
		mtgsThread->SendPointerPacket( GS_RINGTYPE_FREEZE, mode, data );
		mtgsWaitGS();
		return 0;
	}
	else
	{
		// Single threaded...
		return GSfreeze( mode, data );
	}
}

void SaveState::FreezeAll()
{
	if( IsLoading() )
		PreLoadPrep();

	FreezeMem(PS2MEM_BASE, Ps2MemSize::Base);	// 32 MB main memory   
	FreezeMem(PS2MEM_ROM, Ps2MemSize::Rom);		// 4 mb rom memory
	FreezeMem(PS2MEM_ROM1, Ps2MemSize::Rom1);	// 256kb rom1 memory
	FreezeMem(PS2MEM_SCRATCH, Ps2MemSize::Scratch);	// scratch pad 
	FreezeMem(PS2MEM_HW, Ps2MemSize::Hardware);			// hardware memory

	Freeze(cpuRegs);   // cpu regs + COP0
	Freeze(psxRegs);   // iop regs
	Freeze(fpuRegs);   // fpu regs
	Freeze(tlb);           // tlbs

	Freeze(EEsCycle);
	Freeze(EEoCycle);
	Freeze(psxRegs.cycle);		// used to be IOPoCycle.  This retains compatibility.
	Freeze(g_nextBranchCycle);
	Freeze(g_psxNextBranchCycle);

	Freeze(s_iLastCOP0Cycle);
	Freeze(s_iLastPERFCycle);

	Freeze(g_psxWriteOk);
	
	//hope didn't forgot any cpu....

	rcntFreeze();
	gsFreeze();
	vuMicroFreeze();
	vif0Freeze();
	vif1Freeze();
	sifFreeze();
	ipuFreeze();
	gifFreeze();

	// iop now
	FreezeMem(psxM, Ps2MemSize::IopRam);        // 2 MB main memory
	FreezeMem(psxH, Ps2MemSize::IopHardware); // hardware memory
	//FreezeMem(psxS, 0x00010000);        // sif memory	

	sioFreeze();
	cdrFreeze();
	cdvdFreeze();
	psxRcntFreeze();
	sio2Freeze();

	FreezePlugin( "GS", gsSafeFreeze );
	FreezePlugin( "SPU2", SPU2freeze );
	FreezePlugin( "DEV9", DEV9freeze );
	FreezePlugin( "USB", USBfreeze );

	if( IsLoading() )
		PostLoadPrep();
}

// this function is yet incomplete.  Version numbers hare still < 0x12 so it won't be run.
// (which is good because it won't work :P)
void SaveState::_testCdvdCrc()
{
	/*if( GetVersion() < 0x0012 ) return;

	u32 thiscrc = ElfCRC;
	Freeze( thiscrc );
	if( thiscrc != ElfCRC )
		throw Exception::StateCrcMismatch( thiscrc, ElfCRC );*/
}

/////////////////////////////////////////////////////////////////////////////
// gzipped to/from disk state saves implementation

gzBaseStateInfo::gzBaseStateInfo( const char* msg, const string& filename ) :
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


gzSavingState::gzSavingState( const string& filename ) :
  gzBaseStateInfo( _("Saving state to: "), filename )
{
	m_file = gzopen(filename.c_str(), "wb");
	if( m_file == NULL )
		throw Exception::FileNotFound();

	Freeze( m_version );
}


gzLoadingState::gzLoadingState( const string& filename ) :
  gzBaseStateInfo( _("Loading state from: "), filename )
{
	m_file = gzopen(filename.c_str(), "rb");
	if( m_file == NULL )
		throw Exception::FileNotFound();

	gzread( m_file, &m_version, 4 );

	if( m_version != g_SaveVersion )
	{
		if( ( m_version >> 16 ) == 0x7a30 )
		{
			Console::Error(
				"Savestate load aborted:\n"
				"\tVTLB edition cannot safely load savestates created by the VM edition." );
			throw Exception::UnsupportedStateVersion( m_version );
		}
	}

	_testCdvdCrc();
}

gzLoadingState::~gzLoadingState() { }


void gzSavingState::FreezeMem( void* data, int size )
{
	gzwrite( m_file, data, size );
}

void gzLoadingState::FreezeMem( void* data, int size )
{
	gzread( m_file, data, size );
	if( gzeof( m_file ) )
		throw Exception::BadSavedState( m_filename );
}

void gzSavingState::FreezePlugin( const char* name, s32 (CALLBACK *freezer)(int mode, freezeData *data) )
{
	Console::WriteLn( "\tSaving %s", params name );
	freezeData fP = { 0, NULL };

	if (freezer(FREEZE_SIZE, &fP) == -1)
		throw Exception::FreezePluginFailure( name, "saving" );

	gzwrite(m_file, &fP.size, sizeof(fP.size));
	if( fP.size == 0 ) return;

	fP.data = (s8*)malloc(fP.size);
	if (fP.data == NULL)
		throw Exception::OutOfMemory();

	if(freezer(FREEZE_SAVE, &fP) == -1)
		throw Exception::FreezePluginFailure( name, "saving" );

	if (fP.size)
	{
		gzwrite(m_file, fP.data, fP.size);
		free(fP.data);
	}
}

void gzLoadingState::FreezePlugin( const char* name, s32 (CALLBACK *freezer)(int mode, freezeData *data) )
{
	freezeData fP = { 0, NULL };
	Console::WriteLn( "\tLoading %s", params name );

	gzread(m_file, &fP.size, sizeof(fP.size));
	if( fP.size == 0 ) return;

	fP.data = (s8*)malloc(fP.size);
	if (fP.data == NULL)
		throw Exception::OutOfMemory();
	gzread(m_file, fP.data, fP.size);

	if( gzeof( m_file ) )
		throw Exception::BadSavedState( m_filename );

	if(freezer(FREEZE_LOAD, &fP) == -1)
		throw Exception::FreezePluginFailure( name, "loading" );

	if (fP.size) free(fP.data);
}

//////////////////////////////////////////////////////////////////////////////////
// uncompressed to/from memory state saves implementation

memBaseStateInfo::memBaseStateInfo( SafeArray<u8>& memblock, const char* msg ) :
  SaveState( msg, "Memory" )
, m_memory( memblock )
, m_idx( 0 )
{
	// Always clear the MTGS thread state.
	mtgsWaitGS();
}

memSavingState::memSavingState( SafeArray<u8>& save_to ) : memBaseStateInfo( save_to, _("Saving state to: ") )
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
	memBaseStateInfo( load_from, _("Loading state from: ") )
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
		throw Exception::BadSavedState( "memory" );
	}

	fP.data = ((s8*)m_memory.GetPtr()) + m_idx;
	if(freezer(FREEZE_LOAD, &fP) == -1)
		throw Exception::FreezePluginFailure( name, "loading" );

	m_idx += fP.size;
}
