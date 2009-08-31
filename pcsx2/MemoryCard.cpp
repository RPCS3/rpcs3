/*  Pcsx2 - Pc Ps2 Emulator
 *  Copyright (C) 2002-2008  Pcsx2 Team
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

// Note: This file is meant to be part of the HostGui/App, and not part of the
// emu.  The emu accesses it from the SIO interface.  It's possible this could be
// transformed into a plugin, although the benefit of such a system probably isn't
// significant.

#include "PrecompiledHeader.h"

#include "System.h"
#include "MemoryCard.h"
#include "AppConfig.h"

#include <wx/file.h>

#ifdef WIN32
extern void NTFS_CompressFile( const wxString& file, bool compressMode );
#endif

wxFile MemoryCard::cardfile[2];


// Ensures memory card files are created/initialized.
void MemoryCard::Init()
{
	for( int i=0; i<2; i++ )
	{
		if( g_Conf->Mcd[i].Enabled && !cardfile[i].IsOpened() )
			Load( i );
	}
}

void MemoryCard::Shutdown()
{
	for( int i=0; i<2; i++ )
		Unload( i );
}

void MemoryCard::Unload( uint mcd )
{
	jASSUME( mcd < 2 );
	cardfile[mcd].Close();
}

bool MemoryCard::IsPresent( uint mcd )
{
	jASSUME( mcd < 2 );
	return cardfile[mcd].IsOpened();
}

void MemoryCard::Load( uint mcd )
{
	jASSUME( mcd < 2 );
	wxFileName fname( g_Conf->FullpathToMcd( mcd ) );
	wxString str( fname.GetFullPath() );

	if( !fname.FileExists() )
		Create( str );

#ifdef _WIN32
	NTFS_CompressFile( str, g_Conf->McdEnableNTFS );
#endif

	cardfile[mcd].Open( str.c_str(), wxFile::write );

	if( !cardfile[mcd].IsOpened() )
	{
		// Translation note: detailed description should mention that the memory card will be disabled
		// for the duration of this session.
		Msgbox::Alert( pxE( ".Popup:MemoryCard:FailedtoOpen",
			wxsFormat(
				L"Could not load or create a MemoryCard from the file:\n\n%s\n\n"
				L"The MemoryCard in slot %d has been automatically disabled.  You can correct the problem\n"
				L"and re-enable the MemoryCard at any time using Config:MemoryCards from the main menu.",
				str.c_str(), mcd
			) )
		); 
	}
}

void MemoryCard::Seek( wxFile& f, u32 adr )
{
	u32 size = f.Length();
	
	if (size == MCD_SIZE + 64)
		f.Seek( adr + 64 );
	else if (size == MCD_SIZE + 3904)
		f.Seek( adr + 3904 );
	else
		f.Seek( adr );
}

void MemoryCard::Read( uint mcd, u8 *data, u32 adr, int size )
{
	jASSUME( mcd < 2 );
	wxFile& mcfp( cardfile[mcd] );

	if( !mcfp.IsOpened() )
	{
		DevCon::Error( "MemoryCard: Ignoring attempted read from disabled card." );
		memset(data, 0, size);
		return;
	}
	Seek(mcfp, adr);
	mcfp.Read( data, size );
}

void MemoryCard::Save( uint mcd, const u8 *data, u32 adr, int size )
{
	jASSUME( mcd < 2 );
	wxFile& mcfp( cardfile[mcd] );

	if( !mcfp.IsOpened() )
	{
		DevCon::Error( "MemoryCard: Ignoring attempted save/write to disabled card." );
		return;
	}

	Seek(mcfp, adr);
	u8 *currentdata = (u8 *)malloc(size);
	mcfp.Read( currentdata, size);

	for (int i=0; i<size; i++)
	{
		if ((currentdata[i] & data[i]) != data[i])
			Console::Notice("MemoryCard : writing odd data");
		currentdata[i] &= data[i];
	}

	Seek(mcfp, adr);
	mcfp.Write( currentdata, size );
	free(currentdata);
}


void MemoryCard::Erase( uint mcd, u32 adr )
{
	u8 data[528*16];
	memset8_obj<0xff>(data);		// clears to -1's

	jASSUME( mcd < 2 );
	wxFile& mcfp( cardfile[mcd] );

	if( !mcfp.IsOpened() )
	{
		DevCon::Error( "MemoryCard: Ignoring seek for disabled card." );
		return;
	}

	Seek(mcfp, adr);
	mcfp.Write( data, sizeof(data) );
}


void MemoryCard::Create( const wxString& mcdFile )
{
	//int enc[16] = {0x77,0x7f,0x7f,0x77,0x7f,0x7f,0x77,0x7f,0x7f,0x77,0x7f,0x7f,0,0,0,0};

	wxFile fp( mcdFile, wxFile::write );
	if( !fp.IsOpened() ) return;

	u8 effeffs[528];
	memset8_obj<0xff>( effeffs );

	for( uint i=0; i<16384; i++ ) 
		fp.Write( effeffs, sizeof(effeffs) );
}

u64 MemoryCard::GetCRC( uint mcd )
{
	jASSUME( mcd < 2 );

	wxFile& mcfp( cardfile[mcd] );
	if( !mcfp.IsOpened() ) return 0;

	Seek( mcfp, 0 );

	u64 retval = 0;
	for( uint i=MC2_SIZE/sizeof(u64); i; --i )
	{
		u64 temp; mcfp.Read( &temp, sizeof(temp) );
		retval ^= temp;
	}

	return retval;
}