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
extern void NTFS_CompressFile( const char* file );
#endif

static FILE* MemoryCard[2] = { NULL, NULL };

// Ensures memory card files are created/initialized.
void MemoryCard_Init()
{
	for( int i=0; i<2; i++ )
	{
		if( MemoryCard[i] == NULL )
			MemoryCard[i] = LoadMcd(i);
	}
}

void MemoryCard_Shutdown()
{
	for( int i=0; i<2; i++ )
	{
		if(MemoryCard[0]) fclose(MemoryCard[i]);
		MemoryCard[0] = NULL;
	}
}

bool TestMcdIsPresent( int mcd )
{
	jASSUME( mcd == 0 || mcd == 1 );
	return MemoryCard[mcd] != NULL;
}

FILE *LoadMcd(int mcd)
{
	string str;
	FILE *f;

	jASSUME( mcd == 0 || mcd == 1 );
	str = (mcd == 0) ? Config.Mcd1 : Config.Mcd2;

	if( str.empty() )
		Path::Combine( str, MEMCARDS_DIR, fmt_string( "Mcd00%d.ps2", mcd ) );

	if( !Path::Exists(str) )
		CreateMcd(str.c_str());

#ifdef WIN32
	NTFS_CompressFile( str.c_str() );
#endif

	f = fopen(str.c_str(), "r+b");

	if (f == NULL) {
		Msgbox::Alert("Failed loading MemCard from file: %hs", params &str); 
		return NULL;
	}

	return f;
}

void SeekMcd(FILE *f, u32 adr)
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

void ReadMcd(int mcd, u8 *data, u32 adr, int size)
{
	jASSUME( mcd == 0 || mcd == 1 );
	FILE* const mcfp = MemoryCard[mcd];

	if (mcfp == NULL) {
		memset(data, 0, size);
		return;
	}
	SeekMcd(mcfp, adr);
	fread(data, 1, size, mcfp);
}

void SaveMcd(int mcd, const u8 *data, u32 adr, int size)
{
	jASSUME( mcd == 0 || mcd == 1 );
	FILE* const mcfp = MemoryCard[mcd];

	SeekMcd(mcfp, adr);
	u8 *currentdata = (u8 *)malloc(size);
	fread(currentdata, 1, size, mcfp);
	for (int i=0; i<size; i++)
	{
		if ((currentdata[i] & data[i]) != data[i])
			Console::Notice("MemoryCard : writing odd data");
		currentdata[i] &= data[i];
	}
	SeekMcd(mcfp, adr);
	fwrite(currentdata, 1, size, mcfp);

	free(currentdata);
}


void EraseMcd(int mcd, u32 adr)
{
	u8 data[528*16];
	memset8_obj<0xff>(data);		// clears to -1's

	jASSUME( mcd == 0 || mcd == 1 );
	FILE* const mcfp = MemoryCard[mcd];
	SeekMcd(mcfp, adr);
	fwrite(data, 1, 528*16, mcfp);
}


void CreateMcd(const char *mcd)
{
	FILE *fp;	
	int i=0, j=0;
	//int enc[16] = {0x77,0x7f,0x7f,0x77,0x7f,0x7f,0x77,0x7f,0x7f,0x77,0x7f,0x7f,0,0,0,0};

	fp = fopen(mcd, "wb");
	if (fp == NULL) return;
	for(i=0; i < 16384; i++) 
	{
		for(j=0; j < 528; j++) fputc(0xFF,fp);
		//		for(j=0; j < 16; j++) fputc(enc[j],fp);
	}
	fclose(fp);
}

