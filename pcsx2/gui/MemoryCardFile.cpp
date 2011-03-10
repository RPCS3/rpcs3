/*  PCSX2 - PS2 Emulator for PCs
 *  Copyright (C) 2002-2010  PCSX2 Dev Team
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
#include "Utilities/SafeArray.inl"

#include "MemoryCardFile.h"

// IMPORTANT!  If this gets a macro redefinition error it means PluginCallbacks.h is included
// in a global-scope header, and that's a BAD THING.  Include it only into modules that need
// it, because some need to be able to alter its behavior using defines.  Like this:

struct Component_FileMcd;
#define PS2E_THISPTR Component_FileMcd*

#include "System.h"
#include "AppConfig.h"

#if _MSC_VER
#	include "svnrev.h"
#endif

#include <wx/ffile.h>

static const int MCD_SIZE	= 1024 *  8  * 16;		// Legacy PSX card default size

static const int MC2_MBSIZE	= 1024 * 528 * 2;		// Size of a single megabyte of card data
static const int MC2_SIZE	= MC2_MBSIZE * 8;		// PS2 card default size (8MB)

// --------------------------------------------------------------------------------------
//  FileMemoryCard
// --------------------------------------------------------------------------------------
// Provides thread-safe direct file IO mapping.
//
class FileMemoryCard
{
protected:
	wxFFile			m_file[8];
	u8				m_effeffs[528*16];
	SafeArray<u8>	m_currentdata;

public:
	FileMemoryCard();
	virtual ~FileMemoryCard() throw() {}

	void Lock();
	void Unlock();

	void Open();
	void Close();

	s32  IsPresent	( uint slot );
	void GetSizeInfo( uint slot, PS2E_McdSizeInfo& outways );
	s32  Read		( uint slot, u8 *dest, u32 adr, int size );
	s32  Save		( uint slot, const u8 *src, u32 adr, int size );
	s32  EraseBlock	( uint slot, u32 adr );
	u64  GetCRC		( uint slot );

protected:
	bool Seek( wxFFile& f, u32 adr );
	bool Create( const wxString& mcdFile, uint sizeInMB );

	wxString GetDisabledMessage( uint slot ) const
	{
		return pxE( "!Notice:Mcd:HasBeenDisabled", wxsFormat(
			L"The memory card in slot %d has been automatically disabled.  You can correct the problem\n"
			L"and re-enable the memory card at any time using Config:Memory cards from the main menu.",
			slot
		) );
	}
};

uint FileMcd_GetMtapPort(uint slot)
{
	switch( slot )
	{
		case 0: case 2: case 3: case 4: return 0;
		case 1: case 5: case 6: case 7: return 1;

		jNO_DEFAULT
	}

	return 0;		// technically unreachable.
}

// Returns the multitap slot number, range 1 to 3 (slot 0 refers to the standard
// 1st and 2nd player slots).
uint FileMcd_GetMtapSlot(uint slot)
{
	switch( slot )
	{
		case 0: case 1:
			pxFailDev( "Invalid parameter in call to GetMtapSlot -- specified slot is one of the base slots, not a Multitap slot." );
		break;

		case 2: case 3: case 4: return slot-1;
		case 5: case 6: case 7: return slot-4;

		jNO_DEFAULT
	}

	return 0;		// technically unreachable.
}

bool FileMcd_IsMultitapSlot( uint slot )
{
	return (slot > 1);
}
/*
wxFileName FileMcd_GetSimpleName(uint slot)
{
	if( FileMcd_IsMultitapSlot(slot) )
		return g_Conf->Folders.MemoryCards + wxsFormat( L"Mcd-Multitap%u-Slot%02u.ps2", FileMcd_GetMtapPort(slot)+1, FileMcd_GetMtapSlot(slot)+1 );
	else
		return g_Conf->Folders.MemoryCards + wxsFormat( L"Mcd%03u.ps2", slot+1 );
}
*/
wxString FileMcd_GetDefaultName(uint slot)
{
	if( FileMcd_IsMultitapSlot(slot) )
		return wxsFormat( L"Mcd-Multitap%u-Slot%02u.ps2", FileMcd_GetMtapPort(slot)+1, FileMcd_GetMtapSlot(slot)+1 );
	else
		return wxsFormat( L"Mcd%03u.ps2", slot+1 );
}

FileMemoryCard::FileMemoryCard()
{
	memset8<0xff>( m_effeffs );
}

void FileMemoryCard::Open()
{
	for( int slot=0; slot<8; ++slot )
	{
		if( FileMcd_IsMultitapSlot(slot) )
		{
			if( !EmuConfig.MultitapPort0_Enabled && (FileMcd_GetMtapPort(slot) == 0) ) continue;
			if( !EmuConfig.MultitapPort1_Enabled && (FileMcd_GetMtapPort(slot) == 1) ) continue;
		}

		wxFileName fname( g_Conf->FullpathToMcd( slot ) );
		wxString str( fname.GetFullPath() );
		bool cont = false;

		if( fname.GetFullName().IsEmpty() )
		{
			str = L"[empty filename]";
			cont = true;
		}

		if( !g_Conf->Mcd[slot].Enabled )
		{
			str = L"[disabled]";
			cont = true;
		}

		Console.WriteLn( cont ? Color_Gray : Color_Green, L"McdSlot %u: " + str, slot );
		if( cont ) continue;

		const wxULongLong fsz = fname.GetSize();
		if( (fsz == 0) || (fsz == wxInvalidSize) )
		{
			// FIXME : Ideally this should prompt the user for the size of the
			// memory card file they would like to create, instead of trying to
			// create one automatically.
		
			if( !Create( str, 8 ) )
			{
				Msgbox::Alert(
					wxsFormat(_( "Could not create a memory card file: \n\n%s\n\n" ), str.c_str()) +
					GetDisabledMessage( slot )
				);
			}
		}

		// [TODO] : Add memcard size detection and report it to the console log.
		//   (8MB, 256Mb, formatted, unformatted, etc ...)

#ifdef __WXMSW__
		NTFS_CompressFile( str, g_Conf->McdCompressNTFS );
#endif

		if( !m_file[slot].Open( str.c_str(), L"r+b" ) )
		{
			// Translation note: detailed description should mention that the memory card will be disabled
			// for the duration of this session.
			Msgbox::Alert(
				wxsFormat(_( "Access denied to memory card file: \n\n%s\n\n" ), str.c_str()) +
				GetDisabledMessage( slot )
			);
		}
	}
}

void FileMemoryCard::Close()
{
	for( int slot=0; slot<8; ++slot )
		m_file[slot].Close();
}

// Returns FALSE if the seek failed (is outside the bounds of the file).
bool FileMemoryCard::Seek( wxFFile& f, u32 adr )
{
	const u32 size = f.Length();

	// If anyone knows why this filesize logic is here (it appears to be related to legacy PSX
	// cards, perhaps hacked support for some special emulator-specific memcard formats that
	// had header info?), then please replace this comment with something useful.  Thanks!  -- air

	u32 offset = 0;

	if( size == MCD_SIZE + 64 )
		offset = 64;
	else if( size == MCD_SIZE + 3904 )
		offset = 3904;
	else
	{
		// perform sanity checks here?
	}

	return f.Seek( adr + offset );
}

// returns FALSE if an error occurred (either permission denied or disk full)
bool FileMemoryCard::Create( const wxString& mcdFile, uint sizeInMB )
{
	//int enc[16] = {0x77,0x7f,0x7f,0x77,0x7f,0x7f,0x77,0x7f,0x7f,0x77,0x7f,0x7f,0,0,0,0};

	Console.WriteLn( L"(FileMcd) Creating new %uMB memory card: " + mcdFile, sizeInMB );

	wxFFile fp( mcdFile, L"wb" );
	if( !fp.IsOpened() ) return false;

	for( uint i=0; i<(MC2_MBSIZE*sizeInMB)/sizeof(m_effeffs); i++ )
	{
		if( fp.Write( m_effeffs, sizeof(m_effeffs) ) == 0 )
			return false;
	}
	return true;
}

s32 FileMemoryCard::IsPresent( uint slot )
{
	return m_file[slot].IsOpened();
}

void FileMemoryCard::GetSizeInfo( uint slot, PS2E_McdSizeInfo& outways )
{
	outways.SectorSize			= 512;
	outways.EraseBlockSizeInSectors			= 16;

	if( pxAssert( m_file[slot].IsOpened() ) )
		outways.McdSizeInSectors	= m_file[slot].Length() / (outways.SectorSize + outways.EraseBlockSizeInSectors);
	else
		outways.McdSizeInSectors	= 0x4000;
}

s32 FileMemoryCard::Read( uint slot, u8 *dest, u32 adr, int size )
{
	wxFFile& mcfp( m_file[slot] );
	if( !mcfp.IsOpened() )
	{
		DevCon.Error( "(FileMcd) Ignoring attempted read from disabled card." );
		memset(dest, 0, size);
		return 1;
	}
	if( !Seek(mcfp, adr) ) return 0;
	return mcfp.Read( dest, size ) != 0;
}

s32 FileMemoryCard::Save( uint slot, const u8 *src, u32 adr, int size )
{
	wxFFile& mcfp( m_file[slot] );

	if( !mcfp.IsOpened() )
	{
		DevCon.Error( "(FileMcd) Ignoring attempted save/write to disabled card." );
		return 1;
	}

	if( !Seek(mcfp, adr) ) return 0;
	m_currentdata.MakeRoomFor( size );
	mcfp.Read( m_currentdata.GetPtr(), size);

	for (int i=0; i<size; i++)
	{
		if ((m_currentdata[i] & src[i]) != src[i])
			Console.Warning("(FileMcd) Warning: writing to uncleared data.");
		m_currentdata[i] &= src[i];
	}

	if( !Seek(mcfp, adr) ) return 0;
	return mcfp.Write( m_currentdata.GetPtr(), size ) != 0;
}

s32 FileMemoryCard::EraseBlock( uint slot, u32 adr )
{
	wxFFile& mcfp( m_file[slot] );

	if( !mcfp.IsOpened() )
	{
		DevCon.Error( "MemoryCard: Ignoring erase for disabled card." );
		return 1;
	}

	if( !Seek(mcfp, adr) ) return 0;
	return mcfp.Write( m_effeffs, sizeof(m_effeffs) ) != 0;
}

u64 FileMemoryCard::GetCRC( uint slot )
{
	wxFFile& mcfp( m_file[slot] );
	if( !mcfp.IsOpened() ) return 0;

	if( !Seek( mcfp, 0 ) ) return 0;

	// Process the file in 4k chunks.  Speeds things up significantly.
	u64 retval = 0;
	u64 buffer[528*8];		// use 528 (sector size), ensures even divisibility
	
	const uint filesize = mcfp.Length() / sizeof(buffer);
	for( uint i=filesize; i; --i )
	{
		mcfp.Read( &buffer, sizeof(buffer) );
		for( uint t=0; t<ArraySize(buffer); ++t )
			retval ^= buffer[t];
	}

	return retval;
}

// --------------------------------------------------------------------------------------
//  MemoryCard Component API Bindings
// --------------------------------------------------------------------------------------

struct Component_FileMcd
{
	PS2E_ComponentAPI_Mcd	api;	// callbacks the plugin provides back to the emulator
	FileMemoryCard			impl;	// class-based implementations we refer to when API is invoked

	Component_FileMcd();
};

uint FileMcd_ConvertToSlot( uint port, uint slot )
{
	if( slot == 0 ) return port;
	if( port == 0 ) return slot+1;		// multitap 1
	return slot + 4;					// multitap 2
}

static void PS2E_CALLBACK FileMcd_EmuOpen( PS2E_THISPTR thisptr, const PS2E_SessionInfo *session )
{
	thisptr->impl.Open();
}

static void PS2E_CALLBACK FileMcd_EmuClose( PS2E_THISPTR thisptr )
{
	thisptr->impl.Close();
}

static s32 PS2E_CALLBACK FileMcd_IsPresent( PS2E_THISPTR thisptr, uint port, uint slot )
{
	return thisptr->impl.IsPresent( FileMcd_ConvertToSlot( port, slot ) );
}

static void PS2E_CALLBACK FileMcd_GetSizeInfo( PS2E_THISPTR thisptr, uint port, uint slot, PS2E_McdSizeInfo* outways )
{
	thisptr->impl.GetSizeInfo( FileMcd_ConvertToSlot( port, slot ), *outways );
}

static s32 PS2E_CALLBACK FileMcd_Read( PS2E_THISPTR thisptr, uint port, uint slot, u8 *dest, u32 adr, int size )
{
	return thisptr->impl.Read( FileMcd_ConvertToSlot( port, slot ), dest, adr, size );
}

static s32 PS2E_CALLBACK FileMcd_Save( PS2E_THISPTR thisptr, uint port, uint slot, const u8 *src, u32 adr, int size )
{
	return thisptr->impl.Save( FileMcd_ConvertToSlot( port, slot ), src, adr, size );
}

static s32 PS2E_CALLBACK FileMcd_EraseBlock( PS2E_THISPTR thisptr, uint port, uint slot, u32 adr )
{
	return thisptr->impl.EraseBlock( FileMcd_ConvertToSlot( port, slot ), adr );
}

static u64 PS2E_CALLBACK FileMcd_GetCRC( PS2E_THISPTR thisptr, uint port, uint slot )
{
	return thisptr->impl.GetCRC( FileMcd_ConvertToSlot( port, slot ) );
}

Component_FileMcd::Component_FileMcd()
{
	memzero( api );

	api.Base.EmuOpen	= FileMcd_EmuOpen;
	api.Base.EmuClose	= FileMcd_EmuClose;

	api.McdIsPresent	= FileMcd_IsPresent;
	api.McdGetSizeInfo	= FileMcd_GetSizeInfo;
	api.McdRead			= FileMcd_Read;
	api.McdSave			= FileMcd_Save;
	api.McdEraseBlock	= FileMcd_EraseBlock;
	api.McdGetCRC		= FileMcd_GetCRC;
}


// --------------------------------------------------------------------------------------
//  Library API Implementations
// --------------------------------------------------------------------------------------
static const char* PS2E_CALLBACK FileMcd_GetName()
{
	return "PlainJane Mcd";
}

static const PS2E_VersionInfo* PS2E_CALLBACK FileMcd_GetVersion( u32 component )
{
	static const PS2E_VersionInfo version = { 0,1,0, SVN_REV };
	return &version;
}

static s32 PS2E_CALLBACK FileMcd_Test( u32 component, const PS2E_EmulatorInfo* xinfo )
{
	if( component != PS2E_TYPE_Mcd ) return 0;

	// Check and make sure the user has a hard drive?
	// Probably not necessary :p
	return 1;
}

static PS2E_THISPTR PS2E_CALLBACK FileMcd_NewComponentInstance( u32 component )
{
	if( component != PS2E_TYPE_Mcd ) return NULL;

	try
	{
		return new Component_FileMcd();
	}
	catch( std::bad_alloc& )
	{
		Console.Error( "Allocation failed on Component_FileMcd! (out of memory?)" );
	}
	return NULL;
}

static void PS2E_CALLBACK FileMcd_DeleteComponentInstance( PS2E_THISPTR instance )
{
	delete instance;
}

static void PS2E_CALLBACK FileMcd_SetSettingsFolder( const char* folder )
{
}

static void PS2E_CALLBACK FileMcd_SetLogFolder( const char* folder )
{
}

static const PS2E_LibraryAPI FileMcd_Library =
{
	FileMcd_GetName,
	FileMcd_GetVersion,
	FileMcd_Test,
	FileMcd_NewComponentInstance,
	FileMcd_DeleteComponentInstance,
	FileMcd_SetSettingsFolder,
	FileMcd_SetLogFolder
};

// If made into an external plugin, this function should be renamed to PS2E_InitAPI, so that
// PCSX2 can find the export in the expected location.
extern "C" const PS2E_LibraryAPI* FileMcd_InitAPI( const PS2E_EmulatorInfo* emuinfo )
{
	return &FileMcd_Library;
}

// --------------------------------------------------------------------------------------
//  Currently Unused Superblock Header Struct
// --------------------------------------------------------------------------------------
// (provided for reference purposes)

struct superblock
{
	char magic[28]; 			// 0x00
	char version[12]; 			// 0x1c
	u16 page_len; 				// 0x28
	u16 pages_per_cluster;	 	// 0x2a
	u16 pages_per_block;		// 0x2c
	u16 unused; 				// 0x2e
	u32 clusters_per_card;	 	// 0x30
	u32 alloc_offset; 			// 0x34
	u32 alloc_end; 				// 0x38
	u32 rootdir_cluster;		// 0x3c
	u32 backup_block1;			// 0x40
	u32 backup_block2;			// 0x44
	u32 ifc_list[32]; 			// 0x50
	u32 bad_block_list[32]; 	// 0xd0
	u8 card_type; 				// 0x150
	u8 card_flags; 				// 0x151
};
