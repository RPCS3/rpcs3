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

#include "PrecompiledHeader.h"

#include "Misc.h"
#include "MemoryCard.h"
#include "Paths.h"

#ifdef WIN32
extern void NTFS_CompressFile( const char* file, bool compressMode );
#endif

FILE* MemoryCard::cardfile[2] = { NULL, NULL };


// Ensures memory card files are created/initialized.
void MemoryCard::Init()
{
	for( int i=0; i<2; i++ )
	{
		if( Config.Mcd[i].Enabled && cardfile[i] == NULL )
			cardfile[i] = Load(i);
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

	if(cardfile[mcd] == NULL) return;
	fclose( cardfile[mcd] );
	cardfile[mcd] = NULL;
}

bool MemoryCard::IsPresent( uint mcd )
{
	jASSUME( mcd < 2 );
	return cardfile[mcd] != NULL;
}

FILE *MemoryCard::Load( uint mcd )
{
	FILE *f;

	jASSUME( mcd < 2 );
	string str( Config.Mcd[mcd].Filename );

	if( str.empty() )
		Path::Combine( str, MEMCARDS_DIR, fmt_string( "Mcd00%d.ps2", mcd ) );

	if( !Path::Exists(str) )
		Create( str.c_str() );

#ifdef WIN32
	NTFS_CompressFile( str.c_str(), Config.McdEnableNTFS );
#endif

	f = fopen( str.c_str(), "r+b" );

	if (f == NULL)
	{
		Msgbox::Alert("Failed loading MemoryCard from file: %hs", params &str); 
		return NULL;
	}

	return f;
}

void MemoryCard::Seek( FILE *f, u32 adr )
{
	u32 size;

	fseek(f, 0, SEEK_END); size = ftell(f);
	if (size == MCD_SIZE + 64)
		fseek(f, adr + 64, SEEK_SET);
	else if (size == MCD_SIZE + 3904)
		fseek(f, adr + 3904, SEEK_SET);
	else
		fseek(f, adr, SEEK_SET);
}

void MemoryCard::Read( uint mcd, u8 *data, u32 adr, int size )
{
	jASSUME( mcd < 2 );
	FILE* const mcfp = cardfile[mcd];

	if( mcfp == NULL )
	{
		Console::Error( "MemoryCard > Ignoring attempted read from disabled card." );
		memset(data, 0, size);
		return;
	}
	Seek(mcfp, adr);
	fread(data, 1, size, mcfp);
}

void MemoryCard::Save( uint mcd, const u8 *data, u32 adr, int size )
{
	jASSUME( mcd < 2 );
	FILE* const mcfp = cardfile[mcd];

	if( mcfp == NULL )
	{
		Console::Error( "MemoryCard > Ignoring attempted save/write to disabled card." );
		return;
	}

	Seek(mcfp, adr);
	u8 *currentdata = (u8 *)malloc(size);
	fread(currentdata, 1, size, mcfp);

	for (int i=0; i<size; i++)
	{
		if ((currentdata[i] & data[i]) != data[i])
			Console::Notice("MemoryCard : writing odd data");
		currentdata[i] &= data[i];
	}

	Seek(mcfp, adr);
	fwrite(currentdata, 1, size, mcfp);
	free(currentdata);
}


void MemoryCard::Erase( uint mcd, u32 adr )
{
	u8 data[528*16];
	memset8_obj<0xff>(data);		// clears to -1's

	jASSUME( mcd < 2 );
	FILE* const mcfp = cardfile[mcd];

	if( mcfp == NULL )
	{
		DevCon::Notice( "MemoryCard > Ignoring seek for disabled card." );
		return;
	}

	Seek(mcfp, adr);
	fwrite(data, 1, 528*16, mcfp);
}


void MemoryCard::Create( const char *mcdFile )
{
	//int enc[16] = {0x77,0x7f,0x7f,0x77,0x7f,0x7f,0x77,0x7f,0x7f,0x77,0x7f,0x7f,0,0,0,0};

	FILE* fp = fopen( mcdFile, "wb" );
	if( fp == NULL ) return;
	for( uint i=0; i<16384; i++ ) 
	{
		for( uint j=0; j<528; j++ ) fputc( 0xFF,fp );
		//		for(j=0; j<16; j++) fputc(enc[j],fp);
	}
	fclose( fp );
}

u64 MemoryCard::GetCRC( uint mcd )
{
	jASSUME( mcd < 2 );

	FILE* const mcfp = cardfile[mcd];
	if( mcfp == NULL ) return 0;

	Seek( mcfp, 0 );

	u64 retval = 0;
	for( uint i=MC2_SIZE/sizeof(u64); i; --i )
	{
		u64 temp; fread( &temp, sizeof(temp), 1, mcfp );
		retval ^= temp;
	}

	return retval;
}