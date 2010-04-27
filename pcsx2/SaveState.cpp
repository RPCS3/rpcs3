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

#include "ps2/BiosTools.h"
#include "COP0.h"
#include "Cache.h"
#include "AppConfig.h"

#include "Elfheader.h"

using namespace R5900;


static void PreLoadPrep()
{
	SysClearExecutionCache();
}

static void PostLoadPrep()
{
	memzero(pCache);
//	WriteCP0Status(cpuRegs.CP0.n.Status.val);
	for(int i=0; i<48; i++) MapTLB(i);
}

wxString SaveStateBase::GetFilename( int slot )
{
	return (g_Conf->Folders.Savestates +
		wxsFormat( L"%8.8X.%3.3d", ElfCRC, slot )).GetFullPath();
}

SaveStateBase::SaveStateBase( SafeArray<u8>& memblock )
{
	Init( &memblock );
}

SaveStateBase::SaveStateBase( SafeArray<u8>* memblock )
{
	Init( memblock );
}

void SaveStateBase::Init( SafeArray<u8>* memblock )
{
	m_memory	= memblock;
	m_version	= g_SaveVersion;
	m_idx		= 0;
	m_sectid	= FreezeId_Unknown;
	m_pid		= PluginId_GS;
	m_DidBios	= false;
}

void SaveStateBase::PrepBlock( int size )
{
	pxAssumeDev( m_memory, "Savestate memory/buffer pointer is null!" );

	const int end = m_idx+size;
	if( IsSaving() )
		m_memory->MakeRoomFor( end );
	else
	{
		if( m_memory->GetSizeInBytes() < end )
			throw Exception::SaveStateLoadError();
	}
}

void SaveStateBase::FreezeTag( const char* src )
{
	const uint allowedlen = sizeof( m_tagspace )-1;
	pxAssertDev( strlen(src) < allowedlen, wxsFormat( L"Tag name exceeds the allowed length of %d chars.", allowedlen) );

	memzero( m_tagspace );
	strcpy( m_tagspace, src );
	Freeze( m_tagspace );

	if( strcmp( m_tagspace, src ) != 0 )
	{
		pxFail( "Savestate data corruption detected while reading tag" );
		throw Exception::SaveStateLoadError(
			// Untranslated diagnostic msg (use default msg for translation)
			L"Savestate data corruption detected while reading tag: " + fromUTF8(src)
		);
	}
}

void SaveStateBase::FreezeBios()
{
	// Check the BIOS, and issue a warning if the bios for this state
	// doesn't match the bios currently being used (chances are it'll still
	// work fine, but some games are very picky).

	char descin[128], desccmp[128];
	wxString descout;
	IsBIOS( g_Conf->FullpathToBios(), descout );
	memzero( descin );
	memzero( desccmp );

	memcpy_fast( descin, descout.ToUTF8().data(), descout.Length() );
	memcpy_fast( desccmp, descout.ToUTF8().data(), descout.Length() );

	// ... and only freeze bios info once per state, since the user msg could
	// become really annoying on a corrupted state or something.  (have to always
	// load though, so that we advance past the duplicated info, if present)

	if( IsLoading() || !m_DidBios )
		Freeze( descin );

	if( !m_DidBios )
	{
		if( memcmp( descin, desccmp, 128 ) != 0 )
		{
			Console.Newline();
			Console.Indent(1).Error( "Warning: BIOS Version Mismatch, savestate may be unstable!" );
			Console.Indent(2).Error(
				"Current Version:   %s\n"
				"Savestate Version: %s\n",
				descout.ToUTF8().data(), descin
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
	if( IsLoading() )
		PreLoadPrep();

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
	if( IsLoading() )
		PreLoadPrep();

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
#ifdef ENABLE_NEW_IOPDMA
	iopDmacFreeze();
#endif
	psxRcntFreeze();
	sioFreeze();
	sio2Freeze();
	cdrFreeze();
	cdvdFreeze();

	if( IsLoading() )
		PostLoadPrep();
}

void SaveStateBase::WritebackSectionLength( int seekpos, int sectlen, const wxChar* sectname )
{
	int realsectsize = m_idx - seekpos;
	if( IsSaving() )
	{
		// write back the section length...
		*((u32*)m_memory->GetPtr(seekpos-4)) = realsectsize;
	}
	else	// IsLoading!!
	{
		if( sectlen != realsectsize )		// if they don't match then we have a problem, jim.
		{
			throw Exception::SaveStateLoadError( wxEmptyString,
				wxsFormat( L"Invalid size encountered on section '%s'.", sectname ),
				_("The savestate data is invalid or corrupted.")
			);
		}
	}
}

bool SaveStateBase::FreezeSection( int seek_section )
{
	const bool isSeeking = (seek_section != FreezeId_NotSeeking );
	if( IsSaving() ) pxAssertDev( !isSeeking, "Cannot seek on a saving-mode savestate stream." );

	Freeze( m_sectid );
	if( seek_section == m_sectid ) return false;

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
				throw Exception::SaveStateLoadError( wxEmptyString,
					L"Invalid size encountered on BiosVersion section.",
					_("The savestate data is invalid or corrupted.")
				);
			}

			if( isSeeking )
				m_idx += sectlen;
			else
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
				throw Exception::SaveStateLoadError( wxEmptyString,
					L"Invalid size encountered on MainMemory section.",
					_("The savestate data is invalid or corrupted.")
				);
			}

			if( isSeeking )
				m_idx += sectlen;
			else
				FreezeMainMemory();

			int realsectsize = m_idx - seekpos;
			pxAssert( sectlen == realsectsize );
			m_sectid++;
		}
		break;

		case FreezeId_Registers:
		{
			FreezeTag( "HardwareRegisters" );
			int seekpos = m_idx+4;
			int sectlen = 0xdead;	// gets written back over with "real" data in IsSaving() mode

			Freeze( sectlen );
			FreezeRegisters();

			WritebackSectionLength( seekpos, sectlen, L"HardwareRegisters" );
			m_sectid++;
		}
		break;

		case FreezeId_Plugin:
		{
			FreezeTag( "Plugin" );
			int seekpos = m_idx+4;
			int sectlen = 0xdead;	// gets written back over with "real" data in IsSaving() mode

			Freeze( sectlen );
			Freeze( m_pid );

			if( isSeeking )
				m_idx += sectlen;
			else
				GetCorePlugins().Freeze( (PluginsEnum_t)m_pid, *this );

			WritebackSectionLength( seekpos, sectlen, L"Plugins" );

			// following increments only affect Saving mode, which needs to be sure to save all
			// plugins (order doesn't matter but sequential is easy enough. (ignored by Loading mode)
			m_pid++;
			if( m_pid >= PluginId_Count )
				m_sectid = FreezeId_End;
		}
		break;

		case FreezeId_Unknown:
		default:
			pxAssert( IsSaving() );

			// Skip unknown sections with a warning log.
			// Maybe it'll work!  (haha?)

			int size;
			Freeze( m_tagspace );
			Freeze( size );
			m_tagspace[sizeof(m_tagspace)-1] = 0;

			Console.Warning(
				"Warning: Unknown tag encountered while loading savestate; going to ignore it!\n"
				"\tTagname: %s, Size: %d", m_tagspace, size
			);
			m_idx += size;
		break;
	}

	if( wxThread::IsMain() )
		wxSafeYield( NULL, true );

	return true;
}

void SaveStateBase::FreezeAll()
{
	if( IsSaving() )
	{
		// Loading mode streams will assign these, but saving mode reads them so better
		// do some setup first.

		m_sectid	= (int)FreezeId_End+1;
		m_pid		= PluginId_GS;
	}

	while( FreezeSection() );
}

//////////////////////////////////////////////////////////////////////////////////
// uncompressed to/from memory state saves implementation

memSavingState::memSavingState( SafeArray<u8>& save_to )
	: SaveStateBase( save_to )
{
}

memSavingState::memSavingState( SafeArray<u8>* save_to )
	: SaveStateBase( save_to )
{
}

// Saving of state data
void memSavingState::FreezeMem( void* data, int size )
{
	m_memory->MakeRoomFor( m_idx+size );
	memcpy_fast( m_memory->GetPtr(m_idx), data, size );
	m_idx += size;
}

void memSavingState::FreezeAll()
{
	pxAssumeDev( m_memory, "Savestate memory/buffer pointer is null!" );

	// 90% of all savestates fit in under 45 megs (and require more than 43 megs, so might as well...)
	m_memory->ChunkSize = ReallocThreshold;
	m_memory->MakeRoomFor( MemoryBaseAllocSize );

	_parent::FreezeAll();
}

memLoadingState::memLoadingState( const SafeArray<u8>& load_from )
	: SaveStateBase( const_cast<SafeArray<u8>&>(load_from) )
{
}

memLoadingState::memLoadingState( const SafeArray<u8>* load_from )
	: SaveStateBase( const_cast<SafeArray<u8>*>(load_from) )
{
}

memLoadingState::~memLoadingState() throw() { }

// Loading of state data
void memLoadingState::FreezeMem( void* data, int size )
{
	const u8* const src = m_memory->GetPtr(m_idx);
	m_idx += size;
	memcpy_fast( data, src, size );
}

bool memLoadingState::SeekToSection( PluginsEnum_t pid )
{
	m_idx = 0;		// start from the beginning

	do
	{
		while( FreezeSection( FreezeId_Plugin ) );
		if( m_sectid == FreezeId_End ) return false;

		FreezeTag( "Plugin" );
		int sectlen = 0xdead;

		Freeze( sectlen );
		Freeze( m_pid );

	} while( m_pid != pid );
	return true;
}

// --------------------------------------------------------------------------------------
//  SaveState Exception Messages
// --------------------------------------------------------------------------------------

wxString Exception::UnsupportedStateVersion::FormatDiagnosticMessage() const
{
	// Note: no stacktrace needed for this one...
	return wxsFormat( L"Unknown or unsupported savestate version: 0x%x", Version );
}

wxString Exception::UnsupportedStateVersion::FormatDisplayMessage() const
{
	// m_message_user contains a recoverable savestate error which is helpful to the user.
	return wxsFormat(
		m_message_user + L"\n\n" +
		wxsFormat( _("Cannot load savestate.  It is of an unknown or unsupported version."), Version )
	);
}

wxString Exception::StateCrcMismatch::FormatDiagnosticMessage() const
{
	// Note: no stacktrace needed for this one...
	return wxsFormat(
		L"Game/CDVD does not match the savestate CRC.\n"
		L"\tCdvd CRC: 0x%X\n\tGame CRC: 0x%X\n",
		Crc_Savestate, Crc_Cdvd
	);
}

wxString Exception::StateCrcMismatch::FormatDisplayMessage() const
{
	return wxsFormat(
		m_message_user + L"\n\n" +
		wxsFormat(
			L"Savestate game/crc mismatch. Cdvd CRC: 0x%X Game CRC: 0x%X\n",
			Crc_Savestate, Crc_Cdvd
		)
	);
}
