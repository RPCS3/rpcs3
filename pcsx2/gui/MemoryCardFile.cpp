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
#include "Utilities/SafeArray.h"

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

#include <wx/file.h>

static const int MCD_SIZE = 1024 *  8  * 16;
static const int MC2_SIZE = 1024 * 528 * 16;

// --------------------------------------------------------------------------------------
//  FileMemoryCard 
// --------------------------------------------------------------------------------------
// Provides thread-safe direct file IO mapping.
//
class FileMemoryCard
{
protected:
	wxFile			m_file[2][4];
	u8				m_effeffs[528*16];
	SafeArray<u8>	m_currentdata;

public:
	FileMemoryCard();
	virtual ~FileMemoryCard() {}

	void Lock();
	void Unlock();

	s32 IsPresent( uint port, uint slot );
	s32 Read( uint port, uint slot, u8 *dest, u32 adr, int size );
	s32 Save( uint port, uint slot, const u8 *src, u32 adr, int size );
	s32 EraseBlock( uint port, uint slot, u32 adr );
	u64 GetCRC( uint port, uint slot );

protected:
	bool Seek( wxFile& f, u32 adr );
	bool Create( const wxString& mcdFile );

	wxString GetDisabledMessage( uint port, uint slot ) const
	{
		return pxE( ".Popup:MemoryCard:HasBeenDisabled", wxsFormat(
			L"The MemoryCard in port %d/slot %d has been automatically disabled.  You can correct the problem\n"
			L"and re-enable the MemoryCard at any time using Config:MemoryCards from the main menu.",
			port, slot
		) );
	}
};

FileMemoryCard::FileMemoryCard()
{
	memset8<0xff>( m_effeffs );

	for( int port=0; port<2; ++port )
	{
		for( int slot=0; slot<4; ++slot )
		{
			if( !g_Conf->Mcd[port][slot].Enabled || g_Conf->Mcd[port][slot].Filename.GetFullName().IsEmpty() ) continue;

			wxFileName fname( g_Conf->FullpathToMcd( port, slot ) );
			wxString str( fname.GetFullPath() );

			const wxULongLong fsz = fname.GetSize();
			if( (fsz == 0) || (fsz == wxInvalidSize) )
			{
				if( !Create( str ) )
				{
					Msgbox::Alert(
						wxsFormat( _( "Could not create a MemoryCard file: \n\n%s\n\n" ), str.c_str() ) + 
						GetDisabledMessage( port, slot )
					);
				}
			}

			// [TODO] : Add memcard size detection and report it to the console log.
			//   (8MB, 256Mb, whatever)

			NTFS_CompressFile( str, g_Conf->McdEnableNTFS );

			if( !m_file[port][slot].Open( str.c_str(), wxFile::read_write ) )
			{
				// Translation note: detailed description should mention that the memory card will be disabled
				// for the duration of this session.
				Msgbox::Alert(
					wxsFormat( _( "Access denied to MemoryCard file: \n\n%s\n\n" ), str.c_str() ) + 
					GetDisabledMessage( port, slot )
				); 
			}
		}
	}
}

// Returns FALSE if the seek failed (is outside the bounds of the file).
bool FileMemoryCard::Seek( wxFile& f, u32 adr )
{
	const u32 size = f.Length();

	// If anyone knows why this filesize logic is here (it appears to be related to legacy PSX
	// cards, perhaps hacked support for some special memcard formats that had header info?),
	// then please replace this comment with something useful.  Thanks!  -- air

	u32 offset = 0;
	
	if( size == MCD_SIZE + 64 )
		offset = 64;
	else if( size == MCD_SIZE + 3904 )
		offset = 3904;
	else
	{
		// perform sanity checks here?
	}

	return wxInvalidOffset != f.Seek( adr + offset );
}

// returns FALSE if an error occurred (either permission denied or disk full)
bool FileMemoryCard::Create( const wxString& mcdFile )
{
	//int enc[16] = {0x77,0x7f,0x7f,0x77,0x7f,0x7f,0x77,0x7f,0x7f,0x77,0x7f,0x7f,0,0,0,0};

	wxFile fp( mcdFile, wxFile::write );
	if( !fp.IsOpened() ) return false;

	for( uint i=0; i<MC2_SIZE/sizeof(m_effeffs); i++ )
	{
		if( fp.Write( m_effeffs, sizeof(m_effeffs) ) == 0 )
			return false;
	}
	return true;
}

s32 FileMemoryCard::IsPresent( uint port, uint slot )
{
	return m_file[port][slot].IsOpened();
}

s32 FileMemoryCard::Read( uint port, uint slot, u8 *dest, u32 adr, int size )
{
	wxFile& mcfp( m_file[port][slot] );
	if( !mcfp.IsOpened() )
	{
		DevCon.Error( "MemoryCard: Ignoring attempted read from disabled card." );
		memset(dest, 0, size);
		return 1;
	}
	if( !Seek(mcfp, adr) ) return 0;
	return mcfp.Read( dest, size ) != 0;
}

s32 FileMemoryCard::Save( uint port, uint slot, const u8 *src, u32 adr, int size )
{
	wxFile& mcfp( m_file[port][slot] );

	if( !mcfp.IsOpened() )
	{
		DevCon.Error( "MemoryCard: Ignoring attempted save/write to disabled card." );
		return 1;
	}

	if( !Seek(mcfp, adr) ) return 0;
	m_currentdata.MakeRoomFor( size );
	mcfp.Read( m_currentdata.GetPtr(), size);

	for (int i=0; i<size; i++)
	{
		if ((m_currentdata[i] & src[i]) != src[i])
			Console.Notice("MemoryCard: (warning) writing to uncleared data.");
		m_currentdata[i] &= src[i];
	}

	if( !Seek(mcfp, adr) ) return 0;
	return mcfp.Write( m_currentdata.GetPtr(), size ) != 0;
}

s32 FileMemoryCard::EraseBlock( uint port, uint slot, u32 adr )
{
	wxFile& mcfp( m_file[port][slot] );

	if( !mcfp.IsOpened() )
	{
		DevCon.Error( "MemoryCard: Ignoring erase for disabled card." );
		return 1;
	}

	if( !Seek(mcfp, adr) ) return 0;
	return mcfp.Write( m_effeffs, sizeof(m_effeffs) ) != 0;
}

u64 FileMemoryCard::GetCRC( uint port, uint slot )
{
	wxFile& mcfp( m_file[port][slot] );
	if( !mcfp.IsOpened() ) return 0;

	if( !Seek( mcfp, 0 ) ) return 0;

	u64 retval = 0;
	for( uint i=MC2_SIZE/sizeof(u64); i; --i )
	{
		u64 temp; mcfp.Read( &temp, sizeof(temp) );
		retval ^= temp;
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

static s32 PS2E_CALLBACK FileMcd_IsPresent( PS2E_THISPTR thisptr, uint port, uint slot )
{
	return thisptr->impl.IsPresent( port, slot );
}

static s32 PS2E_CALLBACK FileMcd_Read( PS2E_THISPTR thisptr, uint port, uint slot, u8 *dest, u32 adr, int size )
{
	return thisptr->impl.Read( port, slot, dest, adr, size );
}

static s32 PS2E_CALLBACK FileMcd_Save( PS2E_THISPTR thisptr, uint port, uint slot, const u8 *src, u32 adr, int size )
{
	return thisptr->impl.Save( port, slot, src, adr, size );
}

static s32 PS2E_CALLBACK FileMcd_EraseBlock( PS2E_THISPTR thisptr, uint port, uint slot, u32 adr )
{
	return thisptr->impl.EraseBlock( port, slot, adr );
}

static u64 PS2E_CALLBACK FileMcd_GetCRC( PS2E_THISPTR thisptr, uint port, uint slot )
{
	return thisptr->impl.GetCRC( port, slot );
}

Component_FileMcd::Component_FileMcd()
{
	api.McdIsPresent	= FileMcd_IsPresent;
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
//  Currently Unused Superblock Header Structs
// --------------------------------------------------------------------------------------

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

#if 0		// unused code?
struct McdBlock
{
	s8 Title[48];
	s8 ID[14];
	s8 Name[16];
	int IconCount;
	u16 Icon[16*16*3];
	u8 Flags;
};

void GetMcdBlockInfo(int mcd, int block, McdBlock *info);
#endif
