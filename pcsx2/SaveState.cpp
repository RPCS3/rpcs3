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

#include "IopCommon.h"
#include "SaveState.h"

#include "CDVD/IsoFSdrv.h"
#include "ps2/BiosTools.h"

#include "VUmicro.h"
#include "VU.h"
#include "iCore.h"
#include "sVU_zerorec.h"

#include "GS.h"
#include "COP0.h"
#include "Cache.h"
#include "AppConfig.h"

using namespace R5900;

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

wxString SaveStateBase::GetFilename( int slot )
{
	return (g_Conf->Folders.Savestates +
		wxsFormat( L"%8.8X.%3.3d", ElfCRC, slot )).GetFullPath();
}

SaveStateBase::SaveStateBase( SafeArray<u8>& memblock ) :
	m_memory( memblock )	
,	m_version( g_SaveVersion )
,	m_idx( 0 )
,	m_sectid( FreezeId_Unknown )
,	m_pid( PluginId_GS )
,	m_DidBios( false )
{
}

void SaveStateBase::PrepBlock( int size )
{
	const int end = m_idx+size;
	if( IsSaving() )
		m_memory.MakeRoomFor( end );
	else
	{
		if( m_memory.GetSizeInBytes() <= end )
			throw Exception::BadSavedState();
	}
}

void SaveStateBase::FreezeTag( const char* src )
{
	wxASSERT( strlen(src) < (sizeof( m_tagspace )-1) );

	memzero_obj( m_tagspace );
	strcpy( m_tagspace, src );
	Freeze( m_tagspace );

	if( strcmp( m_tagspace, src ) != 0 )
	{
		wxASSERT_MSG( false, L"Savestate data corruption detected while reading tag" );
		throw Exception::BadSavedState(
			// Untranslated diagnostic msg (use default msg for translation)
			L"Savestate data corruption detected while reading tag: " + wxString::FromAscii(src)
		);
	}
}

void SaveStateBase::FreezeBios()
{
	// Check the BIOS, and issue a warning if the bios for this state
	// doesn't match the bios currently being used (chances are it'll still
	// work fine, but some games are very picky).
	
	char descin[128];
	wxString descout;
	IsBIOS( g_Conf->FullpathToBios(), descout );
	memcpy_fast( descin, descout.ToAscii().data(), 128 );

	// ... and only freeze bios info once per state, since the user msg could
	// become really annoying on a corrupted state or something.  (have to always
	// load though, so that we advance past the duplicated info, if present)

	if( IsLoading() || !m_DidBios )
		Freeze( descin );

	if( !m_DidBios )
	{
		if( memcmp( descin, descout.ToAscii().data(), 128 ) != 0 )
		{
			Console::Error(
				"\n\tWarning: BIOS Version Mismatch, savestate may be unstable!\n"
				"\t\tCurrent Version:   %s\n"
				"\t\tSavestate Version: %s\n",
				descout.ToAscii().data(), descin
			);
		}
	}
	m_DidBios = true;
}

static const int MainMemorySizeInBytes = 
	Ps2MemSize::Base + Ps2MemSize::Scratch + Ps2MemSize::Hardware +
	Ps2MemSize::IopRam + Ps2MemSize::IopHardware + 0x0100;

void SaveStateBase::FreezeMainMemory()
{
	// First Block - Memory Dumps
	// ---------------------------
	FreezeMem(PS2MEM_BASE,		Ps2MemSize::Base);		// 32 MB main memory   
	FreezeMem(PS2MEM_SCRATCH,	Ps2MemSize::Scratch);	// scratch pad 
	FreezeMem(PS2MEM_HW,		Ps2MemSize::Hardware);	// hardware memory

	FreezeMem(psxM, Ps2MemSize::IopRam);		// 2 MB main memory
	FreezeMem(psxH, Ps2MemSize::IopHardware);	// hardware memory
	FreezeMem(psxS, 0x000100);					// iop's sif memory	
}

void SaveStateBase::FreezeRegisters()
{
	//if( IsLoading() )
	//	PreLoadPrep();

	// Second Block - Various CPU Registers and States
	// -----------------------------------------------
	FreezeTag( "cpuRegs" );
	Freeze(cpuRegs);		// cpu regs + COP0
	Freeze(psxRegs);		// iop regs
	Freeze(fpuRegs);
	Freeze(tlb);			// tlbs

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
	FreezeTag( "EE-Subsystems" );
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
	FreezeTag( "IOP-Subsystems" );
	psxRcntFreeze();
	sioFreeze();
	sio2Freeze();
	cdrFreeze();
	cdvdFreeze();

	if( IsLoading() )
		PostLoadPrep();
}

bool SaveStateBase::FreezeSection()
{
	Freeze( m_sectid );

	switch( m_sectid )
	{
		case FreezeId_End:
		return false;

		case FreezeId_Bios:
		{
			int sectlen = 128;
			FreezeTag( "BiosVersion" );
			Freeze( sectlen );

			if( sectlen != 128 )
			{
				throw Exception::BadSavedState( wxEmptyString,
					L"Invalid size encountered on BiosVersion section.",
					_("The savestate data is invalid or corrupted.")
				);
			}

			FreezeBios();
			m_sectid++;
		}
		break;

		case FreezeId_Memory:
		{
			FreezeTag( "MainMemory" );

			int seekpos = m_idx+4;
			int sectlen = MainMemorySizeInBytes;
			Freeze( sectlen );
			if( sectlen != MainMemorySizeInBytes )
			{
				throw Exception::BadSavedState( wxEmptyString,
					L"Invalid size encountered on MainMemory section.",
					_("The savestate data is invalid or corrupted.")
				);
			}

			FreezeMainMemory();
			int realsectsize = m_idx - seekpos;
			wxASSERT( sectlen == realsectsize );
			m_sectid++;
		}
		break;
		
		case FreezeId_Registers:
		{
			FreezeTag( "HardwareRegisters" );
			int seekpos = m_idx+4;
			int sectsize;
			Freeze( sectsize );

			FreezeRegisters();
			
			int realsectsize = m_idx - seekpos;
			if( IsSaving() )
			{
				// write back the section length...
				*((u32*)m_memory.GetPtr(seekpos)) = realsectsize - 4;
			}
			else	// IsLoading!!
			{
				if( sectsize != realsectsize )		// if they don't match then we have a problem, jim.
				{
					throw Exception::BadSavedState( wxEmptyString,
						L"Invalid size encountered on HardwareRegisters section.",
						_("The savestate data is invalid or corrupted.")
					);
				}
			}
			m_sectid++;
		}
		break;
		
		case FreezeId_Plugin:
		{
			FreezeTag( "Plugin" );
			int seekpos = m_idx+4;
			int sectsize;
			Freeze( sectsize );
			
			Freeze( m_pid );
			g_plugins->Freeze( (PluginsEnum_t)m_pid, *this );

			int realsectsize = m_idx - seekpos;
			if( IsSaving() )
			{
				// write back the section length...
				*((u32*)m_memory.GetPtr(seekpos)) = realsectsize - 4;
			}
			else
			{
				if( sectsize != realsectsize )		// if they don't match then we have a problem, jim.
				{
					throw Exception::BadSavedState( wxEmptyString,
						L"Invalid size encountered on Plugin section.",
						_("The savestate data is invalid or corrupted.")
					);
				}
			}


			// following increments only affect Saving mode, are ignored by Loading mode.
			m_pid++;
			if( m_pid >= PluginId_Count )
				m_sectid = FreezeId_End;
		}
		break;

		case FreezeId_Unknown:
		default:
			wxASSERT( IsSaving() );

			// Skip unknown sections with a warning log.
			// Maybe it'll work!  (haha?)

			int size;
			Freeze( m_tagspace );
			Freeze( size );
			m_tagspace[sizeof(m_tagspace)-1] = 0;

			Console::Notice(
				"Warning: Unknown tag encountered while loading savestate; going to ignore it!\n"
				"\tTagname: %s, Size: %d", m_tagspace, size
			);
			m_idx += size;
		break;
	}

	wxSafeYield( NULL );

	return true;
}

void SaveStateBase::FreezeAll()
{
	m_sectid	= (int)FreezeId_End+1;
	m_pid		= PluginId_GS;

	while( FreezeSection() );
}

//////////////////////////////////////////////////////////////////////////////////
// uncompressed to/from memory state saves implementation

memSavingState::memSavingState( SafeArray<u8>& save_to ) :
	SaveStateBase( save_to )
{
	save_to.ChunkSize = ReallocThreshold;
	save_to.MakeRoomFor( MemoryBaseAllocSize );
}

// Saving of state data
void memSavingState::FreezeMem( void* data, int size )
{
	const int end = m_idx+size;
	m_memory.MakeRoomFor( end );

	u8* dest = (u8*)m_memory.GetPtr();
	const u8* src = (u8*)data;

	for( ; m_idx<end; ++m_idx, ++src )
		dest[m_idx] = *src;
}

memLoadingState::memLoadingState( const SafeArray<u8>& load_from ) : 
	SaveStateBase( const_cast<SafeArray<u8>&>(load_from) )
{
}

memLoadingState::~memLoadingState() { }

// Loading of state data 
void memLoadingState::FreezeMem( void* data, int size )
{
	const int end = m_idx+size;
	const u8* src = (u8*)m_memory.GetPtr();
	u8* dest = (u8*)data;

	for( ; m_idx<end; ++m_idx, ++dest )
		*dest = src[m_idx];
}
